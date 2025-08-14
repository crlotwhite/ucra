using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UCRA.Interop;

namespace UCRA
{
    /// <summary>
    /// Represents an engine manifest with metadata and capabilities
    /// </summary>
    public sealed class Manifest : IDisposable
    {
        private IntPtr _handle;
        private bool _disposed;

        /// <summary>
        /// Initializes a new instance of the Manifest class by loading from file
        /// </summary>
        /// <param name="manifestPath">Path to the manifest file</param>
        public Manifest(string manifestPath)
        {
            if (string.IsNullOrEmpty(manifestPath))
                throw new ArgumentException("Manifest path cannot be null or empty", nameof(manifestPath));

            var result = NativeMethods.ucra_manifest_load(manifestPath, out _handle);
            ErrorHelper.CheckResult(result, $"Failed to load manifest from '{manifestPath}'");

            LoadManifestData();
        }

        /// <summary>
        /// Gets the engine name
        /// </summary>
        public string Name { get; private set; }

        /// <summary>
        /// Gets the engine version
        /// </summary>
        public string Version { get; private set; }

        /// <summary>
        /// Gets the engine vendor/author
        /// </summary>
        public string Vendor { get; private set; }

        /// <summary>
        /// Gets the license identifier
        /// </summary>
        public string License { get; private set; }

        /// <summary>
        /// Gets the entry point configuration
        /// </summary>
        public ManifestEntry Entry { get; private set; }

        /// <summary>
        /// Gets the audio capabilities
        /// </summary>
        public ManifestAudio Audio { get; private set; }

        /// <summary>
        /// Gets the engine flags
        /// </summary>
        public IReadOnlyList<ManifestFlag> Flags { get; private set; }

        private void LoadManifestData()
        {
            if (_handle == IntPtr.Zero)
                return;

            var nativeManifest = Marshal.PtrToStructure<NativeMethods.Manifest>(_handle);

            Name = Marshal.PtrToStringAnsi(nativeManifest.Name);
            Version = Marshal.PtrToStringAnsi(nativeManifest.Version);
            Vendor = Marshal.PtrToStringAnsi(nativeManifest.Vendor);
            License = Marshal.PtrToStringAnsi(nativeManifest.License);

            // Load entry point info
            Entry = new ManifestEntry
            {
                Type = Marshal.PtrToStringAnsi(nativeManifest.Entry.Type),
                Path = Marshal.PtrToStringAnsi(nativeManifest.Entry.Path),
                Symbol = Marshal.PtrToStringAnsi(nativeManifest.Entry.Symbol)
            };

            // Load audio capabilities
            Audio = LoadAudioCapabilities(nativeManifest.Audio);

            // Load flags
            Flags = LoadFlags(nativeManifest.Flags, nativeManifest.FlagsCount);
        }

        private ManifestAudio LoadAudioCapabilities(NativeMethods.ManifestAudio nativeAudio)
        {
            var rates = new List<uint>();
            if (nativeAudio.Rates != IntPtr.Zero && nativeAudio.RatesCount > 0)
            {
                for (uint i = 0; i < nativeAudio.RatesCount; i++)
                {
                    var ratePtr = nativeAudio.Rates + (int)(i * sizeof(uint));
                    var rate = (uint)Marshal.ReadInt32(ratePtr);
                    rates.Add(rate);
                }
            }

            var channels = new List<uint>();
            if (nativeAudio.Channels != IntPtr.Zero && nativeAudio.ChannelsCount > 0)
            {
                for (uint i = 0; i < nativeAudio.ChannelsCount; i++)
                {
                    var channelPtr = nativeAudio.Channels + (int)(i * sizeof(uint));
                    var channel = (uint)Marshal.ReadInt32(channelPtr);
                    channels.Add(channel);
                }
            }

            return new ManifestAudio
            {
                SupportedRates = rates,
                SupportedChannels = channels,
                SupportsStreaming = nativeAudio.Streaming != 0
            };
        }

        private List<ManifestFlag> LoadFlags(IntPtr flagsPtr, uint flagsCount)
        {
            var flags = new List<ManifestFlag>();
            if (flagsPtr == IntPtr.Zero || flagsCount == 0)
                return flags;

            for (uint i = 0; i < flagsCount; i++)
            {
                var flagPtr = flagsPtr + (int)(i * Marshal.SizeOf<NativeMethods.ManifestFlag>());
                var nativeFlag = Marshal.PtrToStructure<NativeMethods.ManifestFlag>(flagPtr);

                var flag = new ManifestFlag
                {
                    Key = Marshal.PtrToStringAnsi(nativeFlag.Key),
                    Type = Marshal.PtrToStringAnsi(nativeFlag.Type),
                    Description = Marshal.PtrToStringAnsi(nativeFlag.Description),
                    DefaultValue = Marshal.PtrToStringAnsi(nativeFlag.DefaultValue)
                };

                // Load range if available
                if (nativeFlag.Range != IntPtr.Zero)
                {
                    var min = Marshal.PtrToStructure<float>(nativeFlag.Range);
                    var max = Marshal.PtrToStructure<float>(nativeFlag.Range + sizeof(float));
                    flag.Range = (min, max);
                }

                // Load enum values if available
                if (nativeFlag.Values != IntPtr.Zero && nativeFlag.ValuesCount > 0)
                {
                    var values = new List<string>();
                    for (uint j = 0; j < nativeFlag.ValuesCount; j++)
                    {
                        var valuePtr = Marshal.ReadIntPtr(nativeFlag.Values + (int)(j * IntPtr.Size));
                        var value = Marshal.PtrToStringAnsi(valuePtr);
                        if (value != null)
                            values.Add(value);
                    }
                    flag.EnumValues = values;
                }

                flags.Add(flag);
            }

            return flags;
        }

        /// <summary>
        /// Disposes the manifest and releases native resources
        /// </summary>
        public void Dispose()
        {
            if (!_disposed && _handle != IntPtr.Zero)
            {
                NativeMethods.ucra_manifest_free(_handle);
                _handle = IntPtr.Zero;
                _disposed = true;
            }
        }
    }

    /// <summary>
    /// Represents manifest entry point configuration
    /// </summary>
    public class ManifestEntry
    {
        /// <summary>
        /// Gets or sets the entry type (dll, cli, ipc)
        /// </summary>
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the path to the engine binary/library
        /// </summary>
        public string Path { get; set; }

        /// <summary>
        /// Gets or sets the entry symbol name (for dll type)
        /// </summary>
        public string Symbol { get; set; }
    }

    /// <summary>
    /// Represents manifest audio capabilities
    /// </summary>
    public class ManifestAudio
    {
        /// <summary>
        /// Gets or sets the supported sample rates
        /// </summary>
        public IReadOnlyList<uint> SupportedRates { get; set; }

        /// <summary>
        /// Gets or sets the supported channel counts
        /// </summary>
        public IReadOnlyList<uint> SupportedChannels { get; set; }

        /// <summary>
        /// Gets or sets whether streaming is supported
        /// </summary>
        public bool SupportsStreaming { get; set; }
    }

    /// <summary>
    /// Represents a manifest flag definition
    /// </summary>
    public class ManifestFlag
    {
        /// <summary>
        /// Gets or sets the flag key/name
        /// </summary>
        public string Key { get; set; }

        /// <summary>
        /// Gets or sets the flag type
        /// </summary>
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the human-readable description
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets the default value as string
        /// </summary>
        public string DefaultValue { get; set; }

        /// <summary>
        /// Gets or sets the range for numeric types
        /// </summary>
        public (float Min, float Max)? Range { get; set; }

        /// <summary>
        /// Gets or sets the valid values for enum type
        /// </summary>
        public IReadOnlyList<string> EnumValues { get; set; }
    }
}
