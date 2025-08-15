# UCRA Engine Manifest Specification (`resampler.json`)

This document specifies the JSON schema for UCRA engine manifests used by SDKs and tools.
The formal machine-readable schema lives at `schemas/resampler.schema.json`.

- `$id`: https://ucra.dev/schemas/resampler.json
- Draft: JSON Schema draft-07

## Required Top-level Fields

- `name` (string) – Human readable engine name
- `version` (string) – Engine version
- `entry` (object) – Engine entry point
- `audio` (object) – Audio capabilities

## Entry Point (`entry`)

- `type` ("dll" | "cli" | "ipc")
- `path` (string) – Path to engine binary/library
- `symbol` (string, optional) – DLL entry symbol (default: `ucra_entry`)

## Audio Capabilities (`audio`)

- `rates` (int[]) – Supported sample rates (8k–192k)
- `channels` (int[]) – Supported channel counts (1–8)
- `streaming` (boolean, optional, default false)

## Flags (`flags`)

Array of engine configuration flags:

- `key` (string, `/^[a-zA-Z][a-zA-Z0-9_]*$/`)
- `type` ("float" | "int" | "bool" | "string" | "enum")
- `desc` (string)
- `range` ([number, number], required for numeric types)
- `values` (string[], required for enum)
- `default` (any, optional)

## Compatibility (`compat`)

- `utau_cli` (boolean) – Compatible with UTAU CLI bridge
- `openutau_manifest` (boolean) – Can generate OpenUtau manifest

## Example

```json
{
  "name": "ExampleEngine",
  "version": "1.0.0",
  "vendor": "Acme Audio",
  "license": "MIT",
  "entry": {
    "type": "dll",
    "path": "engines/example_engine.dll",
    "symbol": "ucra_entry"
  },
  "audio": {
    "rates": [44100, 48000],
    "channels": [1, 2],
    "streaming": true
  },
  "flags": [
    { "key": "g", "type": "float", "desc": "Gender shift", "range": [-50, 50], "default": 0 },
    { "key": "Y", "type": "int",   "desc": "Breathiness",  "range": [0, 100],  "default": 50 }
  ],
  "compat": { "utau_cli": true, "openutau_manifest": true }
}
```

## Validation

- Validate manifests using the bundled schema: `schemas/resampler.schema.json`.
- Tools in `tools/` and tests (e.g., `test_manifest`) already consume this schema.
