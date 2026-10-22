#!/usr/bin/env python3
"""Deterministic Meridian coverage and pixel assertions; never starts QEMU."""
import concurrent.futures
import json
import os
from pathlib import Path
import subprocess
from PIL import Image, ImageChops, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
LAB=Path(os.environ.get('WATCHFACE_LAB',ROOT.parent/'watchface-lab'))
SHOTS=ROOT/'shots'
scenes=[('demo',0,False,{}),('stress',1,False,{}),('empty',2,False,{}),
 ('detail',0,True,{}),('detail-stress',1,True,{}),('detail-empty',2,True,{}),
 ('12h',0,False,{'12H':1,'HOUR':9,'MINUTE':5}),('detail-12h',0,True,{'12H':1}),
 ('midnight',0,False,{'HOUR':0,'MINUTE':8}),
 ('01h-five-digits',1,False,{'HOUR':1,'MINUTE':0,'CHARGING':0}),
 ('after-sunset',0,False,{'HOUR':21,'MINUTE':15,'UV':0,'UV_MAX':60}),
 ('after-sunset-detail',0,True,{'HOUR':21,'MINUTE':15}),
 ('no-phone',0,False,{'PHONE':0}),('no-health',0,False,{'NO_HEALTH':1}),
 ('no-health-detail',0,True,{'NO_HEALTH':1}),('no-weather',0,False,{'NO_WEATHER':1}),
 ('no-sun',0,False,{'NO_SUN':1}),('low-battery',0,False,{'BATTERY':19}),
 ('critical-battery',0,False,{'BATTERY':5}),('charging',0,False,{'BATTERY':62,'CHARGING':1}),
 ('offline-charging',0,False,{'PHONE':0,'BATTERY':99,'CHARGING':1,'TEMP':-99}),
 ('offline-low',0,False,{'PHONE':0,'BATTERY':5,'TEMP':-99}),
 ('fahrenheit',0,False,{'FAHRENHEIT':1}),('fahrenheit-detail',0,True,{'FAHRENHEIT':1}),
 ('fahrenheit-wide',0,True,{'FAHRENHEIT':1,'HI':55,'LO':-40,'HR_MIN':35,'HR_MAX':220,'HR_PARTIAL':1}),
 ('partial-heart',0,True,{'HR_PARTIAL':1}),('no-range',0,True,{'NO_RANGE':1}),
 ('partial-no-range',0,True,{'NO_RANGE':1,'HR_PARTIAL':1}),
 ('history-no-current',0,True,{'NO_CURRENT_HR':1}),
 ('behind',0,True,{'USUAL':7182}),('on-pace',0,True,{'USUAL':6842}),
 ('no-usual',0,True,{'USUAL':0}),('zero-steps',0,False,{'STEPS':0}),
 ('near-goal',0,False,{'STEPS':9870}),('goal',0,False,{'STEPS':10000}),
 ('rain-midnight-dry',0,False,{'HOUR':22,'TOMORROW_RAIN':0}),
 ('rain-midnight-wet',0,False,{'HOUR':22,'TOMORROW_RAIN':90}),
 ('short-forecast',0,False,{'HOUR':22,'TOMORROW_RAIN':90,'FORECAST_COUNT':2}),
 ('no-forecast',0,False,{'FORECAST_COUNT':0}),
 ('night-uv-zero',0,False,{'HOUR':21,'UV':0,'UV_MAX':0})]
for name,code in [('sun',0),('partly',2),('cloud',3),('rain',63),('snow',73),('storm',95),('fog',45)]:
 scenes.append(('weather-'+name,0,False,{'WEATHER_CODE':code}))
def render(scene):
 name,scenario,detail,values=scene
 env={k:v for k,v in os.environ.items() if not k.startswith('MERIDIAN_')}
 env.update({'MERIDIAN_'+k:str(v) for k,v in values.items()})
 args=['python3',str(LAB/'tools/render.py'),str(ROOT),str(SHOTS/(name+'.png')),'--scenario',str(scenario)]
 if detail:args.append('--detail')
 r=subprocess.run(args,env=env,capture_output=True,text=True)
 if r.returncode:raise RuntimeError(r.stdout+r.stderr)
 return name
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:list(pool.map(render,scenes))
def img(name):return Image.open(SHOTS/(name+'.png')).convert('RGB')
def changed(a,b,rect=None):
 a,b=img(a),img(b)
 if rect:a,b=a.crop(rect),b.crop(rect)
 return ImageChops.difference(a,b).getbbox() is not None
reference=Image.open(LAB/'designs/39-meridian2/shots/demo.png').convert('RGB')
assert ImageChops.difference(reference,img('demo')).getbbox() is None,'Demo art changed'
# The next-day rain must affect the lower-left overnight dial, beyond midnight.
assert changed('rain-midnight-wet','rain-midnight-dry',(15,118,82,191)),'Missing tomorrow rain'
# A short cache must not wrap nonexistent observations into tomorrow.
assert not changed('short-forecast','rain-midnight-dry',(15,118,82,191)),'Forecast cache repeated'
assert changed('after-sunset','night-uv-zero',(22,25,45,41)),'Night UV does not use daily peak'
assert changed('detail','behind',(62,134,135,151)),'Pace direction missing'
assert not changed('detail','history-no-current',(2,2,198,22)),'Recorded HR range hidden without current reading'
# All unchanged pixels must still match the original, not merely look similar.
reference_audit={}
for name in ('demo','stress','empty','detail','detail-stress'):
 original=Image.open(LAB/'designs/39-meridian2/shots'/(name+'.png')).convert('RGB')
 delta=ImageChops.difference(img(name),original)
 changes=0
 for y in range(228):
  for x in range(200):
   if delta.getpixel((x,y))==(0,0,0):continue
   changes+=1;r2=(x-100)**2+(y-114)**2
   allowed=(name!='demo' and 54**2<=r2<=85**2) or (name.startswith('detail') and 53<=x<=147 and 132<=y<=153) or (name=='stress' and 23<=x<=65 and 22<=y<=41) or (name=='empty' and 80<=x<=198 and y<=44)
   assert allowed, f'Unexpected art change: {name} at {x},{y}'
 reference_audit[name]={'changed_pixels':changes,'unexplained_pixels':0,'difference_bounds':delta.getbbox()}
(SHOTS/'reference-audit.json').write_text(json.dumps(reference_audit,indent=2)+'\n')
for name in ('demo','detail','stress','detail-stress','empty','detail-empty'):
 subprocess.run(['python3',str(LAB/'tools/glance.py'),str(SHOTS/(name+'.png'))],check=True,capture_output=True)
cols=6;rows=(len(scenes)+cols-1)//cols
sheet=Image.new('RGB',(cols*216,rows*254),'#dedede');draw=ImageDraw.Draw(sheet)
for i,(name,*_) in enumerate(scenes):
 x=(i%cols)*216+8;y=(i//cols)*254
 draw.text((x,y+4),name,fill='black');sheet.paste(img(name),(x,y+22))
sheet.save(SHOTS/'coverage.png')
(SHOTS/'coverage.json').write_text(json.dumps({'scenes':[s[0] for s in scenes],
 'demo_pixel_identical':True,'midnight_rain_checked':True,'short_cache_checked':True,
 'night_peak_uv_checked':True,'recorded_hr_without_current_checked':True},indent=2)+'\n')
print(f'Rendered {len(scenes)} scenes. Demo pixel-identical; feature regression assertions passed.')
