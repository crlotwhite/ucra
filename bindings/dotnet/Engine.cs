using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using UCRA.Interop;

namespace UCRA
{
    /// <summary>
    /// Configuration for audio rendering
    /// </summary>
    public class RenderConfig
    {
        /// <summary>
        /// Gets or sets the sample rate in Hz
        /// </summary>
        public uint SampleRate { get; set; } = 44100;

        /// <summary>
        /// Gets or sets the number of audio channels
        /// </summary>
        public uint Channels { get; set; } = 1;

        /// <summary>
        /// Gets or sets the block size for streaming
        /// </summary>
        public uint BlockSize { get; set; } = 512;

        /// <summary>
        /// Gets or sets additional flags
        /// </summary>
        public uint Flags { get; set; }

        /// <summary>
        /// Gets the list of note segments to render
        /// </summary>
        public List<NoteSegment> Notes { get; } = new List<NoteSegment>();

        /// <summary>
        /// Gets the dictionary of engine options
        /// </summary>
        public Dictionary<string, string> Options { get; } = new Dictionary<string, string>();
    }

    /// <summary>
    /// Result of an audio rendering operation
    /// </summary>
    public class RenderResult
    {
        /// <summary>
        /// Gets the rendered PCM audio data
        /// </summary>
        public float[] PCM { get; internal set; }

        /// <summary>
        /// Gets the number of frames rendered
        /// </summary>
        public ulong Frames { get; internal set; }

        /// <summary>
        /// Gets the number of audio channels
        /// </summary>
        public uint Channels { get; internal set; }

        /// <summary>
        /// Gets the sample rate of the rendered audio
        /// </summary>
        public uint SampleRate { get; internal set; }

        /// <summary>
        /// Gets the metadata associated with the rendering
        /// </summary>
        public IReadOnlyDictionary<string, string> Metadata { get; internal set; }

        /// <summary>
        /// Gets the status of the rendering operation
        /// </summary>
        public NativeMethods.UCRAResult Status { get; internal set; }
    }

    /// <summary>
    /// Represents a UCRA rendering engine
    /// </summary>
    public sealed class Engine : IDisposable
    {
        private IntPtr _handle;
        private bool _disposed;

        /// <summary>
        /// Initializes a new instance of the Engine class
        /// </summary>
        /// <param name="options">Engine creation options</param>
        public Engine(Dictionary<string, string> options = null)
        {
            var nativeOptions = CreateNativeKeyValueArray(options ?? new Dictionary<string, string>());

            try
            {
                var result = NativeMethods.ucra_engine_create(
                    out _handle,
                    nativeOptions.Item1,
                    (uint)nativeOptions.Item2);

                ErrorHelper.CheckResult(result, "Failed to create UCRA engine");
            }
            finally
            {
                FreeNativeKeyValueArray(nativeOptions);
            }
        }

        /// <summary>
        /// Gets engine information
        /// </summary>
        /// <returns>Engine information string</returns>
        public string GetInfo()
        {
            ThrowIfDisposed();

            const int bufferSize = 512;
            var buffer = Marshal.AllocHGlobal(bufferSize);

            try
            {
                var result = NativeMethods.ucra_engine_getinfo(_handle, buffer, (UIntPtr)bufferSize);
                ErrorHelper.CheckResult(result, "Failed to get engine info");

                return Marshal.PtrToStringAnsi(buffer) ?? string.Empty;
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }

        /// <summary>
        /// Renders audio using the specified configuration
        /// </summary>
        /// <param name="config">Render configuration</param>
        /// <returns>Render result</returns>
        public RenderResult Render(RenderConfig config)
        {
            if (config == null)
                throw new ArgumentNullException(nameof(config));

            ThrowIfDisposed();

            var nativeConfig = CreateNativeRenderConfig(config);

            try
            {
                var result = NativeMethods.ucra_render(_handle, ref nativeConfig.Item1, out var nativeResult);
                ErrorHelper.CheckResult(result, "Failed to render audio");

                return CreateManagedRenderResult(nativeResult);
            }
            finally
            {
                FreeNativeRenderConfig(nativeConfig);
            }
        }

        private RenderResult CreateManagedRenderResult(NativeMethods.RenderResult nativeResult)
        {
            var result = new RenderResult
            {
                Frames = nativeResult.Frames,
                Channels = nativeResult.Channels,
                SampleRate = nativeResult.SampleRate,
                Status = nativeResult.Status
            };

            // Copy PCM data if available
            if (nativeResult.PCM != IntPtr.Zero && nativeResult.Frames > 0)
            {
                var totalSamples = (int)(nativeResult.Frames * nativeResult.Channels);
                var pcmData = new float[totalSamples];
                Marshal.Copy(nativeResult.PCM, pcmData, 0, totalSamples);
                result.PCM = pcmData;
            }

            // Copy metadata if available
            var metadata = new Dictionary<string, string>();
            if (nativeResult.Metadata != IntPtr.Zero && nativeResult.MetadataCount > 0)
            {
                for (uint i = 0; i < nativeResult.MetadataCount; i++)
                {
                    var kvPtr = nativeResult.Metadata + (int)(i * Marshal.SizeOf<NativeMethods.KeyValue>());
                    var kv = Marshal.PtrToStructure<NativeMethods.KeyValue>(kvPtr);

                    var key = Marshal.PtrToStringAnsi(kv.Key);
                    var value = Marshal.PtrToStringAnsi(kv.Value);

                    if (key != null && value != null)
                    {
                        metadata[key] = value;
                    }
                }
            }
            result.Metadata = metadata;

            return result;
        }

        private (NativeMethods.RenderConfig, List<GCHandle>) CreateNativeRenderConfig(RenderConfig config)
        {
            var handles = new List<GCHandle>();
            var nativeConfig = new NativeMethods.RenderConfig
            {
                SampleRate = config.SampleRate,
                Channels = config.Channels,
                BlockSize = config.BlockSize,
                Flags = config.Flags
            };

            // Handle notes
            if (config.Notes.Count > 0)
            {
                var nativeNotes = config.Notes.Select(note => note.ToNative()).ToArray();
                var notesHandle = GCHandle.Alloc(nativeNotes, GCHandleType.Pinned);
                handles.Add(notesHandle);

                nativeConfig.Notes = notesHandle.AddrOfPinnedObject();
                nativeConfig.NoteCount = (uint)nativeNotes.Length;
            }

            // Handle options
            if (config.Options.Count > 0)
            {
                var nativeOptions = CreateNativeKeyValueArray(config.Options);
                handles.Add(nativeOptions.Item3); // Add the main array handle
                handles.AddRange(nativeOptions.Item4); // Add string handles

                nativeConfig.Options = nativeOptions.Item1;
                nativeConfig.OptionCount = (uint)nativeOptions.Item2;
            }

            return (nativeConfig, handles);
        }

        private void FreeNativeRenderConfig((NativeMethods.RenderConfig, List<GCHandle>) nativeConfigData)
        {
            foreach (var handle in nativeConfigData.Item2)
            {
                if (handle.IsAllocated)
                    handle.Free();
            }
        }

        private (IntPtr, int, GCHandle, List<GCHandle>) CreateNativeKeyValueArray(Dictionary<string, string> options)
        {
            if (options.Count == 0)
                return (IntPtr.Zero, 0, default, new List<GCHandle>());

            var stringHandles = new List<GCHandle>();
            var nativeArray = new NativeMethods.KeyValue[options.Count];

            int index = 0;
            foreach (var kvp in options)
            {
                // Pin key string
                var keyBytes = Encoding.UTF8.GetBytes(kvp.Key + '\0');
                var keyHandle = GCHandle.Alloc(keyBytes, GCHandleType.Pinned);
                stringHandles.Add(keyHandle);

                // Pin value string
                var valueBytes = Encoding.UTF8.GetBytes(kvp.Value + '\0');
                var valueHandle = GCHandle.Alloc(valueBytes, GCHandleType.Pinned);
                stringHandles.Add(valueHandle);

                nativeArray[index] = new NativeMethods.KeyValue
                {
                    Key = keyHandle.AddrOfPinnedObject(),
                    Value = valueHandle.AddrOfPinnedObject()
                };
                index++;
            }

            var arrayHandle = GCHandle.Alloc(nativeArray, GCHandleType.Pinned);
            return (arrayHandle.AddrOfPinnedObject(), nativeArray.Length, arrayHandle, stringHandles);
        }

        private void FreeNativeKeyValueArray((IntPtr, int, GCHandle, List<GCHandle>) nativeData)
        {
            if (nativeData.Item3.IsAllocated)
                nativeData.Item3.Free();

            foreach (var handle in nativeData.Item4)
            {
                if (handle.IsAllocated)
                    handle.Free();
            }
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(Engine));
        }

        /// <summary>
        /// Disposes the engine and releases native resources
        /// </summary>
        public void Dispose()
        {
            if (!_disposed && _handle != IntPtr.Zero)
            {
                NativeMethods.ucra_engine_destroy(_handle);
                _handle = IntPtr.Zero;
                _disposed = true;
            }
        }
    }
}
