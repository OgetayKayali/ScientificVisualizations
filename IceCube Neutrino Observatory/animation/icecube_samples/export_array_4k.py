"""Re-rasterize the approved v2 geometry at 3840x2160; never upscale its video."""
from types import SimpleNamespace
import json
import render_array_v2 as a
import motion_common as m

OUT=a.shared.ROOT/'exports'/'icecube_array_v2_4k'
if __name__=='__main__':
    m.configure(2); OUT.mkdir(parents=True,exist_ok=True)
    # Inject native-resolution drawing while retaining v2's logical coordinates.
    a.ImageDraw=SimpleNamespace(Draw=m.Draw)
    a.Image=SimpleNamespace(new=lambda mode,size: m.layer())
    a.shared=SimpleNamespace(dom=m.dom,glow=m.glow)
    a.BG=m.bg()
    a.scene(22).save(OUT/'poster_4k.jpg',quality=95)
    for t in [5.2,10,17.3,26]: a.scene(t).save(OUT/f'kontrol_{t:04.1f}.jpg',quality=95)
    m.encode(a.scene,OUT/'01_icecube_dizilim_yazisiz_4k.mp4',32)
    (OUT/'export.json').write_text(json.dumps(dict(width=3840,height=2160,fps=30,duration=32,audio=False,render='Native-resolution rasterization of approved v2 geometry; not video upscaling.',source='animation/icecube_samples/render_array_v2.py'),indent=2),encoding='utf-8')
