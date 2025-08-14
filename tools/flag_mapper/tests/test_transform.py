from flag_mapper.transform import TransformEngine


def test_simple_copy():
    rules = [{
        'source': {'name':'g'},
        'target': {'name':'g'},
        'transform': {'kind':'copy'}
    }]
    eng = TransformEngine(rules)
    out = eng.apply({'g':'0.7'})
    assert out['result']['g'] == '0.7'


def test_scale_and_default():
    rules = [{
        'source': {'name':'v'},
        'target': {'name':'velocity','default':100},
        'transform': {'kind':'scale','scale':[0,127]}
    }]
    eng = TransformEngine(rules)
    out = eng.apply({'v':'0.5'})
    assert abs(out['result']['velocity'] - 63.5) < 1e-6


def test_map_missing_warns():
    rules = [{
        'source': {'name':'mode'},
        'target': {'name':'mode'},
        'transform': {'kind':'map','map':{'0':'legato'}}
    }]
    eng = TransformEngine(rules)
    out = eng.apply({'mode':'1'})
    assert 'mode' not in out['result']
    assert len(out['warnings']) == 1
