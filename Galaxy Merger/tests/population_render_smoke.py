"""Real OpenGL check of V7 populations, line channels, and portable optical looks.
python tests/population_render_smoke.py build/bin/Release/physics_view.exe build/population-render-tests
"""
from pathlib import Path
import struct
import subprocess
import sys
from PIL import Image, ImageChops, ImageStat

exe = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
out.mkdir(parents=True, exist_ok=True)
fixture = out / "populations.traj"
with fixture.open('wb') as f:
    f.write(b'PHYSTRJ7' + struct.pack('<II3d', 2, 1, 0, 0, 0))
    for name, mass, fraction in [('g.gas.0', .04, 0), ('g.disk.0', .01, 1)]:
        n = name.encode()
        f.write(struct.pack('<I', len(n)) + n)
        f.write(struct.pack('<14d', .1, .8, .8, .8, .5, .4, .2, .4, 1, 6.5, mass, -1, fraction, 0))
    f.write(struct.pack('<I', 0))  # no post-initialization cohorts
    f.write(struct.pack('<10d', 0, *([0]*9)))
    f.write(struct.pack('<12d', *([0]*12)))  # positions, velocities
    f.write(bytes([4, 3]))
    f.write(struct.pack('<8d', -1, -1, .1, 0, .002, 0, 1, 0))
    f.write(struct.pack('<Idd', 0, 0, 0))

def render(name, *flags, success=True):
    image = out / (name + '.png')
    result = subprocess.run([str(exe), str(fixture), '--render', str(image), '--width', '240',
                             '--height', '180', '--distance', '4', '--dust', '0', *flags],
                            capture_output=True, text=True, timeout=60)
    if not success:
        assert result.returncode != 0, (name, 'invalid look accepted')
        return
    assert result.returncode == 0, (name, result.stderr)
    return Image.open(image).convert('RGB')

red = render('halpha', '--young-boost', '0', '--halpha-strength', '24', '--oiii-strength', '0',
             '--halpha-color', '1', '0', '0', '--save-look', str(out/'red.look'))
green = render('oiii', '--young-boost', '0', '--halpha-strength', '0', '--oiii-strength', '24',
               '--oiii-color', '0', '1', '0')
for image, channel in [(red, 0), (green, 1)]:
    sums = ImageStat.Stat(image).sum
    assert sums[channel] > 5 * sum(sums[i] for i in range(3) if i != channel), sums
restored = render('restored', '--look', str(out/'red.look'))
assert ImageChops.difference(red, restored).getbbox() is None, 'saved look changed pixels'
blue = render('blue', '--halpha-strength', '0', '--oiii-strength', '0')
assert ImageStat.Stat(blue).sum[2] > ImageStat.Stat(blue).sum[0], 'young stellar continuum is not blue'
for text in ['ionizing-lifetime -1', 'blue-lifetime nan', 'unknown 1', 'halpha-r 2']:
    bad = out/'bad.look'
    bad.write_text(text, encoding='utf-8')
    render('invalid', '--look', str(bad), success=False)
print('PASS: real V7 rendering, separate H-alpha/OIII/blue continuum, exact look roundtrip, invalid look rejection')
