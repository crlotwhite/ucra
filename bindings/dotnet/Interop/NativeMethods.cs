using System;
using System.Runtime.InteropServices;

namespace UCRA.Interop
{
    /// <summary>
    /// P/Invoke declarations for the native UCRA C API
    /// </summary>
    public static class NativeMethods
    {
        private const string LibraryName = "ucra_impl";

        #region Enums and Constants

        [Flags]
        public enum UCRAResult : uint
        {
            Success = 0,
            InvalidArgument = 1,
            OutOfMemory = 2,
            NotSupported = 3,
            Internal = 4,
            FileNotFound = 5,
            InvalidJson = 6,
            InvalidManifest = 7
        }

        #endregion

        #region Structures

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct KeyValue
        {
            public IntPtr Key;    // const char*
            public IntPtr Value;  // const char*
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct F0Curve
        {
            public IntPtr TimeSec;  // const float*
            public IntPtr F0Hz;     // const float*
            public uint Length;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct EnvCurve
        {
            public IntPtr TimeSec;  // const float*
            public IntPtr Value;    // const float*
            public uint Length;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct NoteSegment
        {
            public double StartSec;
            public double DurationSec;
            public short MidiNote;
            public byte Velocity;
            public IntPtr Lyric;        // const char*
            public IntPtr F0Override;   // const F0Curve*
            public IntPtr EnvOverride;  // const EnvCurve*
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct RenderConfig
        {
            public uint SampleRate;
            public uint Channels;
            public uint BlockSize;
            public uint Flags;
            public IntPtr Notes;        // const NoteSegment*
            public uint NoteCount;
            public IntPtr Options;      // const KeyValue*
            public uint OptionCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct RenderResult
        {
            public IntPtr PCM;           // const float*
            public ulong Frames;
            public uint Channels;
            public uint SampleRate;
            public IntPtr Metadata;      // const KeyValue*
            public uint MetadataCount;
            public UCRAResult Status;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct ManifestFlag
        {
            public IntPtr Key;           // const char*
            public IntPtr Type;          // const char*
            public IntPtr Description;   // const char*
            public IntPtr DefaultValue;  // const char*
            public IntPtr Range;         // const float*
            public IntPtr Values;        // const char**
            public uint ValuesCount;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct ManifestEntry
        {
            public IntPtr Type;    // const char*
            public IntPtr Path;    // const char*
            public IntPtr Symbol;  // const char*
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct ManifestAudio
        {
            public IntPtr Rates;        // const uint32_t*
            public uint RatesCount;
            public IntPtr Channels;     // const uint32_t*
            public uint ChannelsCount;
            public int Streaming;       // bool-like
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct Manifest
        {
            public IntPtr Name;         // const char*
            public IntPtr Version;      // const char*
            public IntPtr Vendor;       // const char*
            public IntPtr License;      // const char*
            public ManifestEntry Entry;
            public ManifestAudio Audio;
            public IntPtr Flags;        // const ManifestFlag*
            public uint FlagsCount;
        }

        #endregion

        #region Core API

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern UCRAResult ucra_engine_create(
            out IntPtr outEngine,
            IntPtr options,     // const KeyValue*
            uint optionCount);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ucra_engine_destroy(IntPtr engine);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern UCRAResult ucra_engine_getinfo(
            IntPtr engine,
            IntPtr outBuffer,   // char*
            UIntPtr bufferSize);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern UCRAResult ucra_render(
            IntPtr engine,
            ref RenderConfig config,
            out RenderResult outResult);

        #endregion

        #region Manifest API

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern UCRAResult ucra_manifest_load(
            [MarshalAs(UnmanagedType.LPStr)] string manifestPath,
            out IntPtr outManifest);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ucra_manifest_free(IntPtr manifest);

        #endregion

        #region Streaming API

        public delegate UCRAResult PullPCMCallback(IntPtr userData, out RenderConfig outConfig);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern UCRAResult ucra_stream_open(
            out IntPtr outStream,
            ref RenderConfig config,
            PullPCMCallback callback,
            IntPtr userData);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern UCRAResult ucra_stream_read(
            IntPtr stream,
            IntPtr outBuffer,   // float*
            uint frameCount,
            out uint outFramesRead);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ucra_stream_close(IntPtr stream);

        #endregion
    }
}
