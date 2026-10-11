# Copyright (c) 2026 Ogetay Kayali. See the repository LICENSE.md.
"""Wordless solar-core / photon / neutrino sequence, native 4K or 1080p."""
from pathlib import Path
from functools import lru_cache
import argparse, math, json
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import motion_common as m
import legacy_rendering_helpers as legacy

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'exports'/'03_solar_photons_and_neutrinos'
DURATION=34
GOLD=(255,205,128); AMBER=(224,141,67); ICE=m.ICE; WHITE=m.WHITE
SUN=None; FULL=None; EARTH=None; BG=None
CUES=[
 (0,6.8,"We cannot receive light directly from the Sun's central region.",'The Sun opens into a cutaway; a photon changes direction inside the core.'),
 (6.8,12,'The material is so dense that light immediately interacts with it.','The camera approaches the interior as a photon repeatedly interacts with dense plasma.'),
 (12,17.8,"But neutrinos produced by nuclear reactions in the Sun's core",'A nuclear process is highlighted in the core; a neutrino emerges along a straight path.'),
 (17.8,20.6,'can pass through these layers with ease,',"The neutrino crosses the Sun's layers and Earth appears."),
 (20.6,23.4,'and we can detect some of them.','A rare neutrino interaction in a schematic detector on Earth produces a recoil electron and light.'),
 (23.4,28.2,'They give us direct information about processes deep inside the Sun','The cutaway, escaping neutrino, and detection event appear together.'),
 (28.2,34,'that we cannot obtain by studying light alone.','The indirect photon path and the neutrino reaching Earth remain visible as detector signals persist.'),
]

def make_sun():
    """Procedural scientific illustration, at maximum native render size."""
    r=650; pad=42; n=round((r+pad)*2*m.S)
    y,x=np.mgrid[0:n,0:n].astype(np.float32)
    x=(x-n/2)/(r*m.S);y=(y-n/2)/(r*m.S)
    rr=np.sqrt(x*x+y*y); ang=np.arctan2(y,x)
    z=np.sqrt(np.clip(1-rr*rr,0,1))
    gran=.035*np.sin(x*201+np.sin(y*51)*2)*np.sin(y*217+np.cos(x*39)*2)+.021*np.cos(x*433+y*291)
    lighting=np.clip(.26+.68*z-.12*x-.14*y+gran,0,1)
    alpha=np.clip((1-rr)*r*m.S,0,1)
    full=np.zeros((n,n,4),np.uint8)
    for c,(lo,hi) in enumerate([(98,255),(44,188),(22,93)]): full[:,:,c]=np.clip(lo+(hi-lo)*lighting,0,255)
    full[:,:,3]=(alpha*255).astype(np.uint8)
    cut=full.copy()
    sector=(np.abs(ang)<1.04)&(rr<1)
    # A radiative interior, convective envelope, and luminous compact core.
    bands=np.clip(.8+.12*np.cos(rr*18)+.04*np.sin(ang*29+rr*37),0,1)
    base=np.zeros((n,n,3),np.float32)
    for c in range(3):
        core=[255,219,149][c]; inner=[163,89,45][c]; outer=[221,136,59][c]
        v=np.where(rr<.24,core-(rr/.24)*[8,44,58][c],np.where(rr<.7,inner+(1-rr/.7)*[49,47,34][c],outer-(rr-.7)*[90,70,32][c]))
        base[:,:,c]=v*bands+gran*60
        cut[:,:,c]=np.where(sector,np.clip(base[:,:,c],0,255),full[:,:,c])
    # Give the core its own glow within the open sector.
    core_glow=np.exp(-(rr/.17)**2)
    for c in range(3): cut[:,:,c]=np.where(sector,np.clip(cut[:,:,c].astype(float)+core_glow*[20,27,34][c],0,255),cut[:,:,c])
    return Image.fromarray(full),Image.fromarray(cut)

def make_earth():
    # Stylized spherical Earth, recognizable by continents and the polar caps.
    r=150; n=round((r+8)*2*m.S)
    y,x=np.mgrid[0:n,0:n].astype(np.float32)
    xx=(x-n/2)/(r*m.S); yy=(y-n/2)/(r*m.S); rr=xx*xx+yy*yy
    zz=np.sqrt(np.clip(1-rr,0,1)); light=np.clip(.23+.73*zz-.22*xx-.13*yy,0,1)
    a=np.zeros((n,n,4),np.uint8)
    for c,k in enumerate([33,116,157]):a[:,:,c]=np.clip(k*light+8,0,255)
    a[:,:,3]=(np.clip((1-np.sqrt(rr))*r*m.S,0,1)*255).astype(np.uint8)
    im=Image.fromarray(a);d=ImageDraw.Draw(im)
    def poly(pts,fill):d.polygon([(round(n/2+px*r*m.S),round(n/2+py*r*m.S)) for px,py in pts],fill=fill)
    poly([(-.85,-.38),(-.61,-.7),(-.33,-.68),(-.2,-.49),(-.34,-.29),(-.14,-.1),(-.3,.03),(-.5,-.13),(-.64,-.11),(-.7,-.31)],(77,160,157,255))
    poly([(-.34,.04),(-.1,.12),(.04,.3),(-.06,.53),(-.21,.8),(-.35,.6),(-.43,.35)],(79,166,161,255))
    poly([(.11,-.62),(.37,-.75),(.67,-.53),(.64,-.29),(.42,-.23),(.36,.04),(.16,-.04),(.05,-.22),(.22,-.38)],(90,165,162,255))
    poly([(.25,-.07),(.48,.01),(.54,.28),(.39,.55),(.19,.42),(.1,.13)],(104,174,163,255))
    poly([(-.42,-.85),(-.08,-.97),(.24,-.9),(.02,-.77)],(204,236,237,255))
    mask=Image.fromarray(a[:,:,3]);im.putalpha(mask)
    d=ImageDraw.Draw(im); d.ellipse((8*m.S,8*m.S,n-8*m.S,n-8*m.S),outline=(109,216,248,240),width=round(2*m.S))
    return im

def paste_sphere(im,sprite,cx,cy,r,base=650):
    k=r/base;size=(round(sprite.width*k),round(sprite.height*k))
    sp=sprite.resize(size,Image.Resampling.LANCZOS)
    im.paste(sp,(round(cx*m.S-sp.width/2),round(cy*m.S-sp.height/2)),sp)

def pose(t):
    zoom=m.ramp(t,5.1,7.4)*(1-m.ramp(t,12,16.5))
    away=m.ramp(t,12,17.8)
    return (m.mix(m.mix(880,650,zoom),520,away),540,m.mix(m.mix(380,650,zoom),340,away))

def sun(im,t):
    cx,cy,r=pose(t);d=m.Draw(im)
    # Soft outer halo, generated as concentric thin strokes rather than a box.
    for j in range(15,0,-1):
        a=(16-j)/16
        m.circle(d,cx,cy,r+j*1.65,None,tuple(int(m.mix(v,w,.05*a)) for v,w in zip((10,27,43),GOLD)),2)
    cut=m.ramp(t,1.1,3.3)
    sprite=FULL if cut==0 else SUN if cut==1 else Image.blend(FULL,SUN,cut)
    paste_sphere(im,sprite,cx,cy,r)
    d=m.Draw(im)
    if cut>0:
        # Fine open-sector edges and layer arcs. No text.
        for a in [-1.04,1.04]:
            m.line(d,[(cx,cy),(cx+math.cos(a)*r,cy+math.sin(a)*r)],(188,136,87),1)
        for rr in [.24,.70]:
            d.arc((cx-r*rr,cy-r*rr,cx+r*rr,cy+r*rr),-59.6,59.6,(226,174,103),1)
    # Visible surface radiation continues: no claim that the Sun emits no light.
    if t<6.8:
        for i,a in enumerate([2.8,3.5,4.2]):
            u=((t*.38+i*.29)%1)
            rr=r+18+u*80
            if .12<u<.83:
                x=cx+rr*math.cos(a);y=cy+rr*math.sin(a)
                m.circle(d,x,y,2.2,tuple(int(m.mix(v,w,math.sin(u*math.pi)*.65)) for v,w in zip((10,27,43),GOLD)))
    return cx,cy,r

WALK=np.array([[.045,.005],[.20,-.08],[.13,.10],[.34,.02],[.28,-.17],[.16,-.08],[.25,.15],[.43,.20],[.46,-.015],[.32,.12],[.22,-.11],[.39,-.19],[.49,.04],[.36,.20],[.18,.05],[.27,-.12],[.045,.005]],float)
MATTER=np.random.default_rng(52).uniform([.10,-.47],[.72,.47],(140,2))
MATTER=np.array([p for p in MATTER if abs(math.atan2(p[1],p[0]))<1.0 and np.linalg.norm(p)<.73])

def photon(im,t,cx,cy,r):
    if t<3.05:return
    d=m.Draw(im);active=m.ramp(t,3.05,3.6)
    # Small plasma scatterers; neither neutral-atom orbits nor a transparent void.
    detail=m.ramp(t,5.4,7.1)*(1-.55*m.ramp(t,12,16.5))
    for j,(x,y) in enumerate(MATTER):
        a=.15+detail*.6
        co=tuple(int(m.mix(c,v,a)) for c,v in zip((104,65,34),(235,181,113)))
        m.circle(d,cx+x*r,cy+y*r,1.4+detail*.8,co)
    for x,y in WALK[:-1]:
        m.circle(d,cx+x*r,cy+y*r,2.4,(215,167,104))
    speed=2.05
    step=(t-3.05)*speed;idx=int(step)%(len(WALK)-1);u=step%1
    p=WALK[idx];q=WALK[idx+1];point=p+(q-p)*u
    # Keep a short angular history to make repeated direction changes legible.
    for k in range(1,min(6,int(step)+1)):
        a=WALK[(idx-k)%(len(WALK)-1)];b=WALK[(idx-k+1)%(len(WALK)-1)]
        col=tuple(int(m.mix(c,v,(6-k)/6*.7)) for c,v in zip((128,78,43),GOLD))
        m.line(d,[(cx+a[0]*r,cy+a[1]*r),(cx+b[0]*r,cy+b[1]*r)],col,2)
    m.line(d,[(cx+p[0]*r,cy+p[1]*r),(cx+point[0]*r,cy+point[1]*r)],GOLD,3)
    px,py=cx+point[0]*r,cy+point[1]*r
    m.circle(d,px,py,5,GOLD);m.circle(d,px,py,2,WHITE)
    # A wave packet plus gamma symbol provides shape-based identification.
    v=(q-p)/np.linalg.norm(q-p);normal=np.array([-v[1],v[0]])
    wav=[]
    for a in np.linspace(-31,-4,35):
        w=np.array([px,py])+v*a+normal*math.sin((a+31)/27*math.tau*2)*4
        wav.append(tuple(w))
    m.line(d,wav,GOLD,2)
    d.text((px+21,py-39),'γ',39,GOLD)
    pulse=1-u
    if pulse>.35:
        m.circle(d,cx+p[0]*r,cy+p[1]*r,7+(1-pulse)*18,None,tuple(int(v*.8) for v in GOLD),1)

def fusion(im,t,cx,cy,r):
    if not 12<t<16.2:return
    d=m.Draw(im);u=m.ramp(t,12,13.7)
    for sign in [-1,1]:
        x=cx+sign*m.mix(43,5,u);y=cy+sign*m.mix(20,2,u)
        m.circle(d,x,y,8,GOLD,WHITE,1)
    a=math.exp(-((t-13.75)/.42)**2)
    if a>.02:
        for j in range(3):m.circle(d,cx,cy,16+j*16+(1-a)*28,None,tuple(int(m.mix(v,w,a)) for v,w in zip((124,76,39),WHITE)),2)

def earth(im,t):
    a=m.ramp(t,16.8,18.8)
    if a<=0:return
    layer=m.layer();paste_sphere(layer,EARTH,1535,380,94,150)
    m.composite(im,layer,a)

def neutrinos(im,t,cx,cy,r):
    if t<13.8:return
    d=m.Draw(im)
    start=np.array([cx+7,cy-4]);target=np.array([1535,380]);direction=target-start
    # The first highlighted neutrino crosses the visible surface during the
    # 'passes through' clause, then arrives at Earth. Timing is illustrative.
    if t<17.8:
        u=m.mix(0,.29,m.ramp(t,13.8,17.8))
    else:u=m.mix(.29,1.37,m.ramp(t,17.8,21.15))
    q=start+direction*u
    trail=start+direction*min(u,1.3)
    m.line(d,[tuple(start),tuple(trail)],(52,133,172),2)
    if u<1.37:
        m.glow(im,[(*q,1)],6);d=m.Draw(im)
        d.text((q[0]+15,q[1]-47),'ν',43,ICE)
    elif t>=21.15:
        d.text((1070,402),'ν',43,ICE)
    # Most neutrinos continue through Earth. Only one rare detection is selected.
    if t>20:
        for j in range(4):
            phase=((t-20)*.36+j*.25)%1
            a=start+np.array([0,(j-1.5)*19]);b=target+np.array([350,(j-1.5)*31])
            p=a+(b-a)*phase
            m.circle(d,*p,3,ICE)
            back=p-(b-a)/np.linalg.norm(b-a)*22
            m.line(d,[tuple(back),tuple(p)],(58,129,166),1)

def detector(im,t):
    appear=m.ramp(t,19.1,20.7)
    if appear<=0:return
    cx,cy,R=1535,790,163
    lay=m.layer();d=m.Draw(lay)
    # A magnified underground optical detector, not IceCube's array.
    m.line(d,[(1496,468),(1423,578),(1404,650)],(58,116,149,200),1)
    m.line(d,[(1558,471),(1659,580),(1671,652)],(58,116,149,200),1)
    m.circle(d,cx,cy,R+13,(8,26,44,245),(77,139,172,230),2)
    m.circle(d,cx,cy,R-8,(12,40,61,255),(32,77,105,255),1)
    for i in range(24):
        a=i*math.tau/24;x=cx+(R-8)*math.cos(a);y=cy+(R-8)*math.sin(a)
        m.circle(d,x,y,5,(59,121,150,255),(130,201,229,255),1)
    d.text((1125,716),'PMT',58,ICE)
    m.line(d,[(1260,755),(1300,755),(1400.8,712.5)],(90,169,204,230),2)
    m.circle(d,1400.8,712.5,8,None,(164,231,252,255),2)
    m.composite(im,lay,appear);d=m.Draw(im)
    if t<20.7:return
    # Neutrino-electron scattering, with outgoing nu and a short charged track.
    event=22.05
    if t>25:event=25.7+3.4*math.floor(max(0,t-25.7)/3.4)
    dt=t-event
    interaction=np.array([cx-34,cy-5])
    if -1.15<dt<.4:
        u=m.clamp((dt+1.15)/1.15)
        p=np.array([cx-R-51,cy+20]);q=interaction
        pos=p+(q-p)*u
        m.line(d,[tuple(p),tuple(pos)],(61,143,184),2)
        m.glow(im,[(*pos,1)],4);d=m.Draw(im)
        if dt<0:d.text((pos[0]-10,pos[1]-46),'ν',31,ICE)
    if 0<=dt<2.2:
        u=m.clamp(dt/.65)
        outgoing=interaction+np.array([260,-52])*u
        m.line(d,[tuple(interaction),tuple(outgoing)],(57,134,172),1)
        if dt<.65:m.circle(d,*outgoing,3,ICE)
        end=interaction+np.array([48,46])*m.ramp(dt,0,.42)
        m.line(d,[tuple(interaction),tuple(end)],WHITE,3)
        d.text((end[0]+9,end[1]+5),'e⁻',25,WHITE)
        hits=[]
        for i in [0,2,4,6,8,10,14,18,22]:
            a=i*math.tau/24
            sensor=np.array([cx+(R-8)*math.cos(a),cy+(R-8)*math.sin(a)])
            flight=m.clamp((dt-.15)/.56)
            p=end+(sensor-end)*flight
            if 0<flight<1:m.circle(d,*p,2.3,ICE)
            if dt>.7:
                strength=max(.15,math.exp(-(dt-.7)/.8))
                hits.append((*sensor,strength))
        if hits:m.glow(im,hits,7)
        d=m.Draw(im)
    # Compact waveform: visual measurement feedback, with no verbal labels.
    if t>=23.4:
        d=m.Draw(im);pts=[]
        for u in np.linspace(0,1,160):
            pulse=math.exp(-((u-.46)/.045)**2)
            pts.append((cx-95+190*u,1038-35*pulse))
        m.line(d,pts,(99,192,228),2)

def frame(t):
    im=BG.copy();cx,cy,r=sun(im,t)
    photon(im,t,cx,cy,r)
    fusion(im,t,cx,cy,r)
    earth(im,t)
    neutrinos(im,t,cx,cy,r)
    detector(im,t)
    # Conclude on the same physical connection, without a dashboard or captions.
    if t>27:
        d=m.Draw(im);a=.25+.25*math.sin((t-27)*1.6)**2
        d.arc((cx-r*.245,cy-r*.245,cx+r*.245,cy+r*.245),-59,59,tuple(int(m.mix(v,w,a)) for v,w in zip(AMBER,GOLD)),2)
    return im

def setup(scale):
    global SUN,FULL,EARTH,BG
    m.configure(scale);BG=m.bg();FULL,SUN=make_sun();EARTH=make_earth()

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'timing.json').write_text(json.dumps(dict(version=2,duration=DURATION,fps=30,width=round(1920*m.S),height=round(1080*m.S),audio=False,narration_language="en",on_screen_symbols=['γ','ν','e⁻','PMT'],timing='Estimated narration timing; no recorded voice supplied.',cues=[dict(start=a,end=b,narration=s,visual=v) for a,b,s,v in CUES]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/'03_solar_photons_and_neutrinos.en.srt').write_text('\n\n'.join(f'{i+1}\n{legacy.stamp(a)} --> {legacy.stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(CUES))+'\n',encoding='utf-8')

def previews():
    ts=[0,3.8,8.7,14.8,18.7,21.7,23.1,30]
    sheet=Image.new('RGB',(1920,540))
    for i,t in enumerate(ts):
        im=frame(t);im.save(OUT/f'frame_{t:04.1f}.jpg',quality=94)
        sheet.paste(im.resize((480,270),Image.Resampling.LANCZOS),(i%4*480,i//4*270))
    sheet.save(OUT/'contact_sheet.jpg',quality=94)
    frame(30).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',type=int,choices=[1,2],default=2);p.add_argument('--preview',action='store_true');a=p.parse_args()
    setup(a.scale);metadata();previews()
    if not a.preview:
        name='03_solar_photons_and_neutrinos_4k.mp4' if a.scale==2 else '03_solar_photons_and_neutrinos_1080p.mp4'
        m.encode(frame,OUT/name,DURATION,OUT/'03_solar_photons_and_neutrinos_1080p.mp4' if a.scale==2 else None)
