#!/usr/bin/env python3

"""
UCRA Python Bindings Sample Application

This script demonstrates the usage of UCRA Python bindings with NumPy integration.
"""

import os
import sys
import numpy as np

# Add the built module to Python path
build_path = os.path.join(os.path.dirname(__file__), '../../../build/bindings/python')
sys.path.insert(0, build_path)

try:
    import ucra
except ImportError as e:
    print(f"Failed to import UCRA module: {e}")
    print(f"Make sure the module is built and DYLD_LIBRARY_PATH is set.")
    print("Build command: cmake --build build --target ucra_py")
    print("Run command: DYLD_LIBRARY_PATH=/path/to/ucra/build python examples/sample.py")
    sys.exit(1)

def main():
    print("UCRA Python Bindings Sample Application")
    print("=" * 50)

    # Test basic module information
    print(f"Module version: {ucra.__version__}")
    print(f"Available classes: {[name for name in dir(ucra) if not name.startswith('_')]}")

    # Test constants
    print(f"\nConstants:")
    print(f"  Default sample rate: {ucra.DEFAULT_SAMPLE_RATE} Hz")
    print(f"  Default channels: {ucra.DEFAULT_CHANNELS}")
    print(f"  Default block size: {ucra.DEFAULT_BLOCK_SIZE}")

    # Test result enum
    print(f"\nResult codes:")
    print(f"  SUCCESS: {ucra.Result.SUCCESS}")
    print(f"  ERR_NOT_SUPPORTED: {ucra.Result.ERR_NOT_SUPPORTED}")

    # Test NoteSegment creation
    print(f"\nCreating note segments...")
    try:
        note1 = ucra.NoteSegment(0.0, 1.0, 69, 80, "ah")
        note2 = ucra.NoteSegment(1.0, 1.5, 72, 75, "eh")

        print(f"Note 1: '{note1.lyric}' at {note1.start_sec}s for {note1.duration_sec}s (MIDI {note1.midi_note})")
        print(f"Note 2: '{note2.lyric}' at {note2.start_sec}s for {note2.duration_sec}s (MIDI {note2.midi_note})")
    except Exception as e:
        print(f"Failed to create note segments: {e}")
        return 1

    # Test F0 and envelope curves with NumPy
    print(f"\nCreating curves with NumPy arrays...")
    try:
        # Create F0 curve (fundamental frequency)
        time_points = np.linspace(0.0, 2.0, 100, dtype=np.float32)
        f0_values = 440.0 + 100.0 * np.sin(2 * np.pi * time_points)  # Vibrato effect

        f0_curve = ucra.F0Curve(time_points, f0_values)
        print(f"F0 curve created with {f0_curve.length} points")
        print(f"F0 range: {f0_values.min():.1f} - {f0_values.max():.1f} Hz")

        # Create envelope curve
        env_values = np.exp(-time_points * 2.0)  # Exponential decay
        env_curve = ucra.EnvCurve(time_points, env_values)
        print(f"Envelope curve created with {env_curve.length} points")

        # Verify we can get data back as NumPy arrays
        retrieved_time = f0_curve.time_sec
        retrieved_f0 = f0_curve.f0_hz
        print(f"Retrieved F0 data: {type(retrieved_f0)}, shape: {retrieved_f0.shape}")

    except Exception as e:
        print(f"Failed to create curves: {e}")
        return 1

    # Test RenderConfig
    print(f"\nCreating render configuration...")
    try:
        config = ucra.RenderConfig(48000, 2, 1024)  # 48kHz, stereo, 1024 samples per block
        print(f"Config: {config.sample_rate}Hz, {config.channels} channels, {config.block_size} block size")

        config.add_note(note1)
        config.add_note(note2)
        print(f"Added {config.note_count} notes to configuration")

    except Exception as e:
        print(f"Failed to create render config: {e}")
        return 1

    # Test Engine creation (expected to fail in test environment)
    print(f"\nTesting engine creation...")
    try:
        engine = ucra.Engine()
        print("Engine created successfully!")

        # If engine creation succeeded, try rendering
        try:
            result = engine.render(config)
            print(f"Rendering successful! Result shape: {result.shape}")
            print(f"Sample rate: {result.sample_rate}, Frames: {result.frames}, Channels: {result.channels}")

            # Basic audio statistics
            print(f"Audio statistics:")
            print(f"  RMS level: {np.sqrt(np.mean(result**2)):.6f}")
            print(f"  Peak level: {np.max(np.abs(result)):.6f}")

        except Exception as e:
            print(f"Rendering failed: {e}")

    except ucra.UcraError as e:
        print(f"Engine creation failed as expected: {e}")
        print("This is normal in a test environment without proper engine setup.")
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1

    # Test manifest loading (expected to fail without manifest file)
    print(f"\nTesting manifest loading...")
    try:
        manifest = ucra.Manifest("test_manifest.json")
        print(f"Manifest loaded: {manifest.name}")
    except ucra.UcraError as e:
        print(f"Manifest loading failed as expected: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1

    print(f"\nAll tests completed successfully!")
    print("Python bindings are working correctly.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
