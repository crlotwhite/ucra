/**
 * @file sample_app.cpp
 * @brief Sample application demonstrating UCRA C++ wrapper usage
 */

#include "../ucra_cpp.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>

/**
 * @brief Write PCM data to a simple WAV file
 */
void write_wav_file(const std::string& filename, const std::vector<float>& pcm,
                   uint32_t sample_rate, uint32_t channels) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot create WAV file: " + filename);
    }

    uint32_t data_size = static_cast<uint32_t>(pcm.size() * sizeof(float));
    uint32_t file_size = 36 + data_size;
    uint16_t bits_per_sample = 32;
    uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
    uint16_t block_align = channels * (bits_per_sample / 8);

    // WAV header
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
    std::cout << "UCRA C++ Wrapper Sample Application\n";
    std::cout << "====================================\n\n";

    try {
        // 1. Create engine
        std::cout << "1. Creating UCRA engine...\n";
        std::unordered_map<std::string, std::string> engine_options;

        // Check if sample voicebank is available in a few common locations
        const char* candidates[] = {
            "voicebank/resampler.json",          // current working dir
            "../../voicebank/resampler.json",    // running from bindings/cpp/tests
            "../voicebank/resampler.json",       // running from build/bindings/cpp/tests
            "./build/voicebank/resampler.json",  // running from repo root
        };

        std::string chosen_voicebank;
        for (const char* c : candidates) {
            std::ifstream f(c);
            if (f.good()) {
                // strip trailing filename to get directory
                std::string path(c);
                auto pos = path.rfind('/');
                if (pos != std::string::npos) {
                    chosen_voicebank = path.substr(0, pos);
                }
                break;
            }
        }

        if (!chosen_voicebank.empty()) {
            std::cout << "   Found sample voicebank at '" << chosen_voicebank << "', using it...\n";
            engine_options["voicebank_path"] = chosen_voicebank;
        } else {
            std::cout << "   No voicebank found, trying sample mode...\n";
            // Try without any options first; if engine rejects, test will report clearly
        }

        ucra::Engine engine(engine_options);

        std::string engine_info = engine.get_info();
        std::cout << "   Engine info: " << engine_info << "\n\n";

        // 2. Create a simple melody
        std::cout << "2. Setting up melody (C major scale)...\n";
        ucra::RenderConfig config(44100, 1, 512);

        // C major scale notes
        std::vector<int16_t> notes = {60, 62, 64, 65, 67, 69, 71, 72}; // C4 to C5
        std::vector<std::string> lyrics = {"do", "re", "mi", "fa", "sol", "la", "ti", "do"};

        double note_duration = 0.5; // 0.5 seconds per note
        for (size_t i = 0; i < notes.size(); ++i) {
            ucra::NoteSegment note(
                i * note_duration,      // start time
                note_duration,          // duration
                notes[i],              // MIDI note
                80,                    // velocity
                lyrics[i]              // lyric
            );

            // Add vibrato to some notes using F0 curve
            if (i % 2 == 1) {
                std::vector<float> time_points;
                std::vector<float> f0_points;

                float base_f0 = 440.0f * std::pow(2.0f, (notes[i] - 69) / 12.0f);
                int num_points = 20;

                for (int j = 0; j < num_points; ++j) {
                    float t = j * note_duration / (num_points - 1);
                    float vibrato = base_f0 * (1.0f + 0.02f * std::sin(2.0f * M_PI * 5.0f * t));
                    time_points.push_back(static_cast<float>(i * note_duration + t));
                    f0_points.push_back(vibrato);
                }

                ucra::F0Curve f0_curve(std::move(time_points), std::move(f0_points));
                note.set_f0_override(std::move(f0_curve));
            }

            config.add_note(std::move(note));
        }

        std::cout << "   Added " << config.notes().size() << " notes\n";
        std::cout << "   Total duration: " << (notes.size() * note_duration) << " seconds\n\n";

        // 3. Render audio
        std::cout << "3. Rendering audio...\n";
        ucra::RenderResult result = engine.render(config);

        std::cout << "   Rendered " << result.frames() << " frames\n";
        std::cout << "   Sample rate: " << result.sample_rate() << " Hz\n";
        std::cout << "   Channels: " << result.channels() << "\n";
        std::cout << "   Duration: " << (static_cast<double>(result.frames()) / result.sample_rate()) << " seconds\n";

        // Display metadata if any
        if (!result.metadata().empty()) {
            std::cout << "   Metadata:\n";
            for (const auto& [key, value] : result.metadata()) {
                std::cout << "     " << key << ": " << value << "\n";
            }
        }
        std::cout << "\n";

        // 4. Save to WAV file
        std::cout << "4. Saving to WAV file...\n";
        if (!result.pcm().empty()) {
            write_wav_file("cpp_sample_output.wav", result.pcm(),
                          result.sample_rate(), result.channels());
            std::cout << "   Saved to: cpp_sample_output.wav\n";
        } else {
            std::cout << "   No PCM data to save\n";
        }

        std::cout << "\n✅ Sample application completed successfully!\n";

    } catch (const ucra::UcraException& e) {
        std::cerr << "\n❌ UCRA error: " << e.what() << "\n";
        std::cerr << "Error code: " << e.error_code() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
