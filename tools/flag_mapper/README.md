# UCRA Flag Mapping Tool

Small Python-based tool to define, validate, and apply mapping rules from legacy engine flags to standardized UCRA flags.

Structure:
- schema/: JSON Schema for mapping rule files
- mappings/: example mapping files (moresampler, tn_fnds)
- flag_mapper/: implementation (loader, engine, CLI)
- tests/: pytest unit tests

Run tests (from repo root, using venv):

```bash
source .venv/bin/activate
pip install -r tools/flag_mapper/requirements.txt
pytest -q tools/flag_mapper/tests
```

CLI example:

```bash
source .venv/bin/activate
python3 tools/flag_mapper/cli.py --rule tools/flag_mapper/mappings/moresampler_map.json --flags "g=0.5;v=10"
```
