# Copyright (c) 2026 Ogetay Kayali. See the repository LICENSE.md.
"""Narrated comparison of vacuum light, light in water, and a massive particle."""
from pathlib import Path
import argparse, json, math
from PIL import Image
import motion_common as m
import legacy_rendering_helpers as old

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'exports'/'05_light_speed_in_vacuum_and_water'
NAME='05_light_speed_in_vacuum_and_water'
DURATION=40
BG=None
ICE=m.ICE; WHITE=m.WHITE; MU=(167,187,255)
CUES=[
 (0,5,'No particle can travel faster than light in a vacuum.','Light and a muon start together; the muon stays behind the light front.'),
 (5,9.5,'That speed is approximately 300,000 kilometers per second.','The comparison holds beside c ≈ 300 000 km/s.'),
 (9.5,14.2,'But light travels more slowly inside a material.','Water appears in the lower row as two light markers advance together.'),
 (14.2,21,'In water, for example, visible light travels about 25 percent more slowly than in a vacuum.','The lower light marker slows to about 0.75c after entering water; the upper marker keeps its speed.'),
 (21,26,'Remember: the physical speed limit is the speed of light in a vacuum.','The vacuum reference c is highlighted while the shorter path in water remains visible.'),
 (26,40,'A particle can therefore travel slower than light in a vacuum, yet faster than light in a material.','Three markers start together: the muon stays ahead of light in water but behind light in a vacuum.'),
]

def col(c,a=1):
    return tuple(round(m.mix(b,v,m.clamp(a))) for b,v in zip((9,27,45),c))

def text(d,x,y,s,size=40,c=WHITE,a=1):
    if a>.001:d.text((x,y),s,size,col(c,a),anchor='mm')

def dashed(d,x,y0,y1,c,width=1):
    for y in range(round(y0),round(y1),17):m.line(d,[(x,y),(x,min(y+7,y1))],c,width)

def photon(d,x,y,a=1,water=False):
    # A localized optical pulse, not an absorption/re-emission random walk.
    wave=[]; wavelength=22*(.75 if water else 1)
    for j in range(111):
        dx=-55+j
        envelope=math.exp(-(dx/28)**2)
        wave.append((x+dx,y+math.sin(dx*math.tau/wavelength)*15*envelope))
    m.line(d,wave,col(ICE,a*.3),7)
    m.line(d,wave,col(WHITE,a),2)
    m.circle(d,x,y,3.5,col(WHITE,a))

def particle(d,x,y,a=1):
    for r,alpha in [(18,.06),(12,.15),(7,1)]:m.circle(d,x,y,r,col(MU,a*alpha))
    m.circle(d,x-1,y-1,2.4,col(WHITE,a))

def track(d,x0,x,y,c,a=1):
    m.line(d,[(x0,y),(x,y)],col(c,a*.4),2)
    m.circle(d,x0,y,4,None,col(c,a*.55),1)

def droplet(d,x,y,a=1):
    pts=[]
    curves=[((0,-25),(-5,-10),(-20,0),(-18,11)),((-18,11),(-17,30),(17,30),(18,11)),((18,11),(20,0),(5,-10),(0,-25))]
    for p0,p1,p2,p3 in curves:
        for i in range(21):
            u=i/20;v=1-u
            pts.append((x+v**3*p0[0]+3*v*v*u*p1[0]+3*v*u*u*p2[0]+u**3*p3[0],y+v**3*p0[1]+3*v*v*u*p1[1]+3*v*u*u*p2[1]+u**3*p3[1]))
    d.polygon(pts,col((28,79,111),a),col(ICE,a*.7),1)

def water(d,t):
    show=m.ramp(t,9.5,11)
    if show<=0:return
    expand=m.ramp(t,25.5,28.1)
    left=m.mix(970,310,expand);top=m.mix(570,485,expand);bottom=m.mix(865,910,expand)
    d.rounded_rectangle((left,top,1760,bottom),18,col((13,44,65),show),col((53,114,149),show*.65),1)
    # Flat left boundary: normal incidence, hence no direction change.
    m.line(d,[(left,top+18),(left,bottom-18)],col(ICE,show*.45),2)
    surface=[(x,top+7+math.sin(x*.03+t*.6)*2.5) for x in range(math.ceil(left+20),1741,6)]
    m.line(d,surface,col(ICE,show*.28),1)
    for row in range(4):
        for j in range(13):
            x=345+j*108+(row%2)*25;y=535+row*95
            if left+30<x<1715 and top+30<y<bottom-20:
                alpha=show*.2*m.clamp((x-left-20)/45)
                angle=.6*math.sin(j*1.9+row)
                m.circle(d,x,y,5,None,col(ICE,alpha),1)
                for a in [angle+.8,angle+2.6]:
                    hx=x+11*math.cos(a);hy=y+11*math.sin(a)
                    m.line(d,[(x,y),(hx,hy)],col(ICE,alpha),1)
                    m.circle(d,hx,hy,2,col(ICE,alpha))
    droplet(d,left+39,top+47,show)
    text(d,left+94,top+47,'H₂O',29,ICE,show*.85)

def guide(d,x,ys,a=1):
    dashed(d,x,ys[0]-45,ys[-1]+48,col(WHITE,a*.22))
    for y in ys:m.line(d,[(x-7,y+42),(x+7,y+42)],col(WHITE,a*.45),1)

def first(d,t):
    a=m.ramp(t,0,.75)*(1-m.ramp(t,9.1,10.3))
    if a<=0:return
    u=m.clamp((t-1)/6.1)
    x=400+1160*u;xm=400+1160*u*.9
    guide(d,x,[320,710],a)
    text(d,260,320,'γ',49,WHITE,a)
    text(d,260,710,'μ⁻',49,MU,a)
    track(d,400,x,320,WHITE,a);track(d,400,xm,710,MU,a)
    photon(d,x,320,a);particle(d,xm,710,a)
    text(d,x,246,'c',40,WHITE,a)
    if t>=3.5:text(d,xm,783,'v < c',33,MU,a*m.ramp(t,3.5,4.5))

def second(d,t):
    a=m.ramp(t,10.2,11.3)*(1-m.ramp(t,26,27.6))
    if a<=0:return
    # Equal speed before entry, v_water / c = 0.75 after x=970.
    elapsed=max(0,min(t-11.5,9))
    distance=elapsed*(1200/9)
    x=380+distance
    xw=380+min(distance,590)+max(0,distance-590)*.75
    ywater=m.mix(715,790,m.ramp(t,25.5,28.1))
    guide(d,x,[320,ywater],a)
    text(d,260,320,'γ',49,WHITE,a);text(d,260,ywater,'γ',49,ICE,a)
    track(d,380,x,320,WHITE,a);track(d,380,xw,ywater,ICE,a)
    photon(d,x,320,a);photon(d,xw,ywater,a,xw>=970)
    text(d,x,245,'c',40,WHITE,a)
    if t>17:
        text(d,xw,ywater+78,'≈ 0.75c',36,ICE,a*m.ramp(t,17,18.2))
    if t>18.3:
        b=a*m.ramp(t,18.3,19.5)
        text(d,1165,455,'−25%',39,ICE,b)
        # Bracket explicitly compares the two distances after water entry.
        for yy,end in [(391,x),(ywater+34,xw)]:
            m.line(d,[(970,yy),(end,yy)],col(ICE,b*.6),1)
            for xx in [970,end]:m.line(d,[(xx,yy-5),(xx,yy+5)],col(ICE,b*.6),1)
    if 21<t<26:
        pulse=math.sin(math.pi*m.clamp((t-21)/5))**2
        m.circle(d,x,320,35+8*pulse,None,col(WHITE,a*.4*pulse),2)

def final_race(d,t):
    a=m.ramp(t,27.3,28.7)
    if a<=0:return
    u=m.clamp((t-29.3)/8)
    x0=400;distance=1160*u
    xc=x0+distance;xm=x0+distance*.90;xw=x0+distance*.75
    guide(d,xc,[320,600,790],a)
    for y,s,c in [(320,'γ',WHITE),(600,'μ⁻',MU),(790,'γ',ICE)]:text(d,260,y,s,49,c,a)
    for y,x,c in [(320,xc,WHITE),(600,xm,MU),(790,xw,ICE)]:track(d,x0,x,y,c,a)
    photon(d,xc,320,a);particle(d,xm,600,a);photon(d,xw,790,a,True)
    text(d,xc,245,'c',40,WHITE,a)
    text(d,xm,655,'v',34,MU,a)
    text(d,xw,855,'≈ 0.75c',34,ICE,a)
    # A moving c reference remains outside the water, aligned vertically.
    if t>=33:
        b=m.ramp(t,33,36)
        text(d,960,991,'0.75c < v(μ⁻) < c',43,WHITE,b)

def frame(t):
    im=BG.copy();d=m.Draw(im)
    water(d,t)
    # Universal unit notation, with no language-dependent explanatory labels.
    show=m.ramp(t,5,6)*(1-m.ramp(t,9.1,10.4))
    recall=m.ramp(t,21,22)*(1-m.ramp(t,25.4,26.8))
    text(d,960,120,'c ≈ 300 000 km/s',49,WHITE,max(show,recall))
    first(d,t);second(d,t);final_race(d,t)
    return im

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'timing.json').write_text(json.dumps(dict(duration=DURATION,fps=30,width=round(m.W*m.S),height=round(m.H*m.S),audio=False,narration_language="en",timing='Estimated narration; not synchronized to a voice recording.',ratios=dict(vacuum_light=1,water_light=.75,illustrative_muon=.90),cues=[dict(start=a,end=b,narration=s,visual=v) for a,b,s,v in CUES]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.en.srt')).write_text('\n\n'.join(f'{i+1}\n{old.stamp(a)} --> {old.stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(CUES))+'\n',encoding='utf-8')

def previews():
    times=[2.5,6.8,11.8,15,17,20.5,23,27.5,29.3,32.5,35,38.5]
    sheet=Image.new('RGB',(1920,1440))
    for i,t in enumerate(times):
        im=frame(t);im.save(OUT/f'frame_{t:05.1f}.jpg',quality=94)
        sheet.paste(im.resize((640,360),Image.Resampling.LANCZOS),(i%3*640,i//3*360))
    sheet.save(OUT/'contact_sheet.jpg',quality=94)
    frame(38.5).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',type=int,choices=[1,2],default=2);p.add_argument('--preview',action='store_true');args=p.parse_args()
    m.configure(args.scale);BG=m.bg();metadata();previews()
    if not args.preview:m.encode(frame,OUT/(NAME+('_4k' if args.scale==2 else '_1080p')+'.mp4'),DURATION,OUT/(NAME+'_1080p.mp4') if args.scale==2 else None)
