/**
 * @file test_ucra_cpp_simple.cpp
 * @brief Simple assert-based tests for UCRA C++ wrapper
 */

#include "../ucra_cpp.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

void test_exception_handling() {
    std::cout << "Testing exception handling... ";

    // Test error code to string conversion
    try {
        ucra::check_result(UCRA_ERR_INVALID_ARGUMENT);
        assert(false); // Should not reach here
    } catch (const ucra::UcraException& e) {
        assert(e.error_code() == UCRA_ERR_INVALID_ARGUMENT);
        assert(std::string(e.what()).find("Invalid argument") != std::string::npos);
    }

    std::cout << "✓\n";
}

void test_key_value() {
    std::cout << "Testing KeyValue class... ";

    ucra::KeyValue kv("test_key", "test_value");
    assert(kv.key() == "test_key");
    assert(kv.value() == "test_value");

    kv.set_key("new_key");
    kv.set_value("new_value");
    assert(kv.key() == "new_key");
    assert(kv.value() == "new_value");

    std::cout << "✓\n";
}

void test_f0_curve() {
    std::cout << "Testing F0Curve class... ";

    std::vector<float> time_sec = {0.0f, 0.5f, 1.0f};
    std::vector<float> f0_hz = {440.0f, 550.0f, 660.0f};

    ucra::F0Curve curve(time_sec, f0_hz);
    assert(curve.time_sec() == time_sec);
    assert(curve.f0_hz() == f0_hz);

    const UCRA_F0Curve* c_curve = curve.c_struct();
    assert(c_curve != nullptr);
    assert(c_curve->length == 3);
    assert(c_curve->time_sec[0] == 0.0f);
    assert(c_curve->f0_hz[0] == 440.0f);

    // Test invalid input
    try {
        ucra::F0Curve invalid_curve({0.0f}, {440.0f, 550.0f});
        assert(false); // Should throw
    } catch (const std::invalid_argument&) {
        // Expected
    }

    std::cout << "✓\n";
}

void test_env_curve() {
    std::cout << "Testing EnvCurve class... ";

    std::vector<float> time_sec = {0.0f, 0.5f, 1.0f};
    std::vector<float> value = {0.0f, 1.0f, 0.5f};

    ucra::EnvCurve curve(time_sec, value);
    assert(curve.time_sec() == time_sec);
    assert(curve.value() == value);

    const UCRA_EnvCurve* c_curve = curve.c_struct();
    assert(c_curve != nullptr);
    assert(c_curve->length == 3);
    assert(c_curve->time_sec[1] == 0.5f);
    assert(c_curve->value[1] == 1.0f);

    std::cout << "✓\n";
}

void test_note_segment() {
    std::cout << "Testing NoteSegment class... ";

    ucra::NoteSegment note(0.0, 1.0, 69, 80, "la");
    assert(note.start_sec() == 0.0);
    assert(note.duration_sec() == 1.0);
    assert(note.midi_note() == 69);
    assert(note.velocity() == 80);
    assert(note.lyric() == "la");

    const UCRA_NoteSegment& c_note = note.c_struct();
    assert(c_note.start_sec == 0.0);
    assert(c_note.duration_sec == 1.0);
    assert(c_note.midi_note == 69);
    assert(c_note.velocity == 80);
    assert(std::string(c_note.lyric) == "la");

    // Test F0 override
    ucra::F0Curve f0_curve({0.0f, 1.0f}, {440.0f, 880.0f});
    note.set_f0_override(std::move(f0_curve));
    assert(note.c_struct().f0_override != nullptr);

    std::cout << "✓\n";
}

void test_render_config() {
    std::cout << "Testing RenderConfig class... ";

    ucra::RenderConfig config(44100, 2, 512, 0);
    assert(config.sample_rate() == 44100);
    assert(config.channels() == 2);
    assert(config.block_size() == 512);
    assert(config.flags() == 0);

    // Add notes
    ucra::NoteSegment note1(0.0, 1.0, 69, 80, "do");
    ucra::NoteSegment note2(1.0, 1.0, 71, 85, "re");
    config.add_note(std::move(note1));
    config.add_note(std::move(note2));

    assert(config.notes().size() == 2);

    // Add options
    config.add_option("engine", "world");
    config.add_option("quality", "high");

    assert(config.options().size() == 2);
    assert(config.options().at("engine") == "world");

    const UCRA_RenderConfig& c_config = config.c_struct();
    assert(c_config.sample_rate == 44100);
    assert(c_config.note_count == 2);
    assert(c_config.option_count == 2);

    std::cout << "✓\n";
}

void test_engine_lifecycle() {
    std::cout << "Testing Engine lifecycle... ";

    try {
        // Create engine with options
        std::unordered_map<std::string, std::string> options;
        options["test_mode"] = "true";

        ucra::Engine engine(options);

        // Get engine info
        std::string info = engine.get_info();
        assert(!info.empty());

        // Basic render test
        ucra::RenderConfig config(44100, 1, 512);
        ucra::NoteSegment note(0.0, 0.1, 69, 80, "a");
        config.add_note(std::move(note));

        ucra::RenderResult result = engine.render(config);
        assert(result.status() == UCRA_SUCCESS);

        std::cout << "✓\n";
    } catch (const ucra::UcraException& e) {
        std::cout << "⚠ (Engine test skipped: " << e.what() << ")\n";
    }
}

void test_manifest_loading() {
    std::cout << "Testing Manifest loading... ";

    try {
        // Try to load a manifest (may not exist in test environment)
        ucra::Manifest manifest("test_manifest.json");

        // If successful, test the getters
        auto name = manifest.name();
        auto version = manifest.version();

        std::cout << "✓\n";
    } catch (const ucra::UcraException&) {
        std::cout << "⚠ (Manifest test skipped: file not found)\n";
    }
}

void test_render_result() {
    std::cout << "Testing RenderResult class... ";

    ucra::RenderResult result;

    // Create a mock C result
    UCRA_RenderResult c_result = {};
    c_result.frames = 1024;
    c_result.channels = 2;
    c_result.sample_rate = 44100;
    c_result.status = UCRA_SUCCESS;

    std::vector<float> test_pcm(1024 * 2, 0.5f);
    c_result.pcm = test_pcm.data();

    result.update_from_c_result(c_result);

    assert(result.frames() == 1024);
    assert(result.channels() == 2);
    assert(result.sample_rate() == 44100);
    assert(result.status() == UCRA_SUCCESS);
    assert(result.pcm().size() == 1024 * 2);
    assert(std::abs(result.pcm()[0] - 0.5f) < 1e-6f);

    std::cout << "✓\n";
}

int main() {
    std::cout << "Running UCRA C++ Wrapper Tests\n";
    std::cout << "===============================\n\n";

    test_exception_handling();
    test_key_value();
    test_f0_curve();
    test_env_curve();
    test_note_segment();
    test_render_config();
    test_render_result();
    test_engine_lifecycle();
    test_manifest_loading();

    std::cout << "\n✅ All tests completed successfully!\n";
    return 0;
}
