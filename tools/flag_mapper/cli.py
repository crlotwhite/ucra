#!/usr/bin/env python3
import argparse
import json
from flag_mapper.loader import MappingRuleSet, RuleLoadError
from flag_mapper.transform import TransformEngine

def parse_flags(s: str):
    out = {}
    if not s:
        return out
    parts = s.split(';')
    for p in parts:
        if '=' in p:
            k,v = p.split('=',1)
            out[k.strip()] = v.strip()
    return out


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--rule', required=True)
    p.add_argument('--schema', default='flag_mapper/schema/mapping_schema.json')
    p.add_argument('--flags', default='')
    args = p.parse_args()
    try:
        ruleset = MappingRuleSet.load(args.rule, args.schema)
    except RuleLoadError as e:
        print(f"Error loading rules: {e}")
        return 2
    eng = TransformEngine(ruleset.rules)
    legacy = parse_flags(args.flags)
    out = eng.apply(legacy)
    print(json.dumps(out['result']))
    if out['warnings']:
        print('\nWarnings:', file=sys.stderr)
        for w in out['warnings']:
            print(w, file=sys.stderr)
    return 0

if __name__ == '__main__':
    import sys
    sys.exit(main())
