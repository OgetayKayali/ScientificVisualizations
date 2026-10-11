# Copyright (c) 2026 Ogetay Kayali. See the repository LICENSE.md.
"""Editable, deterministic IceCube motion graphics. No external visual assets."""
from pathlib import Path
from runtime import ffmpeg, font_path
import argparse, json, math, subprocess, time
from functools import lru_cache
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'exports' / 'icecube_samples_v1'
W,H,FPS = 1920,1080,30
NAVY=(5,16,31); WHITE=(226,245,252); ICE=(125,222,255); CYAN=(63,193,243)
MUTED=(116,157,182); LINE=(35,74,101); GOLD=(255,218,153)
CUES = {
 '01_icecube_detector_array_legacy': [
  [0,3.6,'Each string carries 60 detectors,', 'A single string appears with 60 DOMs lighting up in sequence.'],
  [3.6,6.5,'spaced 17 meters apart,', 'The camera moves closer to two DOMs on the same string, emphasizing their spacing.'],
  [6.5,11.5,'extending from a depth of 1.5 kilometers to 2.5 kilometers.', 'The camera pulls back to reveal the ice surface, the long cable, and the deeply buried modules.'],
  [11.5,15.2,'There are 86 of these strings,', 'One string expands into a schematic array of 86 strings.'],
  [15.2,19.2,'with standard strings spaced about 125 meters apart,', 'Two neighboring standard strings light up while the surrounding array remains dim.'],
  [19.2,23.8,'forming an observatory that spans roughly one cubic kilometer of ice.', 'A slow camera rotation reveals the volume through parallax, without an enclosing cube.'],
  [23.8,28.2,'This lets us follow particles step by step.', 'A representative track advances, followed by signals at nearby DOM positions in the schematic array.'],
  [28.2,32,'But detecting them is not quite that simple.', 'The completed track and detector array hold for the transition.'],
 ],
 '02_photomultiplier_legacy': [
  [0,4.8,'Light striking the photocathode releases an electron.', 'A photon reaches the photocathode and releases one electron.'],
  [4.8,8.3,'A high voltage applied between successive electrodes', 'The voltage difference and successive dynodes are highlighted.'],
  [8.3,12.1,'accelerates the electron toward the next electrode,', 'The first electron accelerates toward the first dynode.'],
  [12.1,17.8,'multiplying the number of electrons at every stage.', 'Electrons multiply in dense, individually distinguishable bunches.'],
  [17.8,24,'The resulting electrical signal can be amplified by a million to a hundred million times.', 'Charge collects at the anode, followed by a gain indication and an output pulse.'],
 ]}

def clamp(x): return max(0.,min(1.,x))
def ease(x):
    x=clamp(x); return x*x*(3-2*x)
def ramp(t,a,b): return ease((t-a)/(b-a))
def mix(a,b,t): return a+(b-a)*t
def color(c,a=1): return tuple(int(v*a) for v in c)
@lru_cache(None)
def font(s,b=False):
    return ImageFont.truetype(font_path(b),s)
def txt(d,xy,s,size=30,fill=WHITE,bold=False,anchor=None):
    d.text(xy,s,font=font(size,bold),fill=fill,anchor=anchor)
def line(d,points,fill=ICE,width=2): d.line(points,fill=fill,width=width,joint='curve')
def circle(d,x,y,r,fill,outline=None,width=1):
    if r>0: d.ellipse((x-r,y-r,x+r,y+r),fill=fill,outline=outline,width=width)
def arrow(d,p,q,fill=ICE,width=3,head=12):
    line(d,[p,q],fill,width)
    a=math.atan2(q[1]-p[1],q[0]-p[0])
    d.polygon([q,(q[0]-head*math.cos(a-.45),q[1]-head*math.sin(a-.45)),(q[0]-head*math.cos(a+.45),q[1]-head*math.sin(a+.45))],fill=fill)
def dashed(d,p,q,fill=LINE,width=2,dash=9):
    n=math.hypot(q[0]-p[0],q[1]-p[1])
    for i in np.arange(0,n,dash*2):
        a=i/n; b=min(i+dash,n)/n
        line(d,[(mix(p[0],q[0],a),mix(p[1],q[1],a)),(mix(p[0],q[0],b),mix(p[1],q[1],b))],fill,width)
def dim(d,p,q,label,vertical=False):
    line(d,[p,q],ICE,2)
    dx,dy=(9,0) if vertical else (0,9)
    for x,y in [p,q]: line(d,[(x-dx,y-dy),(x+dx,y+dy)],ICE,2)
    if vertical: txt(d,((p[0]+q[0])/2+23,(p[1]+q[1])/2),label,32,anchor='lm')
    else: txt(d,((p[0]+q[0])/2,(p[1]+q[1])/2-28),label,32,anchor='mm')

def make_bg():
    y,x=np.mgrid[0:H,0:W]
    g=np.exp(-(((x-1120)/850)**2+((y-560)/650)**2))
    arr=np.zeros((H,W,3),np.uint8)
    for c,v in enumerate(NAVY): arr[:,:,c]=np.clip(v+g*[4,14,21][c],0,255)
    im=Image.fromarray(arr); d=ImageDraw.Draw(im)
    rng=np.random.default_rng(214)
    for _ in range(170):
        px,py=rng.integers([50,180],[W-50,H-80]); circle(d,int(px),int(py),1,(19,46,64))
    return im
BG=make_bg()
def base(num,title,sub):
    im=BG.copy(); d=ImageDraw.Draw(im)
    line(d,[(100,77),(151,77)],ICE,4)
    txt(d,(169,56),'ICECUBE  /  '+num,24,MUTED)
    txt(d,(98,113),title,54,WHITE,True)
    txt(d,(100,190),sub,25,MUTED)
    line(d,[(100,987),(1820,987)],LINE,1)
    return im

@lru_cache(None)
def dom_sprite(r):
    s=r*2+16; yy,xx=np.mgrid[0:s,0:s]; cx=cy=s/2
    nx=(xx-cx)/r; ny=(yy-cy)/r; rr=nx*nx+ny*ny
    mask=rr<=1; z=np.sqrt(np.maximum(0,1-rr))
    arr=np.zeros((s,s,4),np.uint8)
    light=np.clip(.22+.55*z-.2*nx-.25*ny,0,1)
    for c,co in enumerate([60,122,165]): arr[:,:,c]=np.clip(co*light+12,0,255)
    arr[:,:,3]=mask*255
    im=Image.fromarray(arr); d=ImageDraw.Draw(im)
    box=(8,8,s-8,s-8)
    d.ellipse(box,outline=(135,215,240,255),width=max(1,r//45))
    # Downward-looking photocathode, with upper electronics and equatorial band.
    d.pieslice((cx-r*.76,cy-r*.68,cx+r*.76,cy+r*.84),0,180,fill=(43,85,111,255),outline=(148,211,222,255),width=max(1,r//60))
    d.arc((cx-r*.7,cy-r*.6,cx+r*.7,cy+r*.64),7,172,fill=(208,232,225,255),width=max(1,r//30))
    d.rounded_rectangle((cx-r*.5,cy-r*.43,cx+r*.5,cy-r*.2),radius=max(1,r//20),fill=(27,62,80,255),outline=(84,150,168,255),width=1)
    for n in range(5):
        x=cx-r*.37+n*r*.18
        d.rectangle((x,cy-r*.38,x+r*.08,cy-r*.28),fill=(119,164,168,255))
    d.ellipse((8,cy-r*.11,s-8,cy+r*.11),outline=(86,145,177,255),width=max(2,r//24))
    d.arc((12,12,s-12,s-12),205,286,fill=(199,244,255,255),width=max(2,r//28))
    return im
def dom(im,x,y,r=50):
    sp=dom_sprite(int(r)); im.paste(sp,(int(x-sp.width/2),int(y-sp.height/2)),sp)
def glow(im,points,r=6):
    layer=Image.new('RGBA',(W,H)); d=ImageDraw.Draw(layer)
    for x,y,strength in points:
        circle(d,x,y,r*3,(61,187,255,int(95*strength)))
    layer=layer.filter(ImageFilter.GaussianBlur(r*2)); im.paste(layer,(0,0),layer)
    d=ImageDraw.Draw(im)
    for x,y,strength in points:
        circle(d,x,y,max(1,r*strength),color(ICE,strength))
        circle(d,x,y,max(1,r*.38*strength),WHITE)

# A schematic 86-string footprint: 78 outer strings and eight denser central ones.
outer=[(q,r) for q in range(-5,6) for r in range(-5,6) if max(abs(q),abs(r),abs(q+r))<=5 and (q,r)!=(0,0)]
outer=sorted(outer,key=lambda p:p[0]**2+p[0]*p[1]+p[1]**2,reverse=True)[:78]
positions=[(q+.5*r,r*.866,False) for q,r in outer]
positions += [(math.cos(i*math.tau/8)*.65,math.sin(i*math.tau/8)*.65,True) for i in range(8)]
positions.sort(key=lambda p:p[1])
assert len(positions)==86
def project(x,y,z,spread=1): return (1060+(x*67+y*31)*spread,405+y*24*spread+z*430)

def scene1(t):
    im=base('01','An observatory in the ice','From one module to a three-dimensional detector array')
    d=ImageDraw.Draw(im)
    morph=ramp(t,11.5,14.1)
    if t<13:
        a=1-ramp(t,11.5,13)
        txt(d,(105,360),'60',138,color(WHITE,a),True)
        txt(d,(111,523),'DOMs / string',32,color(ICE,a))
        txt(d,(112,575),'Digital Optical Module',25,color(MUTED,a))
    else:
        if t<19.2:
            txt(d,(105,352),'86',138,WHITE,True)
            txt(d,(111,516),'strings',34,ICE)
            txt(d,(111,569),'5,160 optical modules',28,MUTED)
        else:
            txt(d,(107,356),'≈ 1 km³',96,WHITE,True)
            txt(d,(111,488),'instrumented detector volume',30,ICE)
            txt(d,(111,545),'86 strings · 5,160 DOMs',26,MUTED)
    # Morph from a single string to a distributed array.
    cx=mix(825,1060,morph)
    if t<12.8:
        top,bottom=335,871
        line(d,[(cx,264),(cx,bottom+20)],(69,139,174),3)
        reveal=ramp(t,.2,2.8)
        for j in range(60):
            if j/60<=reveal:
                y=top+(bottom-top)*j/59
                circle(d,cx,y,4,ICE); circle(d,cx-1,y-1,1,WHITE)
        if 3.4<t<11.7:
            a=ramp(t,3.4,4.2)
            # A magnified pair, clearly separated from the full string.
            dashed(d,(cx+15,555),(1275,555),color(LINE,a),2)
            txt(d,(1325,299),'CLOSE-UP',23,color(MUTED,a))
            line(d,[(1405,342),(1405,825)],color(LINE,a),3)
            dom(im,1405,440,78); dom(im,1405,717,78)
            d=ImageDraw.Draw(im)
            dim(d,(1563,440),(1563,717),'17 m',True)
        if 6.5<t<11.8:
            a=ramp(t,6.5,7.2)
            for y,s in [(335,'≈ 1,5 km'),(871,'≈ 2,5 km')]:
                dashed(d,(875,y),(1190,y),color(LINE,a))
                txt(d,(910,y-42),s,32,color(WHITE,a))
            txt(d,(925,615),'DEPTH',21,color(MUTED,a))
    if morph>0:
        # Translucent ice volume; annotation line sits outside the array.
        if t>=19:
            op=ramp(t,19.2,20.7)
            lay=Image.new('RGBA',(W,H)); ld=ImageDraw.Draw(lay)
            corners=[project(-5.4,-4.6,0),project(5.4,-4.6,0),project(5.4,4.6,0),project(-5.4,4.6,0)]
            bottoms=[(x,y+430) for x,y in corners]
            ld.polygon(corners,fill=(56,133,177,int(19*op)))
            for a,b in [(0,1),(1,2),(2,3),(3,0)]:
                ld.polygon([corners[a],corners[b],bottoms[b],bottoms[a]],fill=(45,110,155,int(10*op)))
                ld.line([corners[a],corners[b]],fill=(99,180,224,int(95*op)),width=2)
                ld.line([bottoms[a],bottoms[b]],fill=(99,180,224,int(95*op)),width=2)
                ld.line([corners[a],bottoms[a]],fill=(99,180,224,int(70*op)),width=2)
            im.paste(lay,(0,0),lay); d=ImageDraw.Draw(im)
        for k,(x,y,dense) in enumerate(positions):
            a=ramp(t,11.55+(k/86)*.9,13.7+(k/86)*.7)
            p=project(x,y,0,morph); q=project(x,y,1,morph)
            col=color((40,91,119),a)
            line(d,[(p[0],p[1]-58),(q[0],q[1]+8)],col,1)
            for j in range(60):
                z=j/59
                if dense: z=.35+.65*j/59 # Denser central strings; schematic, not engineering geometry.
                px,py=project(x,y,z,morph)
                circle(d,px,py,1.9 if y<0 else 2.3,color((102,198,233) if dense else (61,133,167),a))
        if 15.2<t<19.2:
            near=sorted([(x,y) for x,y,dense in positions if not dense and abs(y+3.464)<.001])
            ax,ay=near[1]; bx,by=near[2]
            p=project(ax,ay,0); q=project(bx,by,0)
            # One standard nearest-neighbor separation, enlarged in a small callout.
            for pt in [p,q]: circle(d,*pt,6,ICE)
            line(d,[p,(p[0],264),(q[0],264),q],ICE,2)
            txt(d,((p[0]+q[0])/2,233),'125 m',33,WHITE,True,anchor='mm')
            txt(d,(111,637),'Standard string spacing',23,MUTED)
        if t>=23.8:
            u=ramp(t,24.2,27.5)
            start=np.array(project(-4,-2,.95)); end=np.array(project(4,2,.05))
            head=start+(end-start)*u
            line(d,[tuple(start),tuple(head)],(122,220,253),3)
            hits=[]
            sensor_xy=np.array([project(x,y,(.35+.65*j/59) if dense else j/59) for x,y,dense in positions for j in range(60)])
            for j in range(14):
                z=j/13
                if z<u:
                    emission=start+(end-start)*z
                    # Offset light collection points from the charged-particle trajectory.
                    desired=emission+np.array([(-1)**j*31,(-1)**(j+1)*23])
                    p=sensor_xy[np.argmin(np.sum((sensor_xy-desired)**2,axis=1))]
                    line(d,[tuple(emission),tuple(p)],(44,105,139),1)
                    strength=.45+.55*math.exp(-((u-z)/.13)**2)
                    hits.append((*p,strength))
            glow(im,hits,7); d=ImageDraw.Draw(im)
            txt(d,(111,625),'From light signals',26,WHITE)
            txt(d,(111,663),'to a particle track',26,WHITE)
            txt(d,(111,727),'Schematic muon event',22,MUTED)
    note='Standard spacing: 17 m / 125 m. DeepCore is denser. Layout and scale are schematic.'
    txt(d,(100,1010),note,22,MUTED)
    txt(d,(1820,1010),'01',22,ICE,anchor='ra')
    return im

def bezier(p,c,q,u):
    return ((1-u)**2*p[0]+2*(1-u)*u*c[0]+u*u*q[0],(1-u)**2*p[1]+2*(1-u)*u*c[1]+u*u*q[1])

def scene2(t):
    im=base('02','From one photon to an electrical signal','Photocathode → dynodes → anode')
    d=ImageDraw.Draw(im)
    # PMT envelope: broad photocathode at left, multiplication section at right.
    outline=[(430,350),(535,305),(675,309),(777,371),(831,405),(1575,405),(1610,435),(1610,734),(1575,765),(831,765),(777,804),(675,865),(535,865),(430,820)]
    lay=Image.new('RGBA',(W,H)); ld=ImageDraw.Draw(lay)
    ld.polygon(outline,fill=(49,103,133,34)); ld.line(outline+[outline[0]],fill=(111,190,227,120),width=3)
    im.paste(lay,(0,0),lay); d=ImageDraw.Draw(im)
    # Curved photosensitive surface, cutaway representation.
    photo=[]
    for y in np.linspace(364,806,90):
        x=429-46*math.sin((y-364)/442*math.pi)
        photo.append((x,y))
    line(d,photo,ICE,8)
    line(d,[(445,368),(445,802)],(36,76,96),1)
    txt(d,(381,876),'Photocathode',28,ICE)
    # Ten schematic dynodes, staggered vertically to show the cascade.
    nodes=[(660+i*83,503 if i%2==0 else 667) for i in range(10)]
    for i,(x,y) in enumerate(nodes):
        active=ramp(t,4.8+i*.12,5.5+i*.12)
        c=tuple(int(mix(a,b,active)) for a,b in zip(LINE,(105,172,199)))
        d.rounded_rectangle((x-24,y-9,x+24,y+9),radius=6,fill=c)
        line(d,[(x-17,y+(-10 if i%2==0 else 10)),(x+17,y+(-10 if i%2==0 else 10))],ICE,2)
        txt(d,(x,y-49 if i%2==0 else y+27),str(i+1),20,MUTED,anchor='ma')
    txt(d,(1010,875),'Dynodes',28,ICE,anchor='ma')
    # Collection electrode and output lead.
    line(d,[(1538,526),(1538,646)],WHITE,8)
    line(d,[(1544,585),(1700,585),(1700,747)],MUTED,3)
    txt(d,(1538,467),'Anode',26,WHITE,anchor='mm')
    if t>=4.8:
        a=ramp(t,4.8,6)
        arrow(d,(685,357),(1445,357),color(ICE,a),2,12)
        txt(d,(1060,326),'Increasing electric potential',24,color(MUTED,a),anchor='mm')
    # Input photon travels to photocathode once, on the narration beat.
    if t<3.0:
        u=ramp(t,.45,2.65); x=mix(155,383,u)
        wave=[(x-95+v*95,585+10*math.sin(v*math.tau*2.5)) for v in np.linspace(0,1,70)]
        line(d,wave,ICE,3); glow(im,[(x,585,1)],7); d=ImageDraw.Draw(im)
        txt(d,(154,485),'Photon',30,ICE)
    if 2.65<t<3.7:
        a=1-clamp((t-2.65)/1.05)
        for k in range(8):
            ang=k*math.tau/8
            arrow(d,(383+16*math.cos(ang),585+16*math.sin(ang)),(383+(30+25*(1-a))*math.cos(ang),585+(30+25*(1-a))*math.sin(ang)),color(ICE,a),2,3)
    # A single photoelectron waits, then accelerates towards the first dynode.
    if 3<=t<11:
        if t<4.2: p=bezier((395,585),(430,552),(476,571),ramp(t,3,4.2))
        elif t<8.3: p=(476,571)
        else:
            u=clamp((t-8.3)/2.7); u=u*u
            p=bezier((476,571),(575,565),nodes[0],u)
            if t>9.2: arrow(d,(497,563),(559,547),MUTED,2,8)
        glow(im,[(*p,1)],6); d=ImageDraw.Draw(im)
        txt(d,(470,625),'e⁻',31,WHITE)
        if t<8.3: txt(d,(105,335),'1 photoelectron',33,WHITE,True)
    # Secondary emission, sampled electron packets (not literal electron counts).
    packet_times=[11+i*.69 for i in range(10)]
    all_glows=[]
    for i,start in enumerate(packet_times):
        p=nodes[i]; q=nodes[i+1] if i<9 else (1538,585)
        if t>start:
            trail=[bezier(p,((p[0]+q[0])/2,(p[1]+q[1])/2),q,u) for u in np.linspace(0,clamp((t-start)/.68),30)]
            line(d,trail,(36,85,114),2)
        if 0<=t-start<=.46:
            strength=1-(t-start)/.46
            circle(d,*p,17+15*(1-strength),None,color(ICE,strength),2)
        n=min(3*(i+1),25)
        for k in range(n):
            u=(t-start-k*.004)/.68
            if 0<=u<=1:
                uu=u*u
                # All emitted particles begin and end on their electrodes.
                spread=(k-(n-1)/2)*2.7
                c=((p[0]+q[0])/2+spread,(p[1]+q[1])/2+spread*.8)
                x,y=bezier(p,c,q,uu)
                all_glows.append((x,y,.9))
    if all_glows: glow(im,all_glows,3)
    d=ImageDraw.Draw(im)
    if 11<=t<17.8:
        txt(d,(103,335),'Electron multiplication',33,WHITE,True)
    if t>=17.8:
        txt(d,(102,320),'10⁶–10⁸',70,WHITE,True)
        txt(d,(105,409),'illustrative gain range',25,ICE)
        txt(d,(105,453),'IceCube PMT: ~10⁷',23,MUTED)
        # Time trace draws progressively, then holds for reading.
        rect=(1635,765,1830,899)
        line(d,[(1635,770),(1635,898),(1830,898)],LINE,2)
        points=[]; progress=ramp(t,18.4,21.8)
        for u in np.linspace(0,progress,max(2,int(210*progress))):
            y=863-82*math.exp(-((u-.4)/.053)**2)+14*math.exp(-((u-.49)/.09)**2)
            points.append((1645+u*175,y))
        line(d,points,ICE,3)
        txt(d,(1734,933),'Output pulse',23,WHITE,anchor='mm')
        if t<19:
            u=ramp(t,17.8,18.8)
            glow(im,[(1538+162*min(1,u*2),585+162*max(0,u*2-1),1)],5)
            d=ImageDraw.Draw(im)
    txt(d,(100,1010),'Schematic cross-section and electron counts; motion is slowed for explanation.',22,MUTED)
    txt(d,(1820,1010),'02',22,ICE,anchor='ra')
    return im

SCENES={'01_icecube_detector_array_legacy':scene1,'02_photomultiplier_legacy':scene2}
def stamp(t):
    ms=round(t*1000); return f'{ms//3600000:02}:{ms//60000%60:02}:{ms//1000%60:02},{ms%1000:03}'
def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    manifest={'version':1,'width':W,'height':H,'fps':FPS,'audio':False,'narration_language':'en','source_context':'Original narration script; not required for rendering','timing':'Editorial cue timings; English narration is not aligned to recorded audio.','palette':{'navy':'#05101F','ice':'#7DDEFF','white':'#E2F5FC'},'scenes':{}}
    for name,cues in CUES.items():
        manifest['scenes'][name]={'duration':cues[-1][1],'cues':[{'start':a,'end':b,'narration':s,'visual':v} for a,b,s,v in cues]}
        (OUT/(name+'.en.srt')).write_text('\n\n'.join(f'{i+1}\n{stamp(a)} --> {stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(cues))+'\n',encoding='utf-8')
    (OUT/'timing.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
def preview():
    for name,fn in SCENES.items():
        ts=[2.8,5.2,9,16.8,22,27.8] if name.startswith('01') else [1.8,3.8,7,10,14.5,22]
        sheet=Image.new('RGB',(1440,900),NAVY); sd=ImageDraw.Draw(sheet)
        for j,t in enumerate(ts):
            im=fn(t); im.save(OUT/f'{name}_{t:04.1f}s.png')
            x=j%2*720; y=j//2*300
            im=im.resize((512,288),Image.Resampling.LANCZOS)
            sheet.paste(im,(x+100,y))
            txt(sd,(x+12,y+12),f'{t:g} s',22,ICE)
        sheet.save(OUT/(name+'_contact.jpg'),quality=92)
        fn(22 if name.startswith('01') else 14.5).save(OUT/(name+'_poster.jpg'),quality=94)
def render(name,scale=1):
    fn=SCENES[name]; duration=CUES[name][-1][1]; width=int(W*scale); height=int(H*scale)
    target=OUT/(name+'.mp4')
    cmd=[ffmpeg(),'-y','-hide_banner','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s',f'{width}x{height}','-r',str(FPS),'-i','-','-an','-c:v','libx264','-preset','fast','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart',str(target)]
    proc=subprocess.Popen(cmd,stdin=subprocess.PIPE)
    start=time.time()
    try:
        for i in range(round(duration*FPS)):
            im=fn(i/FPS)
            if scale!=1: im=im.resize((width,height),Image.Resampling.LANCZOS)
            proc.stdin.write(im.tobytes())
            if i%150==0: print(f'{name}: {i}/{round(duration*FPS)} frames ({time.time()-start:.1f}s)',flush=True)
        proc.stdin.close()
        if proc.wait()!=0: raise RuntimeError('ffmpeg failed')
    except BaseException:
        proc.kill(); raise
    print(f'COMPLETE {target}',flush=True)

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--preview',action='store_true'); p.add_argument('--scene',choices=list(SCENES)); a=p.parse_args()
    metadata()
    if a.preview: preview()
    elif a.scene: render(a.scene)
    else:
        preview()
        for name in SCENES: render(name)
