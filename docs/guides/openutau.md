# Integrating UCRA with OpenUtau

This guide explains how to integrate a UCRA engine with OpenUtau using the OpenUtau resampler manifest.

## Paths and Artifacts

- UCRA CLI bridge: `resampler` (see build steps below)
- UCRA manifest generator (optional): `ucra_manifest_gen` (built when `UCRA_BUILD_TOOLS=ON`)
- UCRA engine manifest: `resampler.json` (UCRA schema)
- OpenUtau manifest: `resampler.yaml`

## Build

```powershell
cmake -S . -B build -DUCRA_BUILD_TOOLS=ON
cmake --build build --config Release
```

Artifacts:

- `build/Release/resampler.exe` (Windows) or `build/resampler`
- `build/Release/ucra_manifest_gen.exe` if tools are enabled

## Generate OpenUtau Manifest

Option A — Using the generator (recommended when available):

```powershell
ucra_manifest_gen --input <voicebank>\resampler.json --output <voicebank>\resampler.yaml --exe resampler
```

Option B — Manual:

- Start from the UCRA manifest (`schemas/resampler.schema.json`).
- Follow mapping notes in `docs/openutau-manifest.md`.
- Create `resampler.yaml` accordingly.

## Install in OpenUtau

1. Place `resampler.exe` somewhere accessible (e.g., within the voicebank folder or a tools folder).
2. Put `resampler.yaml` into the appropriate OpenUtau path (per OpenUtau docs) or alongside the executable.
3. In OpenUtau, select the resampler in preferences or per-track settings.

## Test

- Load a sample project, select the engine, preview, and render.
- If issues occur, verify the YAML matches your UCRA manifest and that the `--exe` path is correct.
