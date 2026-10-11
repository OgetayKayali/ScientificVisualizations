# Copyright (c) 2026 Ogetay Kayali. See the repository LICENSE.md.
"""IceCube history: a language-independent, narrated 70-second sequence."""
from pathlib import Path
from functools import lru_cache
import argparse, math, json
import numpy as np
from PIL import Image
import motion_common as m
import legacy_rendering_helpers as old
from render_neutrino_direction_and_ice_calibration import sensor, line, dot, txt, col, arrow, dashed, vec

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'exports'/'08_icecube_construction_and_discovery_history'
NAME='08_icecube_construction_and_discovery_history'
DURATION=70
BG=None
ICE=m.ICE; WHITE=m.WHITE; MU=(167,187,255); GOLD=(248,207,142)
CUES=[
 (0,4.5,'The concept behind the project dates back to before the 1990s.','1987: a detector sketch in the ice and an idea symbol appear.'),
 (4.5,9.5,'Early tests were carried out in Greenland.','1990: a schematic Greenland map, test borehole, and optical sensors appear.'),
 (9.5,18,'In 1999, IceCube was formally proposed to the United States National Science Foundation, or NSF.','1999: a detector drawing becomes a proposal document and moves toward the NSF symbol.'),
 (18,22,'Construction began around 2004,','2004: hot-water drilling advances through an ice cross-section.'),
 (22,28,'and the first detector string was installed in 2005.','2005: the first sensor string is lowered into a borehole.'),
 (28,32.5,'Two neutrino events were detected with that first string.','Two separate upward-going muon tracks are recorded beside the first string.'),
 (32.5,38,"IceCube's construction was completed in 2010.",'The camera expands from one string to the full array as the final strings are added.'),
 (38,44,'Evidence for astrophysical neutrinos was announced in 2013.','The array becomes a publication page with dots representing the 28-event sample.'),
 (44,53,'Two exceptionally energetic neutrinos had already been detected in 2011 and 2012.','The timeline moves backward to two separate cascade events in 2011 and 2012.'),
 (53,57,'They were nicknamed Bert and Ernie.','The two events remain side by side with their internationally used names.'),
 (57,70,'Those two events alone, however, were not enough to establish a distant cosmic origin conclusively.','The records connect to atmospheric and distant-space possibilities, with question marks indicating uncertainty.'),
]
YEARS=[1987,1990,1999,2004,2005,2010,2011,2012,2013]

def mixcol(a,b,u):return tuple(round(m.mix(x,y,u)) for x,y in zip(a,b))
def timeline(d,t):
    keys=[(0,0),(4,0),(5.5,1),(9,1),(11,2),(17.5,2),(19,3),(21.8,3),(23,4),(32.5,4),(36,5),(38,5),(40,8),(43.5,8),(46,6),(48.5,6),(51,7),(70,7)]
    active=keys[-1][1]
    for (a,x),(b,y) in zip(keys,keys[1:]):
        if a<=t<b:active=m.mix(x,y,m.ramp(t,a,b));break
    x0=290;step=167.5;y=983
    line(d,[(x0,y),(x0+step*8,y)],ICE,1,.22)
    for i,year in enumerate(YEARS):
        a=.28+.72*max(0,1-abs(i-active))
        dot(d,(x0+step*i,y),3,ICE,a)
        txt(d,x0+step*i,1021,str(year),23,ICE,a)
    dot(d,(x0+step*active,y),8,ICE,.22)
    dot(d,(x0+step*active,y),3.5,WHITE)

def date(d,s,a=1,x=960,y=128,size=83):txt(d,x,y,s,size,WHITE,a)
def ice_surface(d,y=351,a=1,left=260,right=1660):
    pts=[(x,y+3*math.sin(x/115)) for x in range(left,right+1,9)]
    line(d,pts,ICE,2,a*.75)
    for yy in [y+105,y+265,y+453]:
        line(d,[(x,yy+9*math.sin(x/220+yy)) for x in range(left,right+1,13)],ICE,1,a*.13)

def blueprint(d,center=(760,535),scale=1,a=1,progress=1):
    cx,cy=center
    for i in range(7):
        x=cx+(i-3)*46*scale
        top=cy-155*scale+(i%3-1)*16*scale
        end=top+320*scale
        line(d,[(x,top),(x,m.mix(top,end,progress))],ICE,1,a*.45)
        for j in range(15):
            u=j/14
            if u<=progress:dot(d,(x,top+u*320*scale),2.6*scale,ICE,a*.75)
    if progress>.6:
        arrow(d,(cx-220*scale,cy+120*scale),(cx+230*scale,cy-110*scale),MU,2,a*m.ramp(progress,.6,1),11*scale)

def paper(d,cx,cy,s=1,a=1):
    pts=[(cx-182*s,cy-230*s),(cx+125*s,cy-230*s),(cx+182*s,cy-173*s),(cx+182*s,cy+230*s),(cx-182*s,cy+230*s),(cx-182*s,cy-230*s)]
    line(d,pts,ICE,2,a)
    line(d,[(cx+125*s,cy-230*s),(cx+125*s,cy-173*s),(cx+182*s,cy-173*s)],ICE,2,a*.65)

def idea(t):
    im=BG.copy();d=m.Draw(im);date(d,'1987')
    blueprint(d,(810,550),1.28,1,m.ramp(t,.3,3.2))
    a=m.ramp(t,1.7,3.6);x=1390;y=455;r=62
    m.circle(d,x,y,r,None,col(ICE,a),3)
    line(d,[(x-39,y+48),(x-30,y+105),(x+30,y+105),(x+39,y+48)],ICE,3,a)
    for yy in [y+117,y+130]:line(d,[(x-28,yy),(x+28,yy)],ICE,3,a)
    line(d,[(x-15,y+104),(x-15,y+24),(x,y+9),(x+15,y+24),(x+15,y+104)],ICE,2,a*.75)
    for j in range(7):
        th=math.pi+j*math.pi/6
        line(d,[(x+82*math.cos(th),y+82*math.sin(th)),(x+99*math.cos(th),y+99*math.sin(th))],ICE,2,a)
    return im

# Deliberately schematic outline, not a geographic basemap.
GREENLAND=[(-.08,-1),(.25,-.96),(.45,-.8),(.6,-.72),(.56,-.5),(.67,-.34),(.49,-.12),(.42,.08),(.31,.35),(.19,.52),(.08,.82),(-.03,1),(-.19,.73),(-.34,.57),(-.4,.31),(-.56,.11),(-.46,-.09),(-.57,-.31),(-.47,-.59),(-.29,-.85)]
def greenland(t):
    im=BG.copy();d=m.Draw(im);date(d,'1990')
    pts=[(535+x*215,532+y*265) for x,y in GREENLAND]
    d.polygon(pts,col(ICE,.13));line(d,pts+[pts[0]],ICE,2,.82)
    for off in [30,60,90]:
        inner=[(535+(x*215)*(.95-off/500),532+y*(265-off)) for x,y in GREENLAND]
        line(d,inner+[inner[0]],ICE,1,.13)
    p=(533,452);pulse=(t-4.5)%1.9
    dot(d,p,5,WHITE)
    m.circle(d,*p,12+pulse*13,None,col(ICE,(1-pulse/1.9)*.65),2)
    dashed(d,(580,460),(1020,460),ICE,.4)
    ice_surface(d,350,1,1030,1640)
    for x,j in [(1220,0),(1450,1)]:
        yy=m.mix(390,620+j*65,m.ramp(t,5.1,7.2))
        line(d,[(x,320),(x,yy)],ICE,2,.65)
        sensor(im,(x,yy),31)
    d=m.Draw(im)
    u=m.clamp((t-7.2)/1.8)
    if t>7.2:
        p=vec((1255,620),(1415,685),u)
        line(d,[(1255,620),p],ICE,2,.65);dot(d,p,4,WHITE)
        m.circle(d,1450,685,45,None,col(ICE,m.ramp(t,8.65,9)),2)
    return im

def proposal(t):
    im=BG.copy();d=m.Draw(im);date(d,'1999')
    u=m.ramp(t,11.7,15.7)
    cx=m.mix(645,1040,u);cy=535;s=m.mix(1,.66,u)
    paper(d,cx,cy,s);blueprint(d,(cx,cy),.60*s,1,1)
    # A neutral agency emblem; NSF is the only institutional text.
    x=1400;y=535;a=m.ramp(t,10,11.5)
    for j in range(20):
        th=j*math.tau/20
        line(d,[(x+106*math.cos(th),y+106*math.sin(th)),(x+120*math.cos(th),y+120*math.sin(th))],ICE,3,a*.65)
    m.circle(d,x,y,99,None,col(ICE,a),2)
    txt(d,x,y,'NSF',53,WHITE,a)
    arrow(d,(1090,535),(1240,535),ICE,2,m.ramp(t,14.5,16.5),12)
    # Submission is represented as delivery, not an approval stamp.
    return im

def rig(d,x,y,a=1):
    line(d,[(x-81,y),(x-32,y-137),(x+32,y-137),(x+81,y)],ICE,3,a)
    line(d,[(x-52,y-65),(x+52,y-65),(x-32,y-137),(x+81,y)],ICE,2,a*.65)
    m.circle(d,x,y-125,14,None,col(ICE,a),2)
    d.rounded_rectangle((x+130,y-88,x+263,y-12),9,None,col(ICE,a*.65),2)
    for j in range(4):line(d,[(x+146,y-71+j*14),(x+243,y-71+j*14)],ICE,1,a*.35)
    line(d,[(x+130,y-27),(x+95,y-27),(x+95,y-115),(x,y-115)],ICE,3,a*.6)

def install(t):
    im=BG.copy();d=m.Draw(im)
    date(d,'2004' if t<22 else '2005')
    ice_surface(d,353,1,260,1660);rig(d,960,350)
    drill=m.ramp(t,18.3,21.7);bottom=m.mix(364,832,drill)
    line(d,[(960,354),(960,bottom)],ICE,29,.06)
    line(d,[(946,355),(946,bottom),(974,bottom),(974,355)],ICE,1,.4)
    if t<22.5:
        a=1-m.ramp(t,21.7,22.5)
        line(d,[(960,228),(960,bottom)],ICE,3,a*.7)
        for j in range(9):
            th=j*math.tau/9
            rr=9+7*((t*1.6+j*.12)%1)
            dot(d,(960+rr*math.cos(th),bottom+rr*math.sin(th)),2,ICE,a*.55)
    else:
        deploy=m.ramp(t,22.5,27.5)
        # All 60 DOMs enter the borehole as one cable, shown at schematic spacing.
        line(d,[(960,229),(960,360+462*deploy)],ICE,2,.75)
        for j in range(60):
            yy=822-j*5.8-(1-deploy)*470
            if yy>360:dot(d,(960,yy),2.7,ICE,.82)
        if 22.5<t<27.5:
            yy=360+462*deploy
            sensor(im,(960,yy),14)
    d=m.Draw(im)
    for idx,start in enumerate([28.1,30.1]):
        age=t-start
        if age<0:continue
        u=m.clamp(age/1.55);x=922-idx*20;head=vec((x,842),(x-6,460),u)
        a=1-m.ramp(age,1.65,2.15)
        line(d,[(x,842),head],MU,2,a*.8)
        if u<1:dot(d,head,4,MU,a)
        for j in range(60):
            yy=822-j*5.8
            if yy>=head[1]+12 and (j*17+idx*11)%60<(35 if idx==0 else 50):
                dot(d,(960,yy),4,ICE,a*.95)
        # Two stored event thumbnails make the separate detections explicit.
        a=m.ramp(age,1,1.6)
        xx=1340+idx*140
        m.circle(d,xx,655,45,None,col(ICE,a*.35),1)
        arrow(d,(xx,685),(xx,625),MU,2,a,8)
        txt(d,xx,735,str(idx+1),26,ICE,a)
    return im

def array_projection(x,y,z):
    xx=x*math.cos(.34)+y*math.sin(.34);yy=-x*math.sin(.34)+y*math.cos(.34)
    return np.array([960+xx*103,370+yy*21+z*401])
ARRAY_POINTS=[]
for i,(x,y,dense) in enumerate(old.positions):
    for j in range(60):
        z=.35+.65*j/59 if dense else j/59
        ARRAY_POINTS.append((i,array_projection(x,y,z),j))
def array_art(d,progress=1,center=(960,550),scale=1,a=1):
    cx,cy=center
    def tr(p):return ((p[0]-960)*scale+cx,(p[1]-550)*scale+cy)
    for i,(x,y,dense) in enumerate(old.positions):
        f=m.clamp(progress*86-i)
        if f<=0:continue
        p=array_projection(x,y,0);q=array_projection(x,y,f)
        line(d,[tr(p),tr(q)],ICE,1,a*.27)
    for i,p,j in ARRAY_POINTS:
        f=m.clamp(progress*86-i)
        if j/59<=f and f>0:dot(d,tr(p),max(1,2.25*scale),ICE,a*.68)

def completion(t):
    im=BG.copy();d=m.Draw(im);date(d,'2010')
    u=m.ramp(t,32.5,36.9)
    # The single cable remains visible while the array expands around it.
    a=1-m.ramp(t,32.5,35)
    line(d,[(960,340),(960,825)],ICE,2,a)
    for j in range(60):dot(d,(960,475+j*5.8),2.7,ICE,a)
    array_art(d,progress=.015+.985*u,a=m.ramp(t,32.5,34))
    if t>36.5:
        a=m.ramp(t,36.5,37.5)
        line(d,[(1480,817),(1500,838),(1544,788)],ICE,4,a)
    return im

def publication(t):
    im=BG.copy();d=m.Draw(im);date(d,'2013')
    u=m.ramp(t,38,40)
    paper(d,960,543,1.22,u)
    array_art(d,center=(960,m.mix(550,439,u)),scale=m.mix(1,.36,u),a=1-.18*u)
    # 28-event sample, not 28 individually certain cosmic identifications.
    for j in range(28):
        a=m.ramp(t,39.8+j*.065,40.4+j*.065)
        dot(d,(960+(j%7-3)*49,645+(j//7)*31),5.5,MU if j<2 else ICE,a)
    return im

_rng=np.random.default_rng(20112012)
CASCADE=[]
for k in range(2):
    pts=[]
    for i in range(11):
        for j in range(19):
            x=(i-5)*32;y=(j-9)*19+12*math.sin(i*1.7)
            distance=math.hypot(x-(k*2-1)*12,y-(k*2-1)*18)
            if _rng.random()<math.exp(-(distance/110)**2):
                pts.append((x/160,y/160,_rng.uniform(.4,1)))
    CASCADE.append(pts)

def cascade(d,c,scale,k,age,a=1):
    cx,cy=c
    # Faint instrument strings keep a cascade distinct from a generic explosion.
    for i in range(11):
        x=cx+(i-5)*32*scale;offset=12*math.sin(i*1.7)
        line(d,[(x,cy+(-185+offset)*scale),(x,cy+(185+offset)*scale)],ICE,1,a*.16)
        for j in range(19):dot(d,(x,cy+((j-9)*19+offset)*scale),1.9*scale,ICE,a*.25)
    grow=m.ramp(age,0,2.25)
    for x,y,w in CASCADE[k]:
        r=math.hypot(x,y)
        if grow<r/1.4:continue
        arrival=r/1.4*2.25
        pulse=math.exp(-((age-arrival)/.32)**2)
        p=(cx+x*160*scale,cy+y*160*scale)
        color=mixcol(WHITE,MU if k else ICE,min(1,r))
        dot(d,p,(2.2+4.7*w*(1-r/1.7)+pulse*2)*scale,color,a*(.5+.5*w))
    if 0<age<2.6:
        m.circle(d,cx,cy,20+grow*220*scale,None,col(ICE,a*(1-grow)*.4),2)

def pair(t):
    im=BG.copy();d=m.Draw(im)
    a1=m.ramp(t,44,45);a2=m.ramp(t,48.3,49.4)
    date(d,'2011',a1,565,165,67);date(d,'2012',a2,1355,165,67)
    cascade(d,(565,550),1.27,0,t-45.1,a1)
    cascade(d,(1355,550),1.27,1,t-49.4,a2)
    # Names are proper names and remain understandable across dubbing languages.
    names=m.ramp(t,53,54.5)
    txt(d,565,838,'Bert',42,ICE,names)
    txt(d,1355,838,'Ernie',42,MU,names)
    return im

def galaxy(d,c,r,a,t=0):
    cx,cy=c
    for arm in [0,math.pi]:
        pts=[]
        for j in range(151):
            u=j/150;th=arm+u*math.pi*3.25+t*.025
            pts.append((cx+math.cos(th)*r*u,cy+math.sin(th)*r*u*.48))
        line(d,pts,ICE,2,a*.7)
    dot(d,c,13,ICE,a*.22);dot(d,c,6,WHITE,a)
    for x,y in [(-.9,-.55),(.82,-.7),(.65,.6),(-.7,.7)]:
        xx=cx+x*r;yy=cy+y*r
        line(d,[(xx-5,yy),(xx+5,yy)],WHITE,1,a*.65)
        line(d,[(xx,yy-5),(xx,yy+5)],WHITE,1,a*.65)

def atmosphere(d,c,r,a):
    cx,cy=c
    m.circle(d,cx,cy,r,None,col(ICE,a*.8),2)
    m.circle(d,cx,cy,r+14,None,col(ICE,a*.22),5)
    d.arc((cx-r*.65,cy-r,cx+r*.65,cy+r),0,360,col(ICE,a*.23),1)
    d.ellipse((cx-r,cy-r*.33,cx+r,cy+r*.33),None,col(ICE,a*.23),1)
    root=np.array([cx+28,cy-r-29]);start=np.array([cx+95,cy-r-100])
    arrow(d,start,root,MU,2,a,9)
    for dx in [-45,-10,33]:
        line(d,[root,(cx+dx,cy-r+10)],MU,2,a*.7)

def uncertainty(t):
    im=BG.copy();d=m.Draw(im)
    date(d,'2011 · 2012')
    u=m.ramp(t,57,60.5)
    for k in range(2):
        c=vec((565 if k==0 else 1355,550),(850+k*220,740),u)
        cascade(d,c,m.mix(1.27,.40,u),k,10,1)
        txt(d,c[0],m.mix(838,857,u),'Bert' if k==0 else 'Ernie',m.mix(42,29,u),ICE if k==0 else MU,1)
    a=m.ramp(t,60,63)
    atmosphere(d,(550,400),104,a)
    galaxy(d,(1390,400),164,a,t)
    # Dashed alternatives are hypotheses, not measured source directions.
    b=m.ramp(t,62.5,65.7)
    for xx in [850,1070]:line(d,[(xx,648),(960,605)],ICE,1,b*.4)
    dot(d,(960,605),4,ICE,b*.75)
    dashed(d,(960,605),(620,518),ICE,b*.55)
    dashed(d,(960,605),(1300,502),ICE,b*.55)
    txt(d,724,550,'?',58,WHITE,b)
    txt(d,1200,550,'?',58,WHITE,b)
    return im

SCENES=[(0,idea),(4.5,greenland),(9.5,proposal),(18,install),(32.5,completion),(38,publication),(44,pair),(57,uncertainty)]
def frame(t):
    idx=max(i for i,(start,f) in enumerate(SCENES) if t>=start)
    start,fn=SCENES[idx]
    if idx and t<start+.9:
        # Hold the outgoing state during the dissolve, avoiding reversed motion.
        im=Image.blend(SCENES[idx-1][1](start),fn(t),m.ramp(t,start,start+.9))
    else:im=fn(t)
    timeline(m.Draw(im),t)
    return im

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'timing.json').write_text(json.dumps(dict(duration=DURATION,fps=30,width=round(1920*m.S),height=round(1080*m.S),audio=False,narration_language="en",paragraph_boundary=32.5,timing='Estimated narration timing; no voice recording supplied',visuals='Schematic; not archival event data',cues=[dict(start=a,end=b,narration=s,visual=v) for a,b,s,v in CUES]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.en.srt')).write_text('\n\n'.join(f'{i+1}\n{old.stamp(a)} --> {old.stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(CUES))+'\n',encoding='utf-8')

def previews():
    times=[3.7,8.5,16.5,20.9,26.5,29.5,31.6,36.8,42.5,47.5,51.5,55.5,59,64,68]
    sheet=Image.new('RGB',(1920,1800))
    for i,t in enumerate(times):
        im=frame(t);im.save(OUT/f'frame_{t:04.1f}.jpg',quality=94)
        thumb=im.resize((640,360),Image.Resampling.LANCZOS)
        sheet.paste(thumb,(i%3*640,i//3*360))
    sheet.save(OUT/'contact_sheet.jpg',quality=93)
    frame(55.5).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',type=int,choices=[1,2],default=2);p.add_argument('--preview',action='store_true');args=p.parse_args()
    m.configure(args.scale);BG=m.bg();metadata();previews()
    if not args.preview:m.encode(frame,OUT/(NAME+('_4k' if args.scale==2 else '_1080p')+'.mp4'),DURATION,OUT/(NAME+'_1080p.mp4') if args.scale==2 else None)
