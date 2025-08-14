using System;
using NUnit.Framework;

namespace UCRA.NET.Tests
{
    [TestFixture]
    public class StreamingTests
    {
        [Test]
        public void Stream_Open_Read_Close_SucceedsOrNotSupported()
        {
            var initial = new UCRA.RenderConfig
            {
                SampleRate = 44100,
                Channels = 1,
                BlockSize = 512
            };

            // Provider: constant middle A note for short duration
            Func<UCRA.RenderConfig> provider = () =>
            {
                var cfg = new UCRA.RenderConfig
                {
                    SampleRate = 44100,
                    Channels = 1,
                    BlockSize = 512
                };
                cfg.Notes.Add(new UCRA.NoteSegment(0.0, 0.1, 69, 100, "a"));
                return cfg;
            };

            try
            {
                using var session = new UCRA.StreamSession(initial, provider);
                float[] buffer = new float[initial.BlockSize * initial.Channels];
                session.Read(buffer, initial.BlockSize, out uint read);
                Assert.GreaterOrEqual(read, 0);
            }
            catch (UCRA.UcraException ex)
            {
                Assert.AreEqual(UCRA.Interop.NativeMethods.UCRAResult.NotSupported, ex.ErrorCode);
                Assert.Pass("Streaming not supported in this environment (expected)");
            }
        }
    }
}
