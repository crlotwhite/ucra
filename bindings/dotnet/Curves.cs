using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using UCRA.Interop;

namespace UCRA
{
    /// <summary>
    /// Represents an F0 (fundamental frequency) curve for vocal synthesis
    /// </summary>
    public sealed class F0Curve : IDisposable
    {
        private float[] _timeSec;
        private float[] _f0Hz;
        private GCHandle _timeHandle;
        private GCHandle _f0Handle;
        private bool _disposed;

        /// <summary>
        /// Initializes a new instance of the F0Curve class
        /// </summary>
        /// <param name="timeSec">Time points in seconds</param>
        /// <param name="f0Hz">F0 values in Hz</param>
        public F0Curve(IEnumerable<float> timeSec, IEnumerable<float> f0Hz)
        {
            if (timeSec == null) throw new ArgumentNullException(nameof(timeSec));
            if (f0Hz == null) throw new ArgumentNullException(nameof(f0Hz));

            _timeSec = timeSec.ToArray();
            _f0Hz = f0Hz.ToArray();

            if (_timeSec.Length != _f0Hz.Length)
            {
                throw new ArgumentException("Time and F0 arrays must have the same length");
            }

            if (_timeSec.Length > 0)
            {
                _timeHandle = GCHandle.Alloc(_timeSec, GCHandleType.Pinned);
                _f0Handle = GCHandle.Alloc(_f0Hz, GCHandleType.Pinned);
            }
        }

        /// <summary>
        /// Gets the time points in seconds
        /// </summary>
        public float[] TimeSec => _timeSec;

        /// <summary>
        /// Gets the F0 values in Hz
        /// </summary>
        public float[] F0Hz => _f0Hz;

        /// <summary>
        /// Gets the number of points in the curve
        /// </summary>
        public int Length => _timeSec.Length;

        /// <summary>
        /// Converts this curve to a native structure
        /// </summary>
        /// <returns>Native F0Curve structure</returns>
        internal NativeMethods.F0Curve ToNative()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(F0Curve));

            if (_timeSec.Length == 0)
            {
                return new NativeMethods.F0Curve
                {
                    TimeSec = IntPtr.Zero,
                    F0Hz = IntPtr.Zero,
                    Length = 0
                };
            }

            return new NativeMethods.F0Curve
            {
                TimeSec = _timeHandle.AddrOfPinnedObject(),
                F0Hz = _f0Handle.AddrOfPinnedObject(),
                Length = (uint)_timeSec.Length
            };
        }

        /// <summary>
        /// Disposes the curve and releases native resources
        /// </summary>
        public void Dispose()
        {
            if (!_disposed)
            {
                if (_timeHandle.IsAllocated)
                    _timeHandle.Free();
                if (_f0Handle.IsAllocated)
                    _f0Handle.Free();
                _disposed = true;
            }
        }
    }

    /// <summary>
    /// Represents an envelope curve for vocal synthesis
    /// </summary>
    public sealed class EnvCurve : IDisposable
    {
        private float[] _timeSec;
        private float[] _value;
        private GCHandle _timeHandle;
        private GCHandle _valueHandle;
        private bool _disposed;

        /// <summary>
        /// Initializes a new instance of the EnvCurve class
        /// </summary>
        /// <param name="timeSec">Time points in seconds</param>
        /// <param name="value">Envelope values</param>
        public EnvCurve(IEnumerable<float> timeSec, IEnumerable<float> value)
        {
            if (timeSec == null) throw new ArgumentNullException(nameof(timeSec));
            if (value == null) throw new ArgumentNullException(nameof(value));

            _timeSec = timeSec.ToArray();
            _value = value.ToArray();

            if (_timeSec.Length != _value.Length)
            {
                throw new ArgumentException("Time and value arrays must have the same length");
            }

            if (_timeSec.Length > 0)
            {
                _timeHandle = GCHandle.Alloc(_timeSec, GCHandleType.Pinned);
                _valueHandle = GCHandle.Alloc(_value, GCHandleType.Pinned);
            }
        }

        /// <summary>
        /// Gets the time points in seconds
        /// </summary>
        public float[] TimeSec => _timeSec;

        /// <summary>
        /// Gets the envelope values
        /// </summary>
        public float[] Value => _value;

        /// <summary>
        /// Gets the number of points in the curve
        /// </summary>
        public int Length => _timeSec.Length;

        /// <summary>
        /// Converts this curve to a native structure
        /// </summary>
        /// <returns>Native EnvCurve structure</returns>
        internal NativeMethods.EnvCurve ToNative()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(EnvCurve));

            if (_timeSec.Length == 0)
            {
                return new NativeMethods.EnvCurve
                {
                    TimeSec = IntPtr.Zero,
                    Value = IntPtr.Zero,
                    Length = 0
                };
            }

            return new NativeMethods.EnvCurve
            {
                TimeSec = _timeHandle.AddrOfPinnedObject(),
                Value = _valueHandle.AddrOfPinnedObject(),
                Length = (uint)_timeSec.Length
            };
        }

        /// <summary>
        /// Disposes the curve and releases native resources
        /// </summary>
        public void Dispose()
        {
            if (!_disposed)
            {
                if (_timeHandle.IsAllocated)
                    _timeHandle.Free();
                if (_valueHandle.IsAllocated)
                    _valueHandle.Free();
                _disposed = true;
            }
        }
    }
}
