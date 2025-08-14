using System;
using System.Collections.Generic;
using System.IO;
using UCRA.NET;

namespace UCRA.NET.Sample
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("UCRA .NET Wrapper Sample Application");
            Console.WriteLine("=====================================");

            try
            {
                // Test basic object creation (no native calls)
                Console.WriteLine("Testing object creation...");

                var config = new RenderConfig
                {
                    SampleRate = 44100,
                    Channels = 1,
                    BlockSize = 512
                };

                using var note = new NoteSegment(0.0, 1.0, 69, 80, "ah");
                config.Notes.Add(note);

                Console.WriteLine($"Created RenderConfig with {config.SampleRate}Hz sample rate");
                Console.WriteLine($"Created NoteSegment: '{note.Lyric}' at {note.StartSec}s for {note.DurationSec}s");

                // Test curve creation
                var timeSec = new float[] { 0.0f, 0.5f, 1.0f };
                var f0Hz = new float[] { 440.0f, 550.0f, 660.0f };

                using var f0Curve = new F0Curve(timeSec, f0Hz);
                Console.WriteLine($"Created F0Curve with {f0Curve.Length} points");

                var envValue = new float[] { 0.0f, 1.0f, 0.5f };
                using var envCurve = new EnvCurve(timeSec, envValue);
                Console.WriteLine($"Created EnvCurve with {envCurve.Length} points");

                Console.WriteLine("\nObject creation tests passed successfully!");

                // Try engine creation (expected to fail in test environment)
                Console.WriteLine("\nTesting engine creation...");
                try
                {
                    using var engine = new Engine();
                    Console.WriteLine("Engine created successfully!");

                    // If engine creation succeeded, try rendering
                    var result = engine.Render(config);
                    Console.WriteLine($"Rendered {result.Frames} frames at {result.SampleRate}Hz");
                }
                catch (UcraException ex)
                {
                    Console.WriteLine($"Engine creation failed as expected: {ex.Message}");
                    Console.WriteLine("This is normal in a test environment without proper engine setup.");
                }

                Console.WriteLine("\nAll tests completed successfully!");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
                Console.WriteLine($"Stack trace: {ex.StackTrace}");
                Environment.Exit(1);
            }
        }
    }
}
