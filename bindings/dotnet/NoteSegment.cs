using System;
using System.Runtime.InteropServices;
using System.Text;
using UCRA.Interop;

namespace UCRA
{
    /// <summary>
    /// Represents a musical note segment for rendering
    /// </summary>
    public sealed class NoteSegment : IDisposable
    {
        private readonly string _lyric;
        private readonly F0Curve _f0Override;
        private readonly EnvCurve _envOverride;
        private GCHandle _lyricHandle;
        private IntPtr _lyricPtr;
        private bool _disposed;

        /// <summary>
        /// Initializes a new instance of the NoteSegment class
        /// </summary>
        /// <param name="startSec">Note start time in seconds</param>
        /// <param name="durationSec">Note duration in seconds</param>
        /// <param name="midiNote">MIDI note number (0-127, or -1 if not applicable)</param>
        /// <param name="velocity">Note velocity (0-127)</param>
        /// <param name="lyric">Optional lyric text</param>
        /// <param name="f0Override">Optional F0 curve override</param>
        /// <param name="envOverride">Optional envelope curve override</param>
        public NoteSegment(
            double startSec,
            double durationSec,
            short midiNote = -1,
            byte velocity = 80,
            string lyric = null,
            F0Curve f0Override = null,
            EnvCurve envOverride = null)
        {
            if (durationSec <= 0)
                throw new ArgumentException("Duration must be positive", nameof(durationSec));
            if (velocity > 127)
                throw new ArgumentException("Velocity must be 0-127", nameof(velocity));
            if (midiNote < -1 || midiNote > 127)
                throw new ArgumentException("MIDI note must be -1 or 0-127", nameof(midiNote));

            StartSec = startSec;
            DurationSec = durationSec;
            MidiNote = midiNote;
            Velocity = velocity;
            _lyric = lyric;
            _f0Override = f0Override;
            _envOverride = envOverride;

            // Pin the lyric string if provided
            if (!string.IsNullOrEmpty(_lyric))
            {
                var lyricBytes = Encoding.UTF8.GetBytes(_lyric + '\0'); // Null-terminate
                _lyricHandle = GCHandle.Alloc(lyricBytes, GCHandleType.Pinned);
                _lyricPtr = _lyricHandle.AddrOfPinnedObject();
            }
        }

        /// <summary>
        /// Gets the note start time in seconds
        /// </summary>
        public double StartSec { get; }

        /// <summary>
        /// Gets the note duration in seconds
        /// </summary>
        public double DurationSec { get; }

        /// <summary>
        /// Gets the MIDI note number
        /// </summary>
        public short MidiNote { get; }

        /// <summary>
        /// Gets the note velocity
        /// </summary>
        public byte Velocity { get; }

        /// <summary>
        /// Gets the lyric text
        /// </summary>
        public string Lyric => _lyric;

        /// <summary>
        /// Gets the F0 override curve
        /// </summary>
        public F0Curve F0Override => _f0Override;

        /// <summary>
        /// Gets the envelope override curve
        /// </summary>
        public EnvCurve EnvOverride => _envOverride;

        /// <summary>
        /// Converts this note segment to a native structure
        /// </summary>
        /// <returns>Native NoteSegment structure</returns>
        internal NativeMethods.NoteSegment ToNative()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(NoteSegment));

            var native = new NativeMethods.NoteSegment
            {
                StartSec = StartSec,
                DurationSec = DurationSec,
                MidiNote = MidiNote,
                Velocity = Velocity,
                Lyric = _lyricPtr
            };

            // Set curve overrides if provided
            if (_f0Override != null)
            {
                var f0Native = _f0Override.ToNative();
                var f0Handle = GCHandle.Alloc(f0Native, GCHandleType.Pinned);
                native.F0Override = f0Handle.AddrOfPinnedObject();
                // Note: This creates a temporary handle that should be managed carefully
            }

            if (_envOverride != null)
            {
                var envNative = _envOverride.ToNative();
                var envHandle = GCHandle.Alloc(envNative, GCHandleType.Pinned);
                native.EnvOverride = envHandle.AddrOfPinnedObject();
                // Note: This creates a temporary handle that should be managed carefully
            }

            return native;
        }

        /// <summary>
        /// Disposes the note segment and releases native resources
        /// </summary>
        public void Dispose()
        {
            if (!_disposed)
            {
                if (_lyricHandle.IsAllocated)
                    _lyricHandle.Free();

                _f0Override?.Dispose();
                _envOverride?.Dispose();

                _disposed = true;
            }
        }
    }
}
