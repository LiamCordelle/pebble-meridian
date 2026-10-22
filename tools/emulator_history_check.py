#!/usr/bin/env python3
"""Run with pebble-tool's Python. Check QEMU and always stop it afterwards.

QEMU does not establish physical Time 2 history retention. Deterministic history
fixtures, gaps, older records and DST are tested in shared-kit/tests/history.test.c.
"""
from pathlib import Path
import os
import json
import re
import subprocess
import shutil
import sys
import tempfile
import time
from uuid import UUID
from libpebble2.communication import PebbleConnection
from libpebble2.protocol.apps import AppRunState,AppRunStateStart,AppRunStateRequest
from pebble_tool.sdk.emulator import ManagedEmulatorTransport

ROOT=Path(__file__).resolve().parents[1]
PACKAGE=json.loads((ROOT/'package.json').read_text())
OUT=ROOT/'build/history-check'
OUT.mkdir(parents=True,exist_ok=True)

if '--clean-profile' in sys.argv[1:]:
    # Preserve the other faces' emulator data while avoiding a stale launcher
    # state accepting an install but continuing to run the previous watchface.
    from pebble_tool.sdk import get_sdk_persist_dir
    running=subprocess.run(['pgrep','-f','qemu-system|qemu-pebble'],capture_output=True)
    if running.returncode==0:
        raise RuntimeError('Stop the existing emulator before using a clean profile')
    profile=Path(get_sdk_persist_dir('emery'))
    backup=Path(tempfile.mkdtemp(prefix=PACKAGE['name']+'-emulator-'))/'saved-state'
    existed=profile.exists()
    if existed:shutil.move(str(profile),str(backup))
    print('Using temporary clean Emery profile.',flush=True)
    try:
        result=subprocess.run([sys.executable,str(Path(__file__).resolve())],cwd=ROOT)
    finally:
        subprocess.run(['pebble','kill'],cwd=ROOT,capture_output=True)
        if profile.exists():shutil.rmtree(profile)
        if existed:shutil.move(str(backup),str(profile))
        backup.parent.rmdir()
        print('Saved Emery profile restored; emulator stopped.',flush=True)
    sys.exit(result.returncode)

def run(*args):
    result=subprocess.run(['pebble',*args],cwd=ROOT,text=True,capture_output=True,
                          timeout=60,check=True)
    print('pebble '+' '.join(args),flush=True)
    return result

with (OUT/'emulator.log').open('w') as output:
    env=os.environ.copy();env['PYTHONUNBUFFERED']='1'
    logs=subprocess.Popen(['pebble','logs','--emulator','emery'],cwd=ROOT,
                          stdout=output,stderr=subprocess.STDOUT,env=env)
    try:
        time.sleep(20)
        run('emu-steps','6842','--emulator','emery')
        run('install',str(ROOT/'build'/(PACKAGE['name']+'.pbw')),'--emulator','emery')
        # Installation can leave the previous watchface active. Launch the
        # intended UUID explicitly and verify it before accepting any evidence.
        uuid=UUID(PACKAGE['pebble']['uuid'])
        connection=PebbleConnection(ManagedEmulatorTransport('emery'))
        connection.connect();connection.run_async()
        try:
            connection.send_packet(AppRunState(data=AppRunStateStart(uuid=uuid)))
            time.sleep(2)
            current=connection.send_and_read(AppRunState(data=AppRunStateRequest()),AppRunState)
            assert current.data.uuid==uuid,'The emulator is displaying a different app'
            print('Verified active '+PACKAGE['pebble']['displayName']+' UUID.',flush=True)
        finally:
            connection.transport.ws.close()
        for _ in range(3):
            run('emu-heart-rate','64','--emulator','emery');time.sleep(1)
        # Wait for a real completed-minute record and the next model refresh.
        print('Waiting 70 seconds for a completed synthetic health minute.',flush=True)
        for _ in range(2):time.sleep(35)
        run('emu-tap','--emulator','emery');run('emu-tap','--emulator','emery')
        run('screenshot','--no-open','--emulator','emery',str(OUT/'face.png'))
        run('emu-tap','--emulator','emery')
        run('screenshot','--no-open','--emulator','emery',str(OUT/'detail.png'))
        time.sleep(8)
        run('screenshot','--no-open','--emulator','emery',str(OUT/'timeout.png'))
    finally:
        try:
            logs.terminate()
            try:logs.wait(timeout=5)
            except subprocess.TimeoutExpired:logs.kill();logs.wait()
        finally:
            run('kill')

text=(OUT/'emulator.log').read_text()
assert not re.search(r'APP_FAULT|stack overflow|app crash',text,re.I),text
assert 'HR minute history ready:' in text,'No completed history scan was logged'
frames=[int(n) for n in re.findall(r'Frame \w+ (\d+)ms',text)]
assert frames,'No app frames were captured'
print(f'History scan completed without app faults; frames {min(frames)}–{max(frames)}ms.',flush=True)
