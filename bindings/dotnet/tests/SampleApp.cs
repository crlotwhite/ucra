using System;
using System.Collections.Generic;
using System.IO;
using UCRA;

namespace UCRA.Sample
{
    class Program
    {
        static int Main(string[] args)
        {
            Console.WriteLine("UCRA .NET Wrapper Sample Application");
            Console.WriteLine("=====================================\n");

            try
            {
                // 1. Test basic wrapper functionality
                Console.WriteLine("1. Testing basic wrapper functionality...");
                TestWrapperClasses();
                Console.WriteLine("✓ Wrapper classes work correctly\n");

                // 2. Try to create an engine
                Console.WriteLine("2. Attempting to create UCRA engine...");
                TestEngineCreation();

                Console.WriteLine("\n✅ Sample application completed successfully!");
                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"\n❌ Error: {ex.Message}");
                if (ex is UcraException ucraEx)
                {
                    Console.WriteLine($"UCRA Error Code: {ucraEx.ErrorCode}");
                }
                return 1;
            }
        }

        static void TestWrapperClasses()
        {
            // Test F0Curve
            Console.WriteLine("   Testing F0Curve...");
            using (var f0Curve = new F0Curve(
                new float[] { 0.0f, 0.5f, 1.0f },
                new float[] { 440.0f, 550.0f, 660.0f }))
            {
                Console.WriteLine($"   - Created F0 curve with {f0Curve.Length} points");
            }

            // Test EnvCurve
            Console.WriteLine("   Testing EnvCurve...");
            using (var envCurve = new EnvCurve(
                new float[] { 0.0f, 0.5f, 1.0f },
                new float[] { 0.0f, 1.0f, 0.5f }))
            {
                Console.WriteLine($"   - Created envelope curve with {envCurve.Length} points");
            }

            // Test NoteSegment
            Console.WriteLine("   Testing NoteSegment...");
            using (var note = new NoteSegment(0.0, 1.0, 69, 80, "la"))
            {
                Console.WriteLine($"   - Created note: {note.Lyric}, MIDI {note.MidiNote}, {note.DurationSec}s");
            }

            // Test RenderConfig
            Console.WriteLine("   Testing RenderConfig...");
            var config = new RenderConfig
            {
                SampleRate = 44100,
                Channels = 2,
                BlockSize = 512
            };

            config.Options["engine"] = "world";
            config.Options["quality"] = "high";

            using (var note1 = new NoteSegment(0.0, 0.5, 60, 80, "do"))
            using (var note2 = new NoteSegment(0.5, 0.5, 62, 80, "re"))
            {
                config.Notes.Add(note1);
                config.Notes.Add(note2);

                Console.WriteLine($"   - Created config: {config.SampleRate}Hz, {config.Channels}ch, {config.Notes.Count} notes");
            }
        }

        static void TestEngineCreation()
        {
            try
            {
                var options = new Dictionary<string, string>
                {
                    { "sample_mode", "true" }
                };

                using (var engine = new Engine(options))
                {
                    Console.WriteLine("✓ Engine created successfully");

                    var info = engine.GetInfo();
                    Console.WriteLine($"   Engine info: {info}");

                    // Try to render audio
                    Console.WriteLine("   Attempting to render audio...");
                    var config = new RenderConfig
                    {
                        SampleRate = 44100,
                        Channels = 1
                    };

                    using (var note = new NoteSegment(0.0, 0.1, 69, 80, "a"))
                    {
                        config.Notes.Add(note);

                        var result = engine.Render(config);
                        Console.WriteLine($"✓ Rendered {result.Frames} frames at {result.SampleRate}Hz");

                        if (result.PCM != null && result.PCM.Length > 0)
                        {
                            Console.WriteLine($"   PCM data length: {result.PCM.Length} samples");

                            // Save to file if possible
                            SaveWavFile("dotnet_sample_output.wav", result);
                            Console.WriteLine("   Saved to: dotnet_sample_output.wav");
                        }
                    }
                }
            }
            catch (UcraException ex) when (ex.ErrorCode == Interop.NativeMethods.UCRAResult.NotSupported)
            {
                Console.WriteLine("⚠ Engine creation not supported (expected in test environment)");
                Console.WriteLine("   This is normal if no UCRA engine implementation is available");
            }
        }

        static void SaveWavFile(string filename, RenderResult result)
        {
            if (result.PCM == null || result.PCM.Length == 0)
                return;

            using var file = new FileStream(filename, FileMode.Create);
            using var writer = new BinaryWriter(file);

            // Simple WAV header for IEEE float format
            uint dataSize = (uint)(result.PCM.Length * sizeof(float));
            uint fileSize = 36 + dataSize;
            ushort bitsPerSample = 32;
            uint byteRate = result.SampleRate * result.Channels * ((uint)bitsPerSample / 8);
            ushort blockAlign = (ushort)(result.Channels * ((uint)bitsPerSample / 8));

            // RIFF header
            writer.Write(System.Text.Encoding.ASCII.GetBytes("RIFF"));
            writer.Write(fileSize);
            writer.Write(System.Text.Encoding.ASCII.GetBytes("WAVE"));

            // fmt chunk
            writer.Write(System.Text.Encoding.ASCII.GetBytes("fmt "));
            writer.Write(16u); // chunk size
            writer.Write((ushort)3); // IEEE float format
            writer.Write((ushort)result.Channels);
            writer.Write(result.SampleRate);
            writer.Write(byteRate);
            writer.Write(blockAlign);
            writer.Write(bitsPerSample);

            // data chunk
            writer.Write(System.Text.Encoding.ASCII.GetBytes("data"));
            writer.Write(dataSize);

            // Write PCM data
            foreach (var sample in result.PCM)
            {
                writer.Write(sample);
            }
        }
    }
}
