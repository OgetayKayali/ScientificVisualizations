"""Optional real OpenGL regression checks. Run with Python + Pillow after building.

python tests/render_smoke.py build/bin/Release/physics_view.exe build/visual-tests
The viewer creates hidden contexts and exits after each PNG; no desktop interaction.
"""
import math
from pathlib import Path
import struct
import subprocess
import sys
from PIL import Image, ImageChops, ImageStat

exe = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
out.mkdir(parents=True, exist_ok=True)
direction = (math.cos(0.8)*math.cos(1.1), math.cos(0.8)*math.sin(1.1), math.sin(0.8))

def fixture(path, dust_depth=1, legacy=False, motion=False, tracer=False):
    with path.open('wb') as f:
        f.write(b'PHYSTRJ3' if legacy else b'PHYSTRJ6' if tracer else b'PHYSTRJ5')
        f.write(struct.pack('<II',2,2 if motion else 1))
        if tracer:f.write(struct.pack('<3d',0.08,0.02,0.4))
        for name, mass, emission in [('test.disk.0',0.01,1),('test.gas.0',0.05,0)]:
            name=name.encode();f.write(struct.pack('<I',len(name)));f.write(name)
            f.write(struct.pack('<10d',0.1,1,0.9,0.7,0.5,emission,0.6,0.8,1,6))
            if not legacy:f.write(struct.pack('<d',0 if tracer and emission else mass))
            if tracer:f.write(struct.pack('<d',mass if emission else -1))
        for t in range(2 if motion else 1):
            f.write(struct.pack('<10d',t,*([0]*9)))
            f.write(struct.pack('<6d',(-1+2*t) if motion else 0,0,0,
                                *(x*dust_depth for x in direction)))
            f.write(struct.pack('<6d',2 if motion else 0,0,0,0,0,0))
            f.write(bytes([3,4]))
            if not legacy:
                f.write(struct.pack('<6d',-1,-1,0,1,0,0.002))
                f.write(struct.pack('<Idd',0,0,0))

def render(name, path, time=0, dust=1):
    target=out/(name+'.png')
    result=subprocess.run([str(exe),str(path),'--render',str(target),'--width','320','--height','180',
                           '--distance','4','--time',str(time),'--dust',str(dust)],
                          capture_output=True,text=True,timeout=60)
    assert result.returncode==0,(name,result.stderr)
    assert target.is_file(),name
    return Image.open(target).convert('RGB')

foreground=out/'foreground.traj';background=out/'background.traj';legacy=out/'legacy.traj';motion=out/'motion.traj'
fixture(foreground,1);fixture(background,-1);fixture(legacy,legacy=True);fixture(motion,motion=True)
clear=render('clear',foreground,dust=0)
front=render('front',foreground)
back=render('back',background)
assert sum(ImageStat.Stat(clear).sum)>0,'empty render'
assert sum(ImageStat.Stat(front).sum)<0.5*sum(ImageStat.Stat(clear).sum),'foreground dust failed to absorb'
assert ImageChops.difference(clear,back).getbbox() is None,'background gas obscured foreground stars'
assert sum(ImageStat.Stat(render('legacy',legacy,dust=0)).sum)>0,'legacy recording failed'
before=render('motion_025',motion,time=0.25,dust=0)
after=render('motion_035',motion,time=0.35,dust=0)
assert ImageChops.difference(before,after).getbbox() is not None,'playback repeated a stored snapshot'
embedded=out/'embedded.traj'
fixture(embedded,0)
mixed=render('embedded',embedded)
assert sum(ImageStat.Stat(front).sum)<sum(ImageStat.Stat(mixed).sum)<sum(ImageStat.Stat(clear).sum), 'embedded stars need partial dust absorption'
tracer=out/'tracer.traj'
fixture(tracer,tracer=True)
traced=render('tracer',tracer,dust=0)
assert ImageChops.difference(clear,traced).getbbox() is None,'V6 massless light changed brightness'
print('PASS: real OpenGL emission, foreground/background/embedded dust, V3/V5/V6 playback, massless light, sub-snapshot motion')
