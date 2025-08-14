/**
 * @file ucra_cpp.hpp
 * @brief Modern C++ wrapper for the UCRA C API
 *
 * This header-only library provides a modern C++ interface to the UCRA
 * (Universal Choir Rendering API) with RAII, smart pointers, and exception
 * handling.
 *
 * @copyright Copyright (c) 2025 UCRA Project
 * @license MIT License
 */

#ifndef UCRA_CPP_HPP
#define UCRA_CPP_HPP

#include "ucra/ucra.h"
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <optional>
#include <functional>

namespace ucra {

/**
 * @brief Exception thrown when UCRA operations fail
 */
class UcraException : public std::runtime_error {
public:
    explicit UcraException(UCRA_Result error_code)
        : std::runtime_error(error_code_to_string(error_code))
        , error_code_(error_code) {}

    UcraException(UCRA_Result error_code, const std::string& message)
        : std::runtime_error(message + " (error code: " + std::to_string(error_code) + ")")
        , error_code_(error_code) {}

    UCRA_Result error_code() const noexcept { return error_code_; }

private:
    UCRA_Result error_code_;

    static std::string error_code_to_string(UCRA_Result code) {
        switch (code) {
            case UCRA_SUCCESS: return "Success";
            case UCRA_ERR_INVALID_ARGUMENT: return "Invalid argument";
            case UCRA_ERR_OUT_OF_MEMORY: return "Out of memory";
            case UCRA_ERR_NOT_SUPPORTED: return "Not supported";
            case UCRA_ERR_INTERNAL: return "Internal error";
            case UCRA_ERR_FILE_NOT_FOUND: return "File not found";
            case UCRA_ERR_INVALID_JSON: return "Invalid JSON";
            case UCRA_ERR_INVALID_MANIFEST: return "Invalid manifest";
            default: return "Unknown error";
        }
    }
};

/**
 * @brief Utility function to check UCRA result and throw on error
 */
inline void check_result(UCRA_Result result) {
    if (result != UCRA_SUCCESS) {
        throw UcraException(result);
    }
}

/**
 * @brief RAII wrapper for key-value pairs
 */
class KeyValue {
public:
    KeyValue() = default;
    KeyValue(std::string key, std::string value)
        : key_(std::move(key)), value_(std::move(value)) {}

    const std::string& key() const noexcept { return key_; }
    const std::string& value() const noexcept { return value_; }

    void set_key(std::string key) { key_ = std::move(key); }
    void set_value(std::string value) { value_ = std::move(value); }

private:
    std::string key_;
    std::string value_;
};

/**
 * @brief RAII wrapper for F0 curve data
 */
class F0Curve {
public:
    F0Curve() = default;
    F0Curve(std::vector<float> time_sec, std::vector<float> f0_hz)
        : time_sec_(std::move(time_sec)), f0_hz_(std::move(f0_hz)) {
        if (time_sec_.size() != f0_hz_.size()) {
            throw std::invalid_argument("F0Curve: time and f0 arrays must have same size");
        }
        update_c_struct();
    }

    const std::vector<float>& time_sec() const noexcept { return time_sec_; }
    const std::vector<float>& f0_hz() const noexcept { return f0_hz_; }

    void set_data(std::vector<float> time_sec, std::vector<float> f0_hz) {
        if (time_sec.size() != f0_hz.size()) {
            throw std::invalid_argument("F0Curve: time and f0 arrays must have same size");
        }
        time_sec_ = std::move(time_sec);
        f0_hz_ = std::move(f0_hz);
        update_c_struct();
    }

    const UCRA_F0Curve* c_struct() const noexcept {
        return time_sec_.empty() ? nullptr : &c_curve_;
    }

private:
    std::vector<float> time_sec_;
    std::vector<float> f0_hz_;
    UCRA_F0Curve c_curve_{};

    void update_c_struct() {
        if (!time_sec_.empty()) {
            c_curve_.time_sec = time_sec_.data();
            c_curve_.f0_hz = f0_hz_.data();
            c_curve_.length = static_cast<uint32_t>(time_sec_.size());
        } else {
            c_curve_ = {};
        }
    }
};

/**
 * @brief RAII wrapper for envelope curve data
 */
class EnvCurve {
public:
    EnvCurve() = default;
    EnvCurve(std::vector<float> time_sec, std::vector<float> value)
        : time_sec_(std::move(time_sec)), value_(std::move(value)) {
        if (time_sec_.size() != value_.size()) {
            throw std::invalid_argument("EnvCurve: time and value arrays must have same size");
        }
        update_c_struct();
    }

    const std::vector<float>& time_sec() const noexcept { return time_sec_; }
    const std::vector<float>& value() const noexcept { return value_; }

    void set_data(std::vector<float> time_sec, std::vector<float> value) {
        if (time_sec.size() != value.size()) {
            throw std::invalid_argument("EnvCurve: time and value arrays must have same size");
        }
        time_sec_ = std::move(time_sec);
        value_ = std::move(value);
        update_c_struct();
    }

    const UCRA_EnvCurve* c_struct() const noexcept {
        return time_sec_.empty() ? nullptr : &c_curve_;
    }

private:
    std::vector<float> time_sec_;
    std::vector<float> value_;
    UCRA_EnvCurve c_curve_{};

    void update_c_struct() {
        if (!time_sec_.empty()) {
            c_curve_.time_sec = time_sec_.data();
            c_curve_.value = value_.data();
            c_curve_.length = static_cast<uint32_t>(time_sec_.size());
        } else {
            c_curve_ = {};
        }
    }
};

/**
 * @brief RAII wrapper for note segment data
 */
class NoteSegment {
public:
    NoteSegment() = default;

    NoteSegment(double start_sec, double duration_sec, int16_t midi_note = -1,
                uint8_t velocity = 80, std::string lyric = "")
        : start_sec_(start_sec), duration_sec_(duration_sec)
        , midi_note_(midi_note), velocity_(velocity), lyric_(std::move(lyric)) {
        update_c_struct();
    }

    // Getters
    double start_sec() const noexcept { return start_sec_; }
    double duration_sec() const noexcept { return duration_sec_; }
    int16_t midi_note() const noexcept { return midi_note_; }
    uint8_t velocity() const noexcept { return velocity_; }
    const std::string& lyric() const noexcept { return lyric_; }
    const F0Curve& f0_override() const noexcept { return f0_override_; }
    const EnvCurve& env_override() const noexcept { return env_override_; }

    // Setters
    void set_start_sec(double start_sec) { start_sec_ = start_sec; update_c_struct(); }
    void set_duration_sec(double duration_sec) { duration_sec_ = duration_sec; update_c_struct(); }
    void set_midi_note(int16_t midi_note) { midi_note_ = midi_note; update_c_struct(); }
    void set_velocity(uint8_t velocity) { velocity_ = velocity; update_c_struct(); }
    void set_lyric(std::string lyric) { lyric_ = std::move(lyric); update_c_struct(); }
    void set_f0_override(F0Curve f0_override) { f0_override_ = std::move(f0_override); update_c_struct(); }
    void set_env_override(EnvCurve env_override) { env_override_ = std::move(env_override); update_c_struct(); }

    const UCRA_NoteSegment& c_struct() const noexcept { return c_note_; }

private:
    double start_sec_{0.0};
    double duration_sec_{1.0};
    int16_t midi_note_{-1};
    uint8_t velocity_{80};
    std::string lyric_;
    F0Curve f0_override_;
    EnvCurve env_override_;
    UCRA_NoteSegment c_note_{};

    void update_c_struct() {
        c_note_.start_sec = start_sec_;
        c_note_.duration_sec = duration_sec_;
        c_note_.midi_note = midi_note_;
        c_note_.velocity = velocity_;
        c_note_.lyric = lyric_.empty() ? nullptr : lyric_.c_str();
        c_note_.f0_override = f0_override_.c_struct();
        c_note_.env_override = env_override_.c_struct();
    }
};

/**
 * @brief RAII wrapper for render configuration
 */
class RenderConfig {
public:
    RenderConfig(uint32_t sample_rate = 44100, uint32_t channels = 1,
                 uint32_t block_size = 512, uint32_t flags = 0)
        : sample_rate_(sample_rate), channels_(channels)
        , block_size_(block_size), flags_(flags) {
        update_c_struct();
    }

    // Getters
    uint32_t sample_rate() const noexcept { return sample_rate_; }
    uint32_t channels() const noexcept { return channels_; }
    uint32_t block_size() const noexcept { return block_size_; }
    uint32_t flags() const noexcept { return flags_; }
    const std::vector<NoteSegment>& notes() const noexcept { return notes_; }
    const std::unordered_map<std::string, std::string>& options() const noexcept { return options_; }

    // Setters
    void set_sample_rate(uint32_t sample_rate) { sample_rate_ = sample_rate; update_c_struct(); }
    void set_channels(uint32_t channels) { channels_ = channels; update_c_struct(); }
    void set_block_size(uint32_t block_size) { block_size_ = block_size; update_c_struct(); }
    void set_flags(uint32_t flags) { flags_ = flags; update_c_struct(); }

    void add_note(NoteSegment note) {
        notes_.emplace_back(std::move(note));
        update_c_struct();
    }

    void set_notes(std::vector<NoteSegment> notes) {
        notes_ = std::move(notes);
        update_c_struct();
    }

    void add_option(const std::string& key, const std::string& value) {
        options_[key] = value;
        update_c_struct();
    }

    void set_options(std::unordered_map<std::string, std::string> options) {
        options_ = std::move(options);
        update_c_struct();
    }

    const UCRA_RenderConfig& c_struct() {
        update_c_struct();
        return c_config_;
    }

private:
    uint32_t sample_rate_;
    uint32_t channels_;
    uint32_t block_size_;
    uint32_t flags_;
    std::vector<NoteSegment> notes_;
    std::unordered_map<std::string, std::string> options_;

    // C structure backing
    UCRA_RenderConfig c_config_{};
    std::vector<UCRA_NoteSegment> c_notes_;
    std::vector<UCRA_KeyValue> c_options_;
    std::vector<KeyValue> option_storage_;

    void update_c_struct() {
        // Update notes
        c_notes_.clear();
        c_notes_.reserve(notes_.size());
        for (const auto& note : notes_) {
            c_notes_.push_back(note.c_struct());
        }

        // Update options
        option_storage_.clear();
        c_options_.clear();
        option_storage_.reserve(options_.size());
        c_options_.reserve(options_.size());

        for (const auto& [key, value] : options_) {
            option_storage_.emplace_back(key, value);
            const auto& kv = option_storage_.back();
            c_options_.push_back({kv.key().c_str(), kv.value().c_str()});
        }

        // Update config
        c_config_.sample_rate = sample_rate_;
        c_config_.channels = channels_;
        c_config_.block_size = block_size_;
        c_config_.flags = flags_;
        c_config_.notes = c_notes_.empty() ? nullptr : c_notes_.data();
        c_config_.note_count = static_cast<uint32_t>(c_notes_.size());
        c_config_.options = c_options_.empty() ? nullptr : c_options_.data();
        c_config_.option_count = static_cast<uint32_t>(c_options_.size());
    }
};

/**
 * @brief RAII wrapper for render result
 */
class RenderResult {
public:
    RenderResult() = default;

    // Move-only semantics
    RenderResult(const RenderResult&) = delete;
    RenderResult& operator=(const RenderResult&) = delete;
    RenderResult(RenderResult&&) = default;
    RenderResult& operator=(RenderResult&&) = default;

    void update_from_c_result(const UCRA_RenderResult& c_result) {
        frames_ = c_result.frames;
        channels_ = c_result.channels;
        sample_rate_ = c_result.sample_rate;
        status_ = c_result.status;

        // Copy PCM data
        if (c_result.pcm && c_result.frames > 0) {
            const size_t total_samples = c_result.frames * c_result.channels;
            pcm_.assign(c_result.pcm, c_result.pcm + total_samples);
        } else {
            pcm_.clear();
        }

        // Copy metadata
        metadata_.clear();
        for (uint32_t i = 0; i < c_result.metadata_count; ++i) {
            const auto& kv = c_result.metadata[i];
            if (kv.key && kv.value) {
                metadata_[kv.key] = kv.value;
            }
        }
    }

    // Getters
    const std::vector<float>& pcm() const noexcept { return pcm_; }
    uint64_t frames() const noexcept { return frames_; }
    uint32_t channels() const noexcept { return channels_; }
    uint32_t sample_rate() const noexcept { return sample_rate_; }
    UCRA_Result status() const noexcept { return status_; }
    const std::unordered_map<std::string, std::string>& metadata() const noexcept { return metadata_; }

private:
    std::vector<float> pcm_;
    uint64_t frames_{0};
    uint32_t channels_{0};
    uint32_t sample_rate_{0};
    UCRA_Result status_{UCRA_SUCCESS};
    std::unordered_map<std::string, std::string> metadata_;
};

/**
 * @brief RAII wrapper for UCRA Engine
 */
class Engine {
public:
    /**
     * @brief Create a new UCRA engine
     * @param options Engine creation options
     */
    explicit Engine(const std::unordered_map<std::string, std::string>& options = {}) {
        std::vector<KeyValue> option_storage;
        std::vector<UCRA_KeyValue> c_options;

        option_storage.reserve(options.size());
        c_options.reserve(options.size());

        for (const auto& [key, value] : options) {
            option_storage.emplace_back(key, value);
            const auto& kv = option_storage.back();
            c_options.push_back({kv.key().c_str(), kv.value().c_str()});
        }

        UCRA_Result result = ucra_engine_create(
            &handle_,
            c_options.empty() ? nullptr : c_options.data(),
            static_cast<uint32_t>(c_options.size())
        );

        check_result(result);
    }

    // Move-only semantics
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    Engine(Engine&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    Engine& operator=(Engine&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                ucra_engine_destroy(handle_);
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~Engine() {
        if (handle_) {
            ucra_engine_destroy(handle_);
        }
    }

    /**
     * @brief Get engine information string
     * @return Engine information
     */
    std::string get_info() const {
        char buffer[512];
        UCRA_Result result = ucra_engine_getinfo(handle_, buffer, sizeof(buffer));
        check_result(result);
        return std::string(buffer);
    }

    /**
     * @brief Render audio with the given configuration
     * @param config Render configuration
     * @return Render result
     */
    RenderResult render(RenderConfig& config) const {
        UCRA_RenderResult c_result;
        UCRA_Result result = ucra_render(handle_, &config.c_struct(), &c_result);
        check_result(result);

        RenderResult cpp_result;
        cpp_result.update_from_c_result(c_result);
        return cpp_result;
    }

    /**
     * @brief Get the underlying C handle (for advanced usage)
     * @return UCRA handle
     */
    UCRA_Handle handle() const noexcept { return handle_; }

private:
    UCRA_Handle handle_{nullptr};
};

/**
 * @brief RAII wrapper for UCRA Manifest
 */
class Manifest {
public:
    /**
     * @brief Load manifest from file
     * @param manifest_path Path to manifest file
     */
    explicit Manifest(const std::string& manifest_path) {
        UCRA_Result result = ucra_manifest_load(manifest_path.c_str(), &manifest_);
        check_result(result);
    }

    // Move-only semantics
    Manifest(const Manifest&) = delete;
    Manifest& operator=(const Manifest&) = delete;

    Manifest(Manifest&& other) noexcept : manifest_(other.manifest_) {
        other.manifest_ = nullptr;
    }

    Manifest& operator=(Manifest&& other) noexcept {
        if (this != &other) {
            if (manifest_) {
                ucra_manifest_free(manifest_);
            }
            manifest_ = other.manifest_;
            other.manifest_ = nullptr;
        }
        return *this;
    }

    ~Manifest() {
        if (manifest_) {
            ucra_manifest_free(manifest_);
        }
    }

    // Getters
    std::optional<std::string> name() const {
        return manifest_->name ? std::optional<std::string>(manifest_->name) : std::nullopt;
    }

    std::optional<std::string> version() const {
        return manifest_->version ? std::optional<std::string>(manifest_->version) : std::nullopt;
    }

    std::optional<std::string> vendor() const {
        return manifest_->vendor ? std::optional<std::string>(manifest_->vendor) : std::nullopt;
    }

    std::optional<std::string> license() const {
        return manifest_->license ? std::optional<std::string>(manifest_->license) : std::nullopt;
    }

    /**
     * @brief Get the underlying C manifest (for advanced usage)
     * @return UCRA manifest pointer
     */
    const UCRA_Manifest* c_manifest() const noexcept { return manifest_; }

private:
    UCRA_Manifest* manifest_{nullptr};
};

/**
 * @brief RAII wrapper for UCRA streaming
 */
class Stream {
public:
    using PullCallback = std::function<RenderConfig()>;

    /**
     * @brief Create a new streaming session
     * @param config Base render configuration
     * @param callback Function to call when more data is needed
     */
    Stream(RenderConfig& config, PullCallback callback)
        : callback_(std::move(callback)) {

        UCRA_Result result = ucra_stream_open(
            &handle_,
            &config.c_struct(),
            &Stream::static_pull_callback,
            this
        );

        check_result(result);
    }

    // Move-only semantics
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    Stream(Stream&& other) noexcept
        : handle_(other.handle_), callback_(std::move(other.callback_)) {
        other.handle_ = nullptr;
    }

    Stream& operator=(Stream&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                ucra_stream_close(handle_);
            }
            handle_ = other.handle_;
            callback_ = std::move(other.callback_);
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~Stream() {
        if (handle_) {
            ucra_stream_close(handle_);
        }
    }

    /**
     * @brief Read PCM data from the stream
     * @param frame_count Number of frames to read
     * @return PCM data and actual frames read
     */
    std::pair<std::vector<float>, uint32_t> read(uint32_t frame_count) {
        // Estimate buffer size (assuming stereo for safety)
        std::vector<float> buffer(frame_count * 2);
        uint32_t frames_read = 0;

        UCRA_Result result = ucra_stream_read(
            handle_,
            buffer.data(),
            frame_count,
            &frames_read
        );

        check_result(result);

        // Resize buffer to actual data size
        buffer.resize(frames_read * 2); // Adjust based on actual channel count if needed
        return {std::move(buffer), frames_read};
    }

private:
    UCRA_StreamHandle handle_{nullptr};
    PullCallback callback_;
    RenderConfig current_config_{};

    static UCRA_Result UCRA_CALL static_pull_callback(void* user_data, UCRA_RenderConfig* out_config) {
        try {
            auto* stream = static_cast<Stream*>(user_data);
            stream->current_config_ = stream->callback_();
            *out_config = stream->current_config_.c_struct();
            return UCRA_SUCCESS;
        } catch (...) {
            return UCRA_ERR_INTERNAL;
        }
    }
};

} // namespace ucra

#endif // UCRA_CPP_HPP
