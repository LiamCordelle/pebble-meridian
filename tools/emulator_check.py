#!/usr/bin/env python3
"""Final QEMU verification, run with pebble-tool's Python. Always stops QEMU.

Use --clean-profile to preserve the existing SDK profile. Synthetic health and
weather exercise real device services, AppMessage, Clay and cache paths.
"""
from pathlib import Path
from datetime import datetime
from zoneinfo import ZoneInfo
import json,os,re,shutil,subprocess,sys,tempfile,threading,time
from uuid import UUID
from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.websocket import MessageTargetPhone
from libpebble2.communication.transports.websocket.protocol import AppConfigSetup,AppConfigResponse,WebSocketPhonesimAppConfig,WebSocketPhonesimConfigResponse
from libpebble2.protocol.apps import AppRunState,AppRunStateStart,AppRunStateRequest
from libpebble2.services.appmessage import AppMessageService,Uint32,ByteArray
from pebble_tool.sdk.emulator import ManagedEmulatorTransport
ROOT=Path(__file__).resolve().parents[1];os.chdir(ROOT)
SHOTS=ROOT/'shots';SHOTS.mkdir(exist_ok=True)
PKG=json.loads((ROOT/'package.json').read_text());UUID_=UUID(PKG['pebble']['uuid'])
if '--clean-profile' in sys.argv[1:]:
 from pebble_tool.sdk import get_sdk_persist_dir
 if subprocess.run(['pgrep','-f','qemu-system|qemu-pebble'],capture_output=True).returncode==0:
  raise RuntimeError('Another QEMU is running; leave it undisturbed and retry after it stops')
 profile=Path(get_sdk_persist_dir('emery'))
 backup=Path(tempfile.mkdtemp(prefix='meridian-emulator-'))/'saved-state'
 existed=profile.exists()
 if existed:shutil.move(str(profile),str(backup))
 print('Temporary Emery profile; existing SDK data preserved.',flush=True)
 try:result=subprocess.run([sys.executable,str(Path(__file__).resolve())],cwd=ROOT)
 finally:
  subprocess.run(['pebble','kill'],capture_output=True)
  if profile.exists():shutil.rmtree(profile)
  if existed:shutil.move(str(backup),str(profile))
  backup.parent.rmdir()
  print('Original SDK profile restored; QEMU stopped.',flush=True)
 sys.exit(result.returncode)
def cli(*args):
 print('pebble '+' '.join(map(str,args)),flush=True)
 r=subprocess.run(['pebble',*map(str,args),'--emulator','emery'],check=True,
                  capture_output=True,text=True,timeout=60)
 with (SHOTS/'emulator-commands.txt').open('a') as f:f.write(' '.join(map(str,args))+'\n'+r.stdout+r.stderr+'\n')
 return r
def shot(name):
 time.sleep(0.4) # service callbacks and deferred layer painting must complete
 cli('screenshot','--no-open',SHOTS/('emulator-'+name+'.png'))
def active(conn):
 state=conn.send_and_read(AppRunState(data=AppRunStateRequest()),AppRunState)
 assert state.data.uuid==UUID_,('Wrong active app',state.data.uuid)
connection=None;service=None;logs=None
with (SHOTS/'emulator.log').open('w') as log:
 env=os.environ.copy();env['PYTHONUNBUFFERED']='1'
 logs=subprocess.Popen(['pebble','logs','--emulator','emery'],stdout=log,stderr=subprocess.STDOUT,env=env)
 try:
  time.sleep(20)
  cli('emu-steps',6842);cli('emu-battery','--percent',76)
  cli('install',ROOT/'build'/('pebble-meridian.pbw'))
  connection=PebbleConnection(ManagedEmulatorTransport('emery'));connection.connect();connection.run_async()
  connection.send_packet(AppRunState(data=AppRunStateStart(uuid=UUID_)));time.sleep(2);active(connection)
  print('Verified active Meridian UUID.',flush=True)
  for _ in range(3):cli('emu-heart-rate',64);time.sleep(1)
  print('Allowing a completed health minute (70s).',flush=True)
  for _ in range(2):time.sleep(35)
  cli('emu-tap');cli('emu-tap')
  keys=json.loads((ROOT/'build/js/message_keys.json').read_text())
  service=AppMessageService(connection);service._current_txid=180
  sent=threading.Event()
  subprocess.run(['node','tools/weather_fixture.js'],check=True)
  weather=(ROOT/'build/tests/emulator-weather.bin').read_bytes()
  def incoming(tid,app,data):
   if app==UUID_ and keys['REQUEST_WEATHER'] in data and not sent.is_set():
    sent.set()
    service.send_message(UUID_,{keys['WEATHER_ID']:Uint32(data[keys['REQUEST_WEATHER']]),keys['WEATHER']:ByteArray(weather)})
  service.register_handler('appmessage',incoming)
  def settings(enabled=True,mode=2,unit=0,phone=False):
   values={'TIME_MODE':mode,'TEMP_UNIT':unit,'STEP_GOAL':10000,'WORLD':0,
           'WEATHER_ENABLED':bool(enabled),'DEMO_FALLBACK':False,'LARGE_LABELS':False}
   if phone:
    connection.transport.send_packet(WebSocketPhonesimAppConfig(config=AppConfigSetup()),target=MessageTargetPhone())
    connection.read_transport_message(MessageTargetPhone,WebSocketPhonesimConfigResponse,timeout=10)
    connection.transport.send_packet(WebSocketPhonesimAppConfig(config=AppConfigResponse(data=json.dumps(values))),target=MessageTargetPhone())
   else:service.send_message(UUID_,{keys[k]:Uint32(int(v)) for k,v in values.items()})
   time.sleep(1)
  connection.send_packet(AppRunState(data=AppRunStateRequest()));time.sleep(2)
  settings(False,1,1,phone=True);shot('settings-12h-fahrenheit')
  settings(True,1,1)
  if not sent.wait(8):raise RuntimeError('No correlated weather request')
  time.sleep(1);shot('weather-fahrenheit')
  settings(True,2,0);shot('face')
  cli('emu-tap');shot('detail');time.sleep(8);shot('timeout')
  cli('emu-tap');cli('emu-tap');shot('toggle')
  cli('emu-battery','--percent',5);time.sleep(1);shot('low-battery')
  cli('emu-battery','--percent',62,'--charging');time.sleep(1);shot('charging')
  cli('emu-battery','--percent',76)
  settings(True,2,0,phone=True)
  cli('emu-button','click','back');cli('install',ROOT/'build/pebble-meridian.pbw')
  connection.send_packet(AppRunState(data=AppRunStateStart(uuid=UUID_)));time.sleep(2);active(connection)
  shot('restarted')
  cli('emu-bt-connection','--connected','no');time.sleep(27);shot('no-phone')
  cli('emu-bt-connection','--connected','yes');time.sleep(1);shot('reconnected')
  # Restore normal settings and time within the disposable profile too.
  settings(True,0,0);cli('emu-set-time',int(time.time()))
  cli('emu-button','click','back');time.sleep(1)
 finally:
  if service:
   try:service.shutdown()
   except Exception:pass
  if connection:
   try:connection.transport.ws.close()
   except Exception:pass
  if logs:
   logs.terminate()
   try:logs.wait(timeout=5)
   except subprocess.TimeoutExpired:logs.kill();logs.wait()
  subprocess.run(['pebble','kill'],capture_output=True)
text=(SHOTS/'emulator.log').read_text()
assert not re.search(r'APP_FAULT|stack overflow|app crash',text,re.I),text
assert 'Weather snapshot accepted' in text,'No weather accepted'
assert 'HR minute history ready:' in text,'No history scan completed'
assert 'Kit settings applied: time=1 unit=1' in text,'Clay settings did not arrive'
frames=[int(n) for n in re.findall(r'Frame \w+ (\d+)ms',text)]
assert frames,'No frame timing logs'
normal=[n for n in frames if n < 0x80000000]
clock_sync=[n for n in frames if n >= 0x80000000]
(SHOTS/'emulator-results.json').write_text(json.dumps({'uuid':str(UUID_),'frames_ms':frames,
 'normal_frame_min_ms':min(normal),'normal_frame_max_ms':max(normal),'clock_sync_underflows':clock_sync,
 'frame_min_ms':min(frames),'frame_max_ms':max(frames),'faults':False,
 'weather_fixture':True,'clay_live_settings':True,'health_injected':True},indent=2)+'\n')
subprocess.run(['python3',str(ROOT/'tools/verify_emulator.py')],check=True)
print(f'Final QEMU check passed; normal frame wall times {min(normal)}–{max(normal)}ms ({len(clock_sync)} clock-sync underflows).',flush=True)
