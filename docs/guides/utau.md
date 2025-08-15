# Integrating UCRA with UTAU-style Editors (CLI Bridge)

This guide shows how to use the UCRA legacy CLI bridge (`resampler`) as a drop-in replacement for UTAU resamplers.

## Prerequisites

- Build the project:
  - Windows (MSVC):
    - `cmake -S . -B build -DUCRA_BUILD_TOOLS=ON`
    - `cmake --build build --config Release`
  - The CLI tool will be at `build/Release/resampler.exe` (MSVC) or `build/resampler` (Makefiles).

## Basic Usage

```powershell
# Example
resampler.exe -i input.wav -o output.wav -n "a 60 100" -v C:\path\to\voicebank -r 44100
```

Required flags:

- `--input` / `-i`: input WAV path
- `--output` / `-o`: output WAV path
- `--note` / `-n`: note info string: `"lyric midi velocity"` (e.g., `"a 60 100"`)
- `--vb-root` / `-v`: voicebank root (must contain `resampler.json`)

Optional flags:

- `--tempo` / `-t`: tempo in BPM (default 120)
- `--flags` / `-f`: legacy engine flags (mapped via flag mapper when available)
- `--f0-curve` / `-c`: path to two-column `time f0` text file
- `--rate` / `-r`: output sample rate (default 44100)

## Voicebank Manifest

Place a manifest at `<voicebank>/resampler.json` following the schema in `schemas/resampler.schema.json`.
The CLI bridge will load the manifest and map flags when applicable.

## Verification

- Run the included tests:
  - `ctest -C Release --output-on-failure`
- Manual smoke test:
  - Render with the CLI and check the resulting WAV.

## Troubleshooting

- Missing `resampler.json`: ensure `--vb-root` points to a directory containing a valid manifest.
- Flag incompatibility: update mapping rules in `tools/flag_mapper/mappings/*` or pass raw `--flags`.
