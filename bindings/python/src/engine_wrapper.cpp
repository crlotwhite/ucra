#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <ucra/ucra.h>
#include <stdexcept>
#include <memory>
#include <vector>
#include <cstring>

namespace py = pybind11;

// RAII wrapper for UCRA result checking
void check_ucra_result(UCRA_Result result, const std::string& operation) {
    if (result != UCRA_SUCCESS) {
        std::string message = operation + " failed";
        switch (result) {
            case UCRA_ERR_INVALID_ARGUMENT:
                throw std::invalid_argument(message + " (Invalid argument)");
            case UCRA_ERR_OUT_OF_MEMORY:
                throw std::runtime_error(message + " (Out of memory)");
            case UCRA_ERR_NOT_SUPPORTED:
                throw std::runtime_error(message + " (Not supported)");
            case UCRA_ERR_FILE_NOT_FOUND:
                throw std::runtime_error(message + " (File not found)");
            case UCRA_ERR_INVALID_JSON:
                throw std::runtime_error(message + " (Invalid JSON)");
            case UCRA_ERR_INVALID_MANIFEST:
                throw std::runtime_error(message + " (Invalid manifest)");
            default:
                throw std::runtime_error(message + " (Unknown error)");
        }
    }
}

// Python wrapper for UCRA_NoteSegment with automatic memory management
class PyNoteSegment {
private:
    UCRA_NoteSegment segment_;

public:
    PyNoteSegment(double start_sec, double duration_sec, int midi_note = 69, int velocity = 80, const std::string& lyric = "") {
        if (duration_sec <= 0.0) {
            throw std::invalid_argument("Duration must be positive");
        }
        if (velocity < 0 || velocity > 127) {
            throw std::invalid_argument("Velocity must be between 0 and 127");
        }
        if (midi_note < -1 || midi_note > 127) {
            throw std::invalid_argument("MIDI note must be between -1 and 127");
        }

        segment_.start_sec = start_sec;
        segment_.duration_sec = duration_sec;
        segment_.midi_note = static_cast<int16_t>(midi_note);
        segment_.velocity = static_cast<uint8_t>(velocity);
    // Store lyric and set pointer into stable storage owned by this instance
    lyric_storage_ = lyric;
    segment_.lyric = lyric_storage_.c_str();
    }

    // Delete copy constructor and assignment operator
    PyNoteSegment(const PyNoteSegment&) = delete;
    PyNoteSegment& operator=(const PyNoteSegment&) = delete;

    // Getters
    double get_start_sec() const { return segment_.start_sec; }
    double get_duration_sec() const { return segment_.duration_sec; }
    int get_midi_note() const { return segment_.midi_note; }
    int get_velocity() const { return segment_.velocity; }
    std::string get_lyric() const { return lyric_storage_; }

    // Internal access to raw data
    const UCRA_NoteSegment* get_raw() const { return &segment_; }

private:
    std::string lyric_storage_;
};

// Python wrapper for UCRA_RenderConfig
class PyRenderConfig {
private:
    UCRA_RenderConfig config_;
    std::vector<std::unique_ptr<PyNoteSegment>> notes_;
    // Raw contiguous array mirroring notes_ for passing to C API
    std::vector<UCRA_NoteSegment> notes_raw_;

public:
    PyRenderConfig(uint32_t sample_rate = 44100, uint32_t channels = 1, uint32_t block_size = 512, uint32_t flags = 0) {
        config_.sample_rate = sample_rate;
        config_.channels = channels;
        config_.block_size = block_size;
        config_.flags = flags;
        config_.note_count = 0;
        config_.notes = nullptr;
        config_.options = nullptr;
        config_.option_count = 0;
    }

    // Delete copy constructor and assignment operator
    PyRenderConfig(const PyRenderConfig&) = delete;
    PyRenderConfig& operator=(const PyRenderConfig&) = delete;

    // Getters
    uint32_t get_sample_rate() const { return config_.sample_rate; }
    uint32_t get_channels() const { return config_.channels; }
    uint32_t get_block_size() const { return config_.block_size; }
    uint32_t get_flags() const { return config_.flags; }

    // Note management
    void add_note(PyNoteSegment& note) {
        // Own a copy of note so its lyric storage stays alive
        notes_.push_back(std::make_unique<PyNoteSegment>(
            note.get_start_sec(), note.get_duration_sec(),
            note.get_midi_note(), note.get_velocity(), note.get_lyric()));

        // Rebuild raw array and update config pointer
        notes_raw_.clear();
        notes_raw_.reserve(notes_.size());
        for (const auto& np : notes_) {
            UCRA_NoteSegment ns = *np->get_raw();
            // Ensure lyric pointer points to the owned storage
            // (np->get_raw()->lyric already points into np's storage)
            ns.lyric = np->get_raw()->lyric;
            ns.f0_override = nullptr;
            ns.env_override = nullptr;
            notes_raw_.push_back(ns);
        }
        config_.notes = notes_raw_.empty() ? nullptr : notes_raw_.data();
        config_.note_count = static_cast<uint32_t>(notes_raw_.size());
    }

    size_t get_note_count() const { return config_.note_count; }

    // Internal access to raw pointer
    const UCRA_RenderConfig* get_raw() const { return &config_; }
};

// Python wrapper for UCRA Engine
class PyEngine {
private:
    UCRA_Handle engine_;

public:
    PyEngine(const std::map<std::string, std::string>& options = {}) {
        // Convert map to C-style options array
        std::vector<UCRA_KeyValue> option_array;
        option_array.reserve(options.size());

        for (const auto& pair : options) {
            UCRA_KeyValue opt;
            opt.key = pair.first.c_str();
            opt.value = pair.second.c_str();
            option_array.push_back(opt);
        }

        UCRA_Result result = ucra_engine_create(&engine_,
            option_array.empty() ? nullptr : option_array.data(),
            static_cast<uint32_t>(option_array.size()));
        check_ucra_result(result, "Engine creation");
    }

    ~PyEngine() {
        if (engine_) {
            ucra_engine_destroy(engine_);
        }
    }

    // Delete copy constructor and assignment operator
    PyEngine(const PyEngine&) = delete;
    PyEngine& operator=(const PyEngine&) = delete;

    // Render method returning NumPy array (subclass with metadata attributes)
    py::object render(PyRenderConfig& config) {
        UCRA_RenderResult result_data;
        UCRA_Result result = ucra_render(engine_, config.get_raw(), &result_data);
        check_ucra_result(result, "Rendering");

        // Create NumPy array with the audio data
        size_t total_samples = result_data.frames * result_data.channels;
        auto numpy_result = py::array_t<float>(
            { static_cast<py::ssize_t>(result_data.frames), static_cast<py::ssize_t>(result_data.channels) },
            { sizeof(float) * result_data.channels, sizeof(float) }
        );

        // Copy data to NumPy array
        auto buf = numpy_result.request();
        std::memcpy(buf.ptr, result_data.pcm, total_samples * sizeof(float));

        // Convert to our ndarray subclass so we can attach attributes
        // Fetch the AudioArray class defined in module init (see main.cpp)
        py::module m = py::module::import("ucra");
        py::object audio_cls = m.attr("AudioArray");
        py::object view = numpy_result.attr("view")(audio_cls);

        // Set metadata attributes on the subclass instance
        view.attr("sample_rate") = py::int_(result_data.sample_rate);
        view.attr("frames") = py::int_(result_data.frames);
        view.attr("channels") = py::int_(result_data.channels);

        return view;
    }
};

void bind_engine(py::module& m) {
    py::class_<PyNoteSegment>(m, "NoteSegment")
        .def(py::init<double, double, int, int, const std::string&>(),
             py::arg("start_sec"), py::arg("duration_sec"),
             py::arg("midi_note") = 69, py::arg("velocity") = 80, py::arg("lyric") = "",
             "Create a note segment")
        .def_property_readonly("start_sec", &PyNoteSegment::get_start_sec, "Start time in seconds")
        .def_property_readonly("duration_sec", &PyNoteSegment::get_duration_sec, "Duration in seconds")
        .def_property_readonly("midi_note", &PyNoteSegment::get_midi_note, "MIDI note number")
        .def_property_readonly("velocity", &PyNoteSegment::get_velocity, "Velocity (0-127)")
        .def_property_readonly("lyric", &PyNoteSegment::get_lyric, "Lyric text");

    py::class_<PyRenderConfig>(m, "RenderConfig")
        .def(py::init<uint32_t, uint32_t, uint32_t, uint32_t>(),
             py::arg("sample_rate") = 44100, py::arg("channels") = 1,
             py::arg("block_size") = 512, py::arg("flags") = 0,
             "Create a render configuration")
        .def_property_readonly("sample_rate", &PyRenderConfig::get_sample_rate, "Sample rate in Hz")
        .def_property_readonly("channels", &PyRenderConfig::get_channels, "Number of channels")
        .def_property_readonly("block_size", &PyRenderConfig::get_block_size, "Block size")
        .def_property_readonly("flags", &PyRenderConfig::get_flags, "Render flags")
        .def("add_note", &PyRenderConfig::add_note, py::arg("note"), "Add a note segment")
        .def_property_readonly("note_count", &PyRenderConfig::get_note_count, "Number of notes");

    py::class_<PyEngine>(m, "Engine")
        .def(py::init<const std::map<std::string, std::string>&>(),
             py::arg("options") = std::map<std::string, std::string>{},
             "Create a UCRA engine")
        .def("render", &PyEngine::render, py::arg("config"), "Render audio with given configuration");
}
