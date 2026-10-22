#!/usr/bin/env python3
"""Run the shared regression suite and this consumer's configuration checks."""
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[1]
subprocess.run([sys.executable, str(ROOT/'shared-kit/tests/run.py')], check=True)
subprocess.run(['node', 'tests/consumer.test.js'], cwd=ROOT, check=True)
subprocess.run([sys.executable, 'tests/design_math.py'], cwd=ROOT, check=True)
