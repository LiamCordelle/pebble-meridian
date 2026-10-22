#!/usr/bin/env python3
"""Run with pebble-tool's Python. Shared checks; custom visual checks stay local."""
from pathlib import Path
import runpy
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'shared-kit/tools'))
runpy.run_path(str(root/'shared-kit/tools/emulator_smoke.py'))['run_smoke'](root)
