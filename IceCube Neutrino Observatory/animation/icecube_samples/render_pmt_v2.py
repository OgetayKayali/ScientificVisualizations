"""PMT v2: legible, compact, multiplying electron bunches; native 4K."""
from pathlib import Path
from functools import lru_cache
import argparse, json, math
import numpy as np
from PIL import Image
import motion_common as m
import render as old

OUT=Path(__file__).resolve().parents[2]/'exports'/'pmt_bunches_v2'
NAME='02_pmt_elektron_demetleri'
DURATION=24
BG=None
ICE=m.ICE; WHITE=m.WHITE; GOLD=m.GOLD
NODES=[np.array([610+i*101,352 if i%2==0 else 735],float) for i in range(10)]
ANODE=np.array([1690.,543.])
COUNTS=[4,9,18,32,52,80,116,162,222,300]
START=11.02; INTERVAL=.69; FLIGHT=.63

def bezier(p,c,q,u): return (1-u)**2*p+2*(1-u)*u*c+u*u*q

@lru_cache(None)
def packing(n):
    # Hexagonal packing makes every electron distinct, even at high density.
    pts=[];spacing=7.4
    for row in range(-20,21):
        for col in range(-20,21):
            x=(col+.5*(row%2))*spacing;y=row*spacing*math.sqrt(3)/2
            pts.append((x,y))
    pts=sorted(pts,key=lambda p:(p[0]*p[0]+p[1]*p[1],p[1],p[0]))[:n]
    return np.array(pts,dtype=float)

def packet_positions(stage,t):
    p=NODES[stage];q=NODES[stage+1] if stage<9 else ANODE
    offsets=packing(COUNTS[stage])
    # Emission is short compared with flight: a compact bunch, not a long stream.
    # Delays resolve the longitudinal rows; transverse rows occupy distinct sites
    # on the dynode. None of the trajectories shares a single focal endpoint.
    velocity=abs(q[1]-p[1])/FLIGHT
    delay=(offsets[:,1]-offsets[:,1].min())/(velocity*1.02)
    u=(t-(START+stage*INTERVAL)-delay)/FLIGHT
    result=[]
    for (dx,dy),v in zip(offsets,u):
        if not 0<=v<1:continue
        v=v**1.18
        a=p+np.array([dx,0]);b=q+(np.array([0,dx]) if stage==9 else np.array([dx,0]))
        c=(a+b)/2+np.array([-13 if stage%2==0 else 13,0])
        pos=bezier(a,c,b,v)
        result.append((pos[0],pos[1]))
    return result

def electron(d,x,y,r=2.6):
    # A dark separation rim is crucial; avoid per-electron bloom that merges dots.
    m.circle(d,x,y,r+.65,(8,30,49))
    m.circle(d,x,y,r,ICE)
    m.circle(d,x-.45,y-.45,max(.7,r*.34),WHITE)

def envelope(im,t):
    lay=m.layer();d=m.Draw(lay)
    outline=[(321,221),(426,153),(574,149),(701,188),(796,263),(1713,263),(1747,300),(1747,807),(1713,854),(796,854),(701,905),(574,934),(426,931),(321,852)]
    d.polygon(outline,fill=(35,86,117,45))
    m.line(d,outline+[outline[0]],(103,177,213,155),2)
    m.composite(im,lay);d=m.Draw(im)
    # Curved photocathode on the entrance window.
    pts=[(321-54*math.sin((y-230)/615*math.pi),y) for y in np.linspace(230,845,100)]
    m.line(d,pts,ICE,7)
    m.line(d,[(335,235),(335,843)],(38,78,104),1)
    for i,(x,y) in enumerate(NODES):
        a=m.ramp(t,4.8+i*.095,5.5+i*.095)
        col=tuple(int(m.mix(c,b,a)) for c,b in zip((46,89,116),(110,170,196)))
        # Widen late dynodes to receive the bigger resolved bunches.
        half=31+min(43,math.sqrt(COUNTS[i])*2.55)
        d.rounded_rectangle((x-half,y-11,x+half,y+11),radius=8,fill=col)
        edge=y+11 if i%2==0 else y-11
        m.line(d,[(x-half+7,edge),(x+half-7,edge)],(150,223,246),2)
    m.line(d,[(ANODE[0],457),(ANODE[0],630)],WHITE,8)
    m.line(d,[(1695,543),(1840,543),(1840,896)],(100,153,183),3)
    d.text((120,85),'PMT',54,ICE)
    # Universal voltage notation appears on the corresponding narration beat.
    show=m.ramp(t,4.8,5.55)*(1-m.ramp(t,10.8,12))
    if show>0:
        col=tuple(int(m.mix(a,b,show)) for a,b in zip((10,27,43),ICE))
        m.line(d,[(792,185),(1640,185)],col,2)
        d.polygon([(1640,185),(1624,178),(1624,192)],fill=col)
        d.text((1205,145),'ΔV',33,col,anchor='mm')

def frame(t):
    im=BG.copy();envelope(im,t);d=m.Draw(im)
    if t<2.78:
        u=m.ramp(t,.4,2.65);x=m.mix(130,267,u)
        wave=[(x-78+78*v,540+9*math.sin(v*math.tau*2.5)) for v in np.linspace(0,1,70)]
        m.line(d,wave,GOLD,3);m.circle(d,x,540,6,GOLD);m.circle(d,x,540,2.5,WHITE)
        d.text((x-13,478),'γ',42,GOLD)
    if 2.65<t<3.4:
        a=1-m.clamp((t-2.65)/.75)
        m.circle(d,267,540,15+(1-a)*32,None,tuple(int(m.mix(v,w,a)) for v,w in zip((30,61,82),GOLD)),2)
    if 3<=t<START:
        if t<4.25:
            pos=bezier(np.array([276.,540.]),np.array([374.,514.]),np.array([455.,526.]),m.ramp(t,3,4.25))
        elif t<8.3:pos=np.array([455.,526.])
        else:
            u=m.clamp((t-8.3)/(START-8.3))**1.65
            pos=bezier(np.array([455.,526.]),np.array([548.,473.]),NODES[0],u)
        electron(d,*pos,5.5)
        d.text((pos[0]+15,pos[1]+18),'e⁻',35,WHITE)
    # Secondary packets grow from four to 300 visible electrons. More than one
    # packet may briefly coexist, because early secondaries already move onward.
    for i,n in enumerate(COUNTS):
        start=START+i*INTERVAL
        if not start-.03<t<start+1.2:continue
        pts=packet_positions(i,t)
        # Resolve even the short emission/collection moments: adjacent bright
        # discs retain a visible gap instead of merging into a white patch.
        radius=2.6 if n>=32 else 3.2
        if len(pts)>1:
            xy=np.array(pts)
            distances=np.sqrt(((xy[:,None,:]-xy[None,:,:])**2).sum(-1))
            np.fill_diagonal(distances,np.inf)
            radii=np.minimum(radius,distances.min(axis=1)*.37)
        else:radii=[radius]*len(pts)
        for (x,y),r in zip(pts,radii):electron(d,x,y,float(r))
        # A subtle electrode flash, separate from the sharp electron points.
        if 0<t-start<.3:
            strength=1-(t-start)/.3
            x,y=NODES[i]
            half=31+min(43,math.sqrt(n)*2.55)
            m.line(d,[(x-half+7,y),(x+half-7,y)],tuple(int(m.mix(c,v,strength)) for c,v in zip((110,170,196),WHITE)),3)
    if t>=18.0:
        a=m.ramp(t,18,18.65)
        d.text((1060,170),'×10⁶–10⁸',58,tuple(int(m.mix(c,v,a)) for c,v in zip((10,27,43),WHITE)),anchor='mm')
        # Collect charge at the anode, then send the electrical pulse down the lead.
        if t<18.85:
            strength=math.exp(-((t-18.32)/.23)**2)
            m.line(d,[(1690,457),(1690,630)],tuple(int(m.mix(c,v,strength)) for c,v in zip(WHITE,ICE)),10)
        if 18.35<t<19.7:
            u=m.ramp(t,18.35,19.7)
            distance=u*(145+353)
            if distance<145:pos=(1695+distance,543)
            else:pos=(1840,543+distance-145)
            m.glow(im,[(*pos,1)],5);d=m.Draw(im)
        # The waveform sits entirely below the tube, with generous separation.
        progress=m.ramp(t,19.3,22)
        trace=[]
        for u in np.linspace(0,progress,max(2,round(progress*180))):
            y=1008-70*math.exp(-((u-.46)/.047)**2)+9*math.exp(-((u-.55)/.07)**2)
            trace.append((1530+u*325,y))
        m.line(d,trace,ICE,3)
    return im

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    cues=old.CUES['02_pmt_foton_sinyal']
    visual=['γ fotokatoda gelir; tek e⁻ çıkar.','ΔV ve dinotların aydınlanması.','Tek elektron ilk dinoda hızlanarak ulaşır.','Sıkı ve ayrışmış demetler halinde elektron çoğalması.','Anotta yük toplanması, kazanç ve çıkış darbesi.']
    (OUT/'timing.json').write_text(json.dumps(dict(version=2,width=round(1920*m.S),height=round(1080*m.S),duration=24,fps=30,audio=False,symbols=['PMT','γ','e⁻','ΔV','×10⁶–10⁸'],representative_electrons_per_stage=COUNTS,cues=[dict(start=a,end=b,narration=s,visual=v) for (a,b,s,_),v in zip(cues,visual)]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.srt')).write_text('\n\n'.join(f'{i+1}\n{old.stamp(a)} --> {old.stamp(b)}\n{s}' for i,(a,b,s,_) in enumerate(cues))+'\n',encoding='utf-8')

def previews():
    times=[2,4,7,10.1,11.36,13.45,15.55,16.98,17.62,21.8]
    sheet=Image.new('RGB',(1920,864))
    for i,t in enumerate(times):
        im=frame(t);im.save(OUT/f'kare_{t:05.2f}.jpg',quality=95)
        sheet.paste(im.resize((384,216),Image.Resampling.LANCZOS),(i%5*384,i//5*432))
    contact=Image.new('RGB',(1920,432))
    contact.paste(sheet.crop((0,0,1920,216)),(0,0));contact.paste(sheet.crop((0,432,1920,648)),(0,216))
    contact.save(OUT/'kontrol.jpg',quality=95)
    frame(17.62).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',choices=[1,2],type=int,default=2);p.add_argument('--preview',action='store_true');args=p.parse_args()
    m.configure(args.scale);BG=m.bg();metadata();previews()
    if not args.preview:
        m.encode(frame,OUT/(NAME+('_4k' if args.scale==2 else '_1080p')+'.mp4'),DURATION,OUT/(NAME+'_onizleme_1080p.mp4') if args.scale==2 else None)
