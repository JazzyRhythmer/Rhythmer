"""Current sample assets; legacy build/six remains an untouched reference."""
import os
from pathlib import Path
def sample_dir(root):
    current=root/'build/six-ssd'
    return Path(os.environ.get('RHYTHMER_SAMPLES_DIR',current if current.exists() else root/'build/six'))
