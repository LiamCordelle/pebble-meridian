#!/usr/bin/env python3
"""Generate a crisp native Meridian icon and static store icons."""
from pathlib import Path
from io import BytesIO
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
# 25px launcher icon: sky ring, sun and a flooded central porthole.
icon=Image.new('RGBA',(25,25),(0,0,0,0));d=ImageDraw.Draw(icon)
d.ellipse((1,1,23,23),fill='#000055',outline='#AAAAFF',width=2)
d.arc((1,1,23,23),180,335,fill='#FFAA55',width=2)
d.ellipse((7,7,17,17),fill='#000055',outline='#AAAAFF')
d.pieslice((8,8,16,16),0,180,fill='#0055FF')
d.ellipse((2,3,8,9),fill='#FFFF55');d.ellipse((4,5,6,7),fill='white')
buffer=BytesIO();icon.save(buffer,format='PNG')
icon_path=ROOT/'resources/images/menu_icon.png'
if not icon_path.exists() or icon_path.read_bytes()!=buffer.getvalue():
 icon_path.write_bytes(buffer.getvalue())
image=Image.open(ROOT/'shots/demo.png').convert('RGB')
for size in (80,144):
 image.resize((size,size),Image.Resampling.LANCZOS).save(ROOT/f'icon_{size}x{size}.png')
(ROOT/'store').mkdir(exist_ok=True)
for source,name in [('shots/demo.png','face.png'),('shots/detail.png','detail.png'),
                    ('icon_80x80.png','icon_80x80.png'),('icon_144x144.png','icon_144x144.png')]:
 Image.open(ROOT/source).save(ROOT/'store'/name)
print('Created Meridian launcher/store icons and face/detail store screenshots.')
