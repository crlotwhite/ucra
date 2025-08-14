import pytest
from flag_mapper.loader import MappingRuleSet, RuleLoadError


def test_load_valid(tmp_path):
    schema = tmp_path / 'schema.json'
    schema.write_text('''{"type":"object","required":["engine","rules"],"properties":{"engine":{"type":"string"},"rules":{"type":"array"}}}''')
    data = tmp_path / 'map.json'
    data.write_text('{"engine":"moresampler","rules": []}')
    mr = MappingRuleSet.load(str(data), str(schema))
    assert mr.engine == 'moresampler'


def test_load_missing_file(tmp_path):
    with pytest.raises(RuleLoadError):
        MappingRuleSet.load(str(tmp_path / 'nope.json'), str(tmp_path / 'schema.json'))


def test_load_invalid_schema(tmp_path):
    schema = tmp_path / 'schema.json'
    schema.write_text('{"type":"object","required":["engine","rules"],"properties":{"engine":{"type":"string"}}}')
    data = tmp_path / 'map.json'
    data.write_text('{"engine": 123}')
    with pytest.raises(RuleLoadError):
        MappingRuleSet.load(str(data), str(schema))
