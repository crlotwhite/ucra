// Minimal C++ emitter to match golden_output.wav configuration
#include "../ucra_cpp.hpp"
#include <fstream>
#include <vector>
#include <cstdint>

static void write_wav_file(const char* filename, const std::vector<float>& pcm,
                           uint32_t sample_rate, uint32_t channels) {
    std::ofstream file(filename, std::ios::binary);
    uint32_t data_size = static_cast<uint32_t>(pcm.size() * sizeof(float));
    uint32_t file_size = 36 + data_size;
    uint16_t bits_per_sample = 32;
    uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
    uint16_t block_align = channels * (bits_per_sample / 8);

    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&file_size), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    uint32_t fmt_size = 16;
    uint16_t audio_format = 3; // IEEE float
    file.write(reinterpret_cast<const char*>(&fmt_size), 4);
    file.write(reinterpret_cast<const char*>(&audio_format), 2);
    file.write(reinterpret_cast<const char*>(&channels), 2);
    file.write(reinterpret_cast<const char*>(&sample_rate), 4);
    file.write(reinterpret_cast<const char*>(&byte_rate), 4);
    file.write(reinterpret_cast<const char*>(&block_align), 2);
    file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&data_size), 4);
    file.write(reinterpret_cast<const char*>(pcm.data()), data_size);
}

int main() {
    try {
        ucra::Engine engine; // default options
        ucra::RenderConfig cfg(44100, 1, 512);
        ucra::NoteSegment note(0.0, 2.0, 67, 120, "sol");
        cfg.add_note(std::move(note));
        auto res = engine.render(cfg);
        write_wav_file("cpp_sample_output.wav", res.pcm(), res.sample_rate(), res.channels());
        return 0;
    } catch (...) {
        return 1;
    }
}
