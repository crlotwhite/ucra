using System;
using System.IO;
using UCRA;

namespace UCRA.NET.Sample
{
    public static class ProgramEmitWav
    {
        static void WriteWavFloat32(string path, float[] pcm, uint sampleRate, uint channels)
        {
            using var fs = new FileStream(path, FileMode.Create, FileAccess.Write);
            using var bw = new BinaryWriter(fs);
            uint dataSize = (uint)(pcm.Length * sizeof(float));
            uint fileSize = dataSize + 36;
            bw.Write(System.Text.Encoding.ASCII.GetBytes("RIFF"));
            bw.Write(fileSize);
            bw.Write(System.Text.Encoding.ASCII.GetBytes("WAVE"));
            bw.Write(System.Text.Encoding.ASCII.GetBytes("fmt "));
            bw.Write(16u); // fmt size
            bw.Write((ushort)3); // IEEE float
            bw.Write((ushort)channels);
            bw.Write(sampleRate);
            uint byteRate = sampleRate * channels * 4;
            bw.Write(byteRate);
            ushort blockAlign = (ushort)(channels * 4);
            bw.Write(blockAlign);
            bw.Write((ushort)32); // bits per sample
            bw.Write(System.Text.Encoding.ASCII.GetBytes("data"));
            bw.Write(dataSize);
            foreach (var v in pcm) bw.Write(v);
        }

    public static int Main(string[] args)
        {
            try
            {
                var cfg = new RenderConfig { SampleRate = 44100, Channels = 1, BlockSize = 512 };
                using var note = new NoteSegment(0.0, 2.0, 67, 120, "sol");
                cfg.Notes.Add(note);

                using var eng = new Engine();
                var res = eng.Render(cfg);
                if (res.PCM == null || res.PCM.Length == 0)
                {
                    Console.Error.WriteLine("No PCM returned");
                    return 2;
                }
                var outPath = Path.Combine(Directory.GetCurrentDirectory(), "dotnet_sample_output.wav");
                WriteWavFloat32(outPath, res.PCM, res.SampleRate, res.Channels);
                Console.WriteLine($"Wrote {outPath}");
                return 0;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex);
                return 1;
            }
        }
    }
}
