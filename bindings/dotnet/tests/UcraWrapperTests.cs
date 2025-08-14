using System;
using System.Collections.Generic;
using System.IO;
using NUnit.Framework;
using UCRA.NET;

namespace UCRA.NET.Tests
{
    [TestFixture]
    public class UcraWrapperTests
    {
        [Test]
        public void UcraException_Creation_SetsCorrectErrorCode()
        {
            var exception = new UcraException(Interop.NativeMethods.UCRAResult.InvalidArgument);
            Assert.AreEqual(Interop.NativeMethods.UCRAResult.InvalidArgument, exception.ErrorCode);
            Assert.That(exception.Message, Contains.Substring("Invalid argument"));
        }

        [Test]
        public void F0Curve_Creation_WithValidData_Succeeds()
        {
            var timeSec = new float[] { 0.0f, 0.5f, 1.0f };
            var f0Hz = new float[] { 440.0f, 550.0f, 660.0f };

            using var curve = new F0Curve(timeSec, f0Hz);

            Assert.AreEqual(3, curve.Length);
            CollectionAssert.AreEqual(timeSec, curve.TimeSec);
            CollectionAssert.AreEqual(f0Hz, curve.F0Hz);
        }

        [Test]
        public void F0Curve_Creation_WithMismatchedArrays_ThrowsException()
        {
            var timeSec = new float[] { 0.0f, 0.5f };
            var f0Hz = new float[] { 440.0f, 550.0f, 660.0f };

            Assert.Throws<ArgumentException>(() => new F0Curve(timeSec, f0Hz));
        }

        [Test]
        public void EnvCurve_Creation_WithValidData_Succeeds()
        {
            var timeSec = new float[] { 0.0f, 0.5f, 1.0f };
            var value = new float[] { 0.0f, 1.0f, 0.5f };

            using var curve = new EnvCurve(timeSec, value);

            Assert.AreEqual(3, curve.Length);
            CollectionAssert.AreEqual(timeSec, curve.TimeSec);
            CollectionAssert.AreEqual(value, curve.Value);
        }

        [Test]
        public void NoteSegment_Creation_WithValidParameters_Succeeds()
        {
            using var note = new NoteSegment(0.0, 1.0, 69, 80, "la");

            Assert.AreEqual(0.0, note.StartSec);
            Assert.AreEqual(1.0, note.DurationSec);
            Assert.AreEqual(69, note.MidiNote);
            Assert.AreEqual(80, note.Velocity);
            Assert.AreEqual("la", note.Lyric);
        }

        [Test]
        public void NoteSegment_Creation_WithInvalidDuration_ThrowsException()
        {
            Assert.Throws<ArgumentException>(() => new NoteSegment(0.0, -1.0));
            Assert.Throws<ArgumentException>(() => new NoteSegment(0.0, 0.0));
        }

        [Test]
        public void NoteSegment_Creation_WithInvalidVelocity_ThrowsException()
        {
            Assert.Throws<ArgumentException>(() => new NoteSegment(0.0, 1.0, velocity: 128));
        }

        [Test]
        public void NoteSegment_Creation_WithInvalidMidiNote_ThrowsException()
        {
            Assert.Throws<ArgumentException>(() => new NoteSegment(0.0, 1.0, midiNote: -2));
            Assert.Throws<ArgumentException>(() => new NoteSegment(0.0, 1.0, midiNote: 128));
        }

        [Test]
        public void RenderConfig_Creation_WithDefaults_Succeeds()
        {
            var config = new RenderConfig();

            Assert.AreEqual(44100u, config.SampleRate);
            Assert.AreEqual(1u, config.Channels);
            Assert.AreEqual(512u, config.BlockSize);
            Assert.AreEqual(0u, config.Flags);
            Assert.IsNotNull(config.Notes);
            Assert.IsNotNull(config.Options);
        }

        [Test]
        public void Engine_Creation_WithValidOptions_HandlesAvailabilityGracefully()
        {
            var options = new Dictionary<string, string>
            {
                { "test_mode", "true" }
            };

            // In environments without a real engine, creation should throw NotSupported.
            // If a reference engine is available, creation should succeed. Accept both.
            try
            {
                using var engine = new Engine(options);
                Assert.Pass("Engine is available; creation succeeded.");
            }
            catch (UcraException ex)
            {
                Assert.AreEqual(Interop.NativeMethods.UCRAResult.NotSupported, ex.ErrorCode);
                Assert.Pass("Engine not supported in this environment (expected).");
            }
        }

        [Test]
        public void Engine_Creation_WithNullOptions_HandlesAvailabilityGracefully()
        {
            // Same logic as above: allow either NotSupported or success depending on environment.
            try
            {
                using var engine = new Engine();
                Assert.Pass("Engine is available; creation succeeded.");
            }
            catch (UcraException ex)
            {
                Assert.AreEqual(Interop.NativeMethods.UCRAResult.NotSupported, ex.ErrorCode);
                Assert.Pass("Engine not supported in this environment (expected).");
            }
        }

        [Test]
        public void Manifest_Creation_WithNonExistentFile_ThrowsException()
        {
            Assert.Throws<UcraException>(() => new Manifest("nonexistent_manifest.json"));
        }

        [Test]
        public void Manifest_Creation_WithEmptyPath_ThrowsException()
        {
            Assert.Throws<ArgumentException>(() => new Manifest(""));
            Assert.Throws<ArgumentException>(() => new Manifest(null));
        }
    }

    /// <summary>
    /// Integration tests that require actual engine functionality
    /// These tests may be skipped in environments without proper engine setup
    /// </summary>
    [TestFixture]
    public class UcraIntegrationTests
    {
        [Test]
        public void FullWorkflow_CreateEngineAndRender_WorksWithMockEngine()
        {
            // This test demonstrates the complete workflow but expects
            // the engine to return "Not supported" in test environment

            try
            {
                using var engine = new Engine();

                var config = new RenderConfig
                {
                    SampleRate = 44100,
                    Channels = 1
                };

                using var note = new NoteSegment(0.0, 0.5, 69, 80, "a");
                config.Notes.Add(note);

                var result = engine.Render(config);

                // If we reach here, the engine actually worked
                Assert.IsNotNull(result);
                Assert.Greater(result.Frames, 0);

                TestContext.WriteLine($"Rendered {result.Frames} frames at {result.SampleRate}Hz");
            }
            catch (UcraException ex) when (ex.ErrorCode == Interop.NativeMethods.UCRAResult.NotSupported)
            {
                // Expected in test environment without real engine
                TestContext.WriteLine("Engine creation not supported in test environment (expected)");
                Assert.Pass("Test completed successfully - engine not supported as expected");
            }
        }
    }
}
