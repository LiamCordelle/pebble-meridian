#!/usr/bin/env python3
"""Initialize the shared kit if needed, then use its release workflow."""
from pathlib import Path
import runpy
import subprocess
ROOT = Path(__file__).resolve().parents[1]
if not (ROOT/'shared-kit/tools/update_release.py').is_file():
    subprocess.run(['git', 'submodule', 'update', '--init', '--', 'shared-kit'], cwd=ROOT, check=True)
runpy.run_path(str(ROOT/'shared-kit/tools/update_release.py'))['main'](ROOT)
