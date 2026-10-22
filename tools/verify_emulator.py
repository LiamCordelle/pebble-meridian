#!/usr/bin/env python3
"""Verify captured QEMU outcomes, without relying on install-success messages."""
from pathlib import Path
import json
from PIL import Image,ImageChops
ROOT=Path(__file__).resolve().parents[1];SHOTS=ROOT/'shots'
def image(name):return Image.open(SHOTS/('emulator-'+name+'.png')).convert('RGB')
def equal(a,b,box):return ImageChops.difference(image(a).crop(box),image(b).crop(box)).getbbox() is None
# The date region must return: detail has a flag/goal ETA in that same region.
assert not equal('face','detail',(65,77,135,95)),'Detail view never appeared'
assert equal('face','timeout',(65,77,135,95)),'Eight-second return failed'
assert equal('face','toggle',(65,77,135,95)),'Second flick did not return'
# Restart without a second synthetic reply must recover weather + 24h/°C state.
assert equal('face','restarted',(2,2,90,42)),'Weather cache/settings did not survive restart'
assert not equal('face','weather-fahrenheit',(2,2,90,23)),'Fahrenheit settings did not apply'
assert not equal('face','low-battery',(120,2,198,42)),'Low battery indicator missing'
assert not equal('face','charging',(120,2,198,42)),'Charging indicator missing'
assert not equal('no-phone','reconnected',(105,2,198,42)),'Phone loss/recovery indicator missing'
result=json.loads((SHOTS/'emulator-results.json').read_text())
result.update(detail_return_verified=True,detail_toggle_verified=True,
 cache_restart_verified=True,status_pixels_verified=True,unit_pixels_verified=True)
(SHOTS/'emulator-results.json').write_text(json.dumps(result,indent=2)+'\n')
print('QEMU pixels verified: detail timeout/toggle, cached weather/settings after restart, units and status.')
