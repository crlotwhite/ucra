#!/usr/bin/env python3
import os, sys
import sysconfig
import numpy as np

# Locate built extension in build/bindings/python (name ucra.*.so)
repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../'))
build_py_dir = os.path.join(repo_root, 'build', 'bindings', 'python')
sys.path.insert(0, build_py_dir)

try:
    import ucra
except Exception as e:
    print(f"Import error: {e}")
    sys.exit(1)


def write_wav_float32(path, pcm, sample_rate, channels):
    import struct
    pcm = np.asarray(pcm, dtype=np.float32)
    data_size = pcm.size * 4
    file_size = 36 + data_size
    with open(path, 'wb') as f:
        f.write(b'RIFF')
        f.write(struct.pack('<I', file_size))
        f.write(b'WAVE')
        f.write(b'fmt ')
        f.write(struct.pack('<I', 16))
        f.write(struct.pack('<H', 3))  # IEEE float
        f.write(struct.pack('<H', channels))
        f.write(struct.pack('<I', sample_rate))
        byte_rate = sample_rate * channels * 4
        f.write(struct.pack('<I', byte_rate))
        block_align = channels * 4
        f.write(struct.pack('<H', block_align))
        f.write(struct.pack('<H', 32))
        f.write(b'data')
        f.write(struct.pack('<I', data_size))
        f.write(pcm.tobytes())


def main():
    sample_rate = 44100
    channels = 1
    note = ucra.NoteSegment(0.0, 2.0, 67, 120, "sol")
    cfg = ucra.RenderConfig(sample_rate, channels, 512)
    cfg.add_note(note)
    eng = ucra.Engine()
    res = eng.render(cfg)
    # Handle both numpy array and wrapper object styles
    if hasattr(res, 'pcm') and callable(getattr(res, 'pcm')):
        pcm = np.asarray(res.pcm(), dtype=np.float32)
        sr = getattr(res, 'sample_rate', lambda: sample_rate)()
        ch = getattr(res, 'channels', lambda: channels)()
    else:
        pcm = np.asarray(res, dtype=np.float32)
        sr = sample_rate
        ch = channels
    out = os.path.join(os.getcwd(), 'python_sample_output.wav')
    write_wav_float32(out, pcm, int(sr), int(ch))
    print('Wrote', out)

if __name__ == '__main__':
    sys.exit(main())
