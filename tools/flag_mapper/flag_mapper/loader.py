import json
from jsonschema import validate, ValidationError
from pathlib import Path

class RuleLoadError(Exception):
    pass

class MappingRuleSet:
    def __init__(self, engine, rules, version=None):
        self.engine = engine
        self.rules = rules
        self.version = version

    @staticmethod
    def load(path: str, schema_path: str):
        p = Path(path)
        if not p.exists():
            raise RuleLoadError(f"Rule file not found: {path}")
        data = json.loads(p.read_text())
        schema = json.loads(Path(schema_path).read_text())
        try:
            validate(instance=data, schema=schema)
        except ValidationError as e:
            raise RuleLoadError(f"Schema validation failed: {e.message}")
        return MappingRuleSet(data.get('engine'), data.get('rules'), data.get('version'))
