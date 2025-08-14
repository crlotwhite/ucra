from typing import Any, Dict, List

class TransformError(Exception):
    pass

class TransformEngine:
    def __init__(self, rules: List[Dict]):
        self.rules = rules

    def apply(self, legacy_flags: Dict[str, Any]) -> Dict[str, Any]:
        result = {}
        warnings = []
        # rules is list of objects with source, target, transform
        for r in self.rules:
            src = r['source']['name']
            tgt = r['target']['name']
            transform = r.get('transform', {'kind':'copy'})
            if src not in legacy_flags:
                # apply default if any
                if 'default' in r['target']:
                    result[tgt] = r['target']['default']
                continue
            val = legacy_flags[src]
            kind = transform.get('kind','copy')
            if kind == 'copy':
                result[tgt] = val
            elif kind == 'scale':
                lo, hi = transform.get('scale', [0,1])
                try:
                    f = float(val)
                    # linear scale from [0,1] -> [lo,hi]
                    result[tgt] = lo + (hi-lo)*f
                except Exception:
                    warnings.append(f"scale: could not convert {val} to float for {src}")
            elif kind == 'map':
                mp = transform.get('map', {})
                if val in mp:
                    result[tgt] = mp[val]
                else:
                    warnings.append(f"map: value {val} not in mapping for {src}")
            elif kind == 'constant':
                result[tgt] = transform.get('value')
            else:
                warnings.append(f"unknown transform kind: {kind}")
        return {'result': result, 'warnings': warnings}
