#!/usr/bin/env python3

import pytest
import numpy as np
import sys
import os

# Try to import the module
try:
    import ucra
except ImportError:
    # Add build directory to path if not installed
    build_dirs = [
        os.path.join(os.path.dirname(__file__), "..", "build"),
        os.path.join(os.path.dirname(__file__), "..", "build", "lib"),
    ]

    for build_dir in build_dirs:
        if os.path.exists(build_dir):
            sys.path.insert(0, build_dir)
            break

    import ucra


class TestUcraTypes:
    """Test basic types and constants."""

    def test_constants(self):
        """Test that module constants are defined."""
        assert hasattr(ucra, 'DEFAULT_SAMPLE_RATE')
        assert hasattr(ucra, 'DEFAULT_CHANNELS')
        assert hasattr(ucra, 'DEFAULT_BLOCK_SIZE')

        assert ucra.DEFAULT_SAMPLE_RATE == 44100
        assert ucra.DEFAULT_CHANNELS == 1
        assert ucra.DEFAULT_BLOCK_SIZE == 512

    def test_result_enum(self):
        """Test that Result enum is available."""
        assert hasattr(ucra, 'Result')
        assert hasattr(ucra.Result, 'SUCCESS')
        assert hasattr(ucra.Result, 'ERR_INVALID_ARGUMENT')
        assert hasattr(ucra.Result, 'ERR_NOT_SUPPORTED')

    def test_version(self):
        """Test version information."""
        # Version function not implemented in current UCRA
        pytest.skip("Version function not implemented in current UCRA version")


class TestNoteSegment:
    """Test NoteSegment functionality."""

    def test_note_creation(self):
        """Test basic note segment creation."""
        note = ucra.NoteSegment(0.0, 1.0, 69, 80, "ah")

        assert note.start_sec == 0.0
        assert note.duration_sec == 1.0
        assert note.midi_note == 69
        assert note.velocity == 80
        assert note.lyric == "ah"

    def test_note_defaults(self):
        """Test note creation with default parameters."""
        note = ucra.NoteSegment(0.5, 2.0)

        assert note.start_sec == 0.5
        assert note.duration_sec == 2.0
        assert note.midi_note == 69  # Default
        assert note.velocity == 80   # Default
        assert note.lyric == ""      # Default

    def test_invalid_duration(self):
        """Test that invalid durations raise exceptions."""
        with pytest.raises(ValueError):
            ucra.NoteSegment(0.0, 0.0)  # Zero duration

        with pytest.raises(ValueError):
            ucra.NoteSegment(0.0, -1.0)  # Negative duration

    def test_invalid_velocity(self):
        """Test that invalid velocities raise exceptions."""
        with pytest.raises(ValueError):
            ucra.NoteSegment(0.0, 1.0, 69, 128)  # Velocity too high

        with pytest.raises(ValueError):
            ucra.NoteSegment(0.0, 1.0, 69, -1)   # Velocity too low

    def test_invalid_midi_note(self):
        """Test that invalid MIDI notes raise exceptions."""
        with pytest.raises(ValueError):
            ucra.NoteSegment(0.0, 1.0, 128)  # MIDI note too high

        with pytest.raises(ValueError):
            ucra.NoteSegment(0.0, 1.0, -2)   # MIDI note too low


class TestCurves:
    """Test F0 and envelope curve functionality."""

    def test_f0_curve_creation(self):
        """Test F0 curve creation with NumPy arrays."""
        time = np.array([0.0, 0.5, 1.0], dtype=np.float32)
        f0 = np.array([440.0, 550.0, 660.0], dtype=np.float32)

        curve = ucra.F0Curve(time, f0)

        assert curve.length == 3
        np.testing.assert_array_equal(curve.time_sec, time)
        np.testing.assert_array_equal(curve.f0_hz, f0)

    def test_env_curve_creation(self):
        """Test envelope curve creation with NumPy arrays."""
        time = np.array([0.0, 0.5, 1.0], dtype=np.float32)
        value = np.array([0.0, 1.0, 0.5], dtype=np.float32)

        curve = ucra.EnvCurve(time, value)

        assert curve.length == 3
        np.testing.assert_array_equal(curve.time_sec, time)
        np.testing.assert_array_equal(curve.value, value)

    def test_curve_mismatched_arrays(self):
        """Test that mismatched array sizes raise exceptions."""
        time = np.array([0.0, 0.5], dtype=np.float32)
        f0 = np.array([440.0, 550.0, 660.0], dtype=np.float32)

        with pytest.raises(ValueError):
            ucra.F0Curve(time, f0)

    def test_curve_empty_arrays(self):
        """Test that empty arrays raise exceptions."""
        time = np.array([], dtype=np.float32)
        f0 = np.array([], dtype=np.float32)

        with pytest.raises(ValueError):
            ucra.F0Curve(time, f0)

    def test_curve_multidimensional_arrays(self):
        """Test that multidimensional arrays raise exceptions."""
        time = np.array([[0.0, 0.5]], dtype=np.float32)
        f0 = np.array([[440.0, 550.0]], dtype=np.float32)

        with pytest.raises(ValueError):
            ucra.F0Curve(time, f0)


class TestRenderConfig:
    """Test RenderConfig functionality."""

    def test_config_creation(self):
        """Test render config creation."""
        config = ucra.RenderConfig(48000, 2, 1024, 0)

        assert config.sample_rate == 48000
        assert config.channels == 2
        assert config.block_size == 1024
        assert config.flags == 0
        assert config.note_count == 0

    def test_config_defaults(self):
        """Test render config with default parameters."""
        config = ucra.RenderConfig()

        assert config.sample_rate == 44100
        assert config.channels == 1
        assert config.block_size == 512
        assert config.flags == 0

    def test_add_note(self):
        """Test adding notes to config."""
        config = ucra.RenderConfig()
        note = ucra.NoteSegment(0.0, 1.0, 69, 80, "test")

        config.add_note(note)
        assert config.note_count == 1

    def test_multiple_notes(self):
        """Test adding multiple notes."""
        config = ucra.RenderConfig()

        for i in range(3):
            note = ucra.NoteSegment(float(i), 1.0, 60 + i, 80, f"note{i}")
            config.add_note(note)

        assert config.note_count == 3


class TestEngine:
    """Test Engine functionality."""

    def test_engine_creation_without_options(self):
        """Test engine creation without options (expected to fail)."""
        try:
            engine = ucra.Engine()
            # If this succeeds, the engine is actually available
            assert engine is not None
        except ucra.UcraError as e:
            # Expected in test environment
            assert "Not supported" in str(e) or "creation failed" in str(e)

    def test_engine_creation_with_options(self):
        """Test engine creation with options (expected to fail)."""
        options = {"test_mode": "true"}

        try:
            engine = ucra.Engine(options)
            assert engine is not None
        except ucra.UcraError as e:
            # Expected in test environment
            assert "Not supported" in str(e) or "creation failed" in str(e)

    def test_engine_render(self):
        """Test engine rendering (expected to fail)."""
        try:
            engine = ucra.Engine()
            config = ucra.RenderConfig()
            note = ucra.NoteSegment(0.0, 1.0, 69, 80, "test")
            config.add_note(note)

            result = engine.render(config)

            # If we get here, rendering actually worked
            assert isinstance(result, np.ndarray)
            assert result.ndim == 2  # [frames, channels]
            assert hasattr(result, 'sample_rate')
            assert hasattr(result, 'frames')
            assert hasattr(result, 'channels')

        except ucra.UcraError as e:
            # Expected in test environment
            pytest.skip(f"Engine not available for rendering test: {e}")


class TestManifest:
    """Test Manifest functionality."""

    def test_manifest_creation_nonexistent_file(self):
        """Test that loading non-existent manifest raises exception."""
        with pytest.raises(ucra.UcraError):
            ucra.Manifest("nonexistent_manifest.json")

    def test_manifest_creation_empty_path(self):
        """Test that empty path raises exception."""
        with pytest.raises(ValueError):
            ucra.Manifest("")


if __name__ == "__main__":
    pytest.main([__file__])
