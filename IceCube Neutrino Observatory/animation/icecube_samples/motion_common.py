"""Resolution-independent drawing and native-resolution video export."""
from pathlib import Path
from runtime import ffmpeg, font_path
import math, subprocess, time
from functools import lru_cache
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

W,H,FPS=1920,1080,30
S=1
NAVY=(5,16,31); ICE=(125,222,255); WHITE=(226,245,252); GOLD=(255,202,123)
def configure(scale):
    global S
    S=scale
def clamp(x): return max(0.,min(1.,x))
def ease(x):
    x=clamp(x); return x*x*(3-2*x)
def ramp(t,a,b): return ease((t-a)/(b-a))
def mix(a,b,t): return a+(b-a)*t
def sc(p):
    if isinstance(p,(tuple,list,np.ndarray)): return tuple(sc(v) for v in p)
    return float(p)*S
class Draw:
    def __init__(self,im): self.d=ImageDraw.Draw(im)
    def line(self,xy,fill=None,width=1,joint=None): self.d.line(sc(xy),fill=fill,width=max(1,round(width*S)),joint=joint)
    def ellipse(self,xy,fill=None,outline=None,width=1): self.d.ellipse(sc(xy),fill=fill,outline=outline,width=max(1,round(width*S)))
    def polygon(self,xy,fill=None,outline=None,width=1): self.d.polygon(sc(xy),fill=fill,outline=outline,width=max(1,round(width*S)))
    def rectangle(self,xy,fill=None,outline=None,width=1): self.d.rectangle(sc(xy),fill=fill,outline=outline,width=max(1,round(width*S)))
    def rounded_rectangle(self,xy,radius=0,fill=None,outline=None,width=1): self.d.rounded_rectangle(sc(xy),radius=radius*S,fill=fill,outline=outline,width=max(1,round(width*S)))
    def arc(self,xy,start,end,fill=None,width=1): self.d.arc(sc(xy),start,end,fill=fill,width=max(1,round(width*S)))
    def text(self,xy,text,size=32,fill=WHITE,anchor=None): self.d.text(sc(xy),text,font=font(size),fill=fill,anchor=anchor)
@lru_cache(None)
def font(size): return ImageFont.truetype(font_path(),round(size*S))
def layer(): return Image.new('RGBA',(round(W*S),round(H*S)))
def bg():
    y,x=np.mgrid[0:round(H*S),0:round(W*S)].astype(np.float32); x/=S; y/=S
    g=np.exp(-(((x-960)/930)**2+((y-530)/740)**2))
    a=np.empty((round(H*S),round(W*S),3),np.uint8)
    for c,v in enumerate(NAVY): a[:,:,c]=v+g*[4,14,21][c]
    return Image.fromarray(a)
def circle(d,x,y,r,fill=None,outline=None,width=1):
    if r>0: d.ellipse((x-r,y-r,x+r,y+r),fill,outline,width)
def line(d,points,fill=ICE,width=2): d.line(points,fill,width,'curve')
def composite(im,overlay,alpha=1):
    if alpha<1:
        overlay=overlay.copy(); overlay.putalpha(overlay.getchannel('A').point(lambda x:round(x*alpha)))
    im.paste(overlay,(0,0),overlay)
def glow(im,points,r=6):
    if not points: return
    # Bound the blur to the local illuminated region, avoiding a full-frame blur.
    pad=r*12
    x0=max(0,math.floor((min(p[0] for p in points)-pad)*S)); y0=max(0,math.floor((min(p[1] for p in points)-pad)*S))
    x1=min(im.width,math.ceil((max(p[0] for p in points)+pad)*S)); y1=min(im.height,math.ceil((max(p[1] for p in points)+pad)*S))
    if x1<=x0 or y1<=y0: return
    patch=Image.new('RGBA',(x1-x0,y1-y0)); pd=ImageDraw.Draw(patch)
    for x,y,strength in points:
        xx=x*S-x0; yy=y*S-y0; rr=r*3*S
        pd.ellipse((xx-rr,yy-rr,xx+rr,yy+rr),fill=(61,187,255,int(95*strength)))
    patch=patch.filter(ImageFilter.GaussianBlur(r*2*S)); im.paste(patch,(x0,y0),patch)
    d=Draw(im)
    for x,y,strength in points:
        circle(d,x,y,max(1,r*strength),tuple(int(c*strength) for c in ICE))
        circle(d,x,y,max(1,r*.38*strength),WHITE)
def dom(im,x,y,r=50):
    import render as old
    sprite=old.dom_sprite(round(r*S))
    im.paste(sprite,(round(x*S-sprite.width/2),round(y*S-sprite.height/2)),sprite)
def encode(scene,path,duration,preview_path=None):
    path=Path(path); path.parent.mkdir(parents=True,exist_ok=True)
    width,height=round(W*S),round(H*S)
    cmd=[ffmpeg(),'-y','-hide_banner','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s',f'{width}x{height}','-r',str(FPS),'-i','-','-an','-c:v','libx264','-preset','fast','-crf','17','-pix_fmt','yuv420p','-movflags','+faststart',str(path)]
    p=subprocess.Popen(cmd,stdin=subprocess.PIPE); start=time.time()
    try:
        for i in range(round(duration*FPS)):
            im=scene(i/FPS)
            assert im.size==(width,height)
            p.stdin.write(im.tobytes())
            if i%150==0: print(f'{path.stem}: {i}/{round(duration*FPS)} ({time.time()-start:.1f}s)',flush=True)
        p.stdin.close()
        if p.wait(): raise RuntimeError('Video encoding failed')
    except BaseException:
        p.kill(); raise
    print(f'COMPLETE {path}',flush=True)
    if preview_path:
        subprocess.run([ffmpeg(),'-y','-v','error','-i',str(path),'-vf','scale=1920:1080:flags=lanczos','-an','-c:v','libx264','-preset','fast','-crf','19','-movflags','+faststart',str(preview_path)],check=True)

