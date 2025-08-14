using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UCRA.Interop;

namespace UCRA
{
    /// <summary>
    /// High-level streaming session wrapper for the UCRA Streaming API.
    /// </summary>
    public sealed class StreamSession : IDisposable
    {
        private IntPtr _handle;
        private bool _disposed;
        private readonly Func<RenderConfig> _provider;
        private readonly object _allocLock = new object();
        private readonly List<IntPtr> _pendingAllocations = new List<IntPtr>();
        private NativeMethods.PullPCMCallback _nativeCallback; // Keep delegate alive

        /// <summary>
        /// Create and open a streaming session.
        /// </summary>
        /// <param name="initialConfig">Initial stream configuration (sample rate, channels, block size).</param>
        /// <param name="provider">Callback that supplies a RenderConfig for each pull.</param>
        public StreamSession(RenderConfig initialConfig, Func<RenderConfig> provider)
        {
            _provider = provider ?? throw new ArgumentNullException(nameof(provider));
            if (initialConfig == null) throw new ArgumentNullException(nameof(initialConfig));

            _nativeCallback = OnPullPcm; // capture delegate

            // Build minimal native config from initialConfig
            var nativeCfg = new NativeMethods.RenderConfig
            {
                SampleRate = initialConfig.SampleRate,
                Channels = initialConfig.Channels,
                BlockSize = initialConfig.BlockSize,
                Flags = initialConfig.Flags,
                Notes = IntPtr.Zero,
                NoteCount = 0,
                Options = IntPtr.Zero,
                OptionCount = 0
            };

            var result = NativeMethods.ucra_stream_open(out _handle, ref nativeCfg, _nativeCallback, IntPtr.Zero);
            try
            {
                ErrorHelper.CheckResult(result, "Failed to open UCRA stream");
            }
            finally
            {
                // Free any unmanaged memory allocated during ucra_stream_open callback prefill
                FreePendingAllocations();
            }
        }

        /// <summary>
        /// Read PCM frames into the supplied buffer.
        /// </summary>
        /// <param name="buffer">Destination buffer (length must be >= frames * channels).</param>
        /// <param name="frames">Requested frames to read.</param>
        /// <param name="framesRead">Actual frames read.</param>
        public void Read(float[] buffer, uint frames, out uint framesRead)
        {
            if (buffer == null) throw new ArgumentNullException(nameof(buffer));
            ThrowIfDisposed();

            // Allocate unmanaged temp buffer (channels unknown until pull, but safe upper bound using buffer length)
            int samples = buffer.Length;
            int bytes = samples * sizeof(float);
            IntPtr unmanaged = Marshal.AllocHGlobal(bytes);
            try
            {
                var result = NativeMethods.ucra_stream_read(_handle, unmanaged, frames, out framesRead);
                ErrorHelper.CheckResult(result, "Failed to read from UCRA stream");

                // Determine channels best-effort: assume caller sized buffer properly
                int channels = samples / (int)frames;
                if (channels <= 0) channels = 1;
                int samplesToCopy = (int)framesRead * channels;
                if (samplesToCopy > buffer.Length) samplesToCopy = buffer.Length;

                Marshal.Copy(unmanaged, buffer, 0, samplesToCopy);
            }
            finally
            {
                Marshal.FreeHGlobal(unmanaged);
                // Free any unmanaged memory allocated during the callback
                FreePendingAllocations();
            }
        }

        /// <summary>
        /// Close the stream and release resources.
        /// </summary>
        public void Dispose()
        {
            if (_disposed) return;
            if (_handle != IntPtr.Zero)
            {
                NativeMethods.ucra_stream_close(_handle);
                _handle = IntPtr.Zero;
            }
            FreePendingAllocations();
            _disposed = true;
        }

        private NativeMethods.UCRAResult OnPullPcm(IntPtr userData, out NativeMethods.RenderConfig outConfig)
        {
            // Build native config from managed RenderConfig provided by caller
            var cfg = _provider();
            if (cfg == null)
            {
                outConfig = default;
                return NativeMethods.UCRAResult.InvalidArgument;
            }

            outConfig = new NativeMethods.RenderConfig
            {
                SampleRate = cfg.SampleRate,
                Channels = cfg.Channels,
                BlockSize = cfg.BlockSize,
                Flags = cfg.Flags,
                Notes = IntPtr.Zero,
                NoteCount = 0,
                Options = IntPtr.Zero,
                OptionCount = 0
            };

            // Notes (allocate unmanaged copy)
            if (cfg.Notes.Count > 0)
            {
                int sz = Marshal.SizeOf<NativeMethods.NoteSegment>();
                IntPtr notesPtr = Marshal.AllocHGlobal(sz * cfg.Notes.Count);
                lock (_allocLock) _pendingAllocations.Add(notesPtr);

                for (int i = 0; i < cfg.Notes.Count; i++)
                {
                    var n = cfg.Notes[i];
                    var native = new NativeMethods.NoteSegment
                    {
                        StartSec = n.StartSec,
                        DurationSec = n.DurationSec,
                        MidiNote = n.MidiNote,
                        Velocity = n.Velocity,
                        Lyric = IntPtr.Zero,
                        F0Override = IntPtr.Zero,
                        EnvOverride = IntPtr.Zero
                    };
                    IntPtr dst = notesPtr + i * sz;
                    Marshal.StructureToPtr(native, dst, false);
                }

                outConfig.Notes = notesPtr;
                outConfig.NoteCount = (uint)cfg.Notes.Count;
            }

            // Options: omit for now (OpenUtau adapter can map via Engine options in future)
            return NativeMethods.UCRAResult.Success;
        }

        private void FreePendingAllocations()
        {
            lock (_allocLock)
            {
                for (int i = 0; i < _pendingAllocations.Count; i++)
                {
                    if (_pendingAllocations[i] != IntPtr.Zero)
                    {
                        Marshal.FreeHGlobal(_pendingAllocations[i]);
                    }
                }
                _pendingAllocations.Clear();
            }
        }

        private void ThrowIfDisposed()
        {
            if (_disposed) throw new ObjectDisposedException(nameof(StreamSession));
        }
    }
}
