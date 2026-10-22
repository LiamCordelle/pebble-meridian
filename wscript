# The implementation lives in the shared kit; ordinary builds never fetch.
from pathlib import Path
import runpy
from waflib import Context
_kit_build = Path(Context.run_dir) / 'shared-kit/tools/pebble_build.py'
if not _kit_build.is_file():
    raise RuntimeError('Shared kit missing. Run: git submodule update --init shared-kit')
_module = runpy.run_path(str(_kit_build))
top, out = '.', 'build'
options, configure, build = _module['options'], _module['configure'], _module['build']
