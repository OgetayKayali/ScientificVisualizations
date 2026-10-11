# Copyright (c) 2026 Ogetay Kayali. See the repository LICENSE.md.
"""Two narration paragraphs joined by an atom-to-plate morph; native 4K."""
from pathlib import Path
import argparse, json, math
import numpy as np
from PIL import Image
import motion_common as m
import legacy_rendering_helpers as old

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'exports'/'04_electron_and_muon_tracks_plate_analogy'
NAME='04_electron_and_muon_tracks_plate_analogy'
DURATION=82
BG=None
ICE=m.ICE; WHITE=m.WHITE; MU=(167,187,255); DIM=(43,85,117)
ROWS=[350,765]
XS=[650+155*i for i in range(7)]
CUES=[
 (0,4.2,'Neutrinos occasionally interact with matter.','Many neutrinos pass through matter; one interaction is selected.'),
 (4.2,7.2,'When they do, other elementary particles can be produced.','The first interaction produces short secondary tracks and an electron.'),
 (7.2,11,'The particles of particular interest here are electrons and muons.','A separate second event produces a muon, establishing two comparison rows.'),
 (11,14.5,'The electron is the familiar negatively charged particle.','The electron and its e⁻ symbol are highlighted.'),
 (14.5,20.2,'A muon is similar to an electron, but about 207 times heavier.','The mass comparison m(μ⁻) ≈ 207 m(e⁻) appears.'),
 (20.2,27.6,'Muons are especially useful for detecting neutrinos.','The upper row shows a short electron shower; the lower row shows a long muon track.'),
 (27.6,36,'At high energies, they can travel much farther through matter than electrons, leaving long tracks.','The long muon track is compared with the short electron shower before the transition.'),
 (36,41.5,'Imagine firing projectiles through a sequence of metal plates.','Atoms smoothly transform into metal plates at the same positions.'),
 (41.5,46,'The plates represent atoms in the surrounding material.','Atom symbols briefly reappear over the plates to establish the analogy.'),
 (46,50.5,'A small 9 mm projectile represents the electron.','A small projectile in the upper row is paired with e⁻ and 9 mm.'),
 (50.5,54,'In this analogy, it passes through the first few plates,','The small projectile enters the first plates.'),
 (54,57.2,'but changes direction as it goes,','The track changes direction after impacts.'),
 (57.2,60.2,'knocking fragments from the plates.','Plate and projectile fragments scatter with the same visual appearance.'),
 (60.2,66.2,'The fragments make it difficult to identify the original projectile and trace where it came from.','The upper row holds a short, scattered track with mixed fragments.'),
 (66.2,72.5,'Now imagine a much larger projectile, such as a .50-caliber round.','A larger schematic projectile in the lower row is paired with μ⁻ and .50.'),
 (72.5,78.7,'In our simplified analogy, it follows a straighter path through more plates.','The larger projectile leaves a straighter track through the array and small holes in the plates.'),
 (78.7,82,'That is our muon.','The projectile transforms into a μ⁻ symbol, and both tracks remain visible.'),
]

def col(c,a=1):return tuple(int(m.mix(b,v,m.clamp(a))) for b,v in zip((10,28,46),c))
def dot(d,x,y,r,c):m.circle(d,x,y,r,c);m.circle(d,x-.6,y-.6,max(.9,r*.3),WHITE)
def symbol(d,x,y,s,size=42,c=WHITE,a=1):d.text((x,y),s,size,col(c,a),anchor='mm')
def nu_symbol(d,x,y,flavor=None,a=1):
    symbol(d,x,y,'ν',38,ICE,a)
    if flavor:d.text((x+13,y+7),flavor,19,col(ICE,a))
def segment(points,u):
    lens=[np.linalg.norm(np.array(b)-a) for a,b in zip(points,points[1:])]
    total=sum(lens);remain=total*m.clamp(u);out=[tuple(points[0])]
    for i,L in enumerate(lens):
        if remain>=L:out.append(tuple(points[i+1]));remain-=L
        else:
            p=np.array(points[i])+(np.array(points[i+1])-points[i])*(remain/L)
            out.append(tuple(p));break
    return out

def atom(d,x,y,t,alpha=1,r=30):
    for j in range(7):
        a=j*math.tau/6
        px=x+(0 if j==6 else r*.22*math.cos(a));py=y+(0 if j==6 else r*.22*math.sin(a))
        m.circle(d,px,py,r*.13,col((91,146,177),alpha))
    d.ellipse((x-r,y-r*.52,x+r,y+r*.52),None,col((52,97,129),alpha),1)
    d.ellipse((x-r*.52,y-r,x+r*.52,y+r),None,col((52,97,129),alpha),1)
    a=t*.27+x*.003
    m.circle(d,x+r*math.cos(a),y+r*.52*math.sin(a),2,col(ICE,alpha))

def plate_outline(x,y,amount):
    pts=[]
    for a in np.linspace(0,math.tau,80,endpoint=False):
        ca,sa=math.cos(a),math.sin(a)
        k=1/max(abs(ca)/23,abs(sa)/121)
        circle=np.array([30*ca,30*sa])
        box=np.array([k*ca,k*sa-k*ca*.4])
        p=circle+(box-circle)*amount
        pts.append((x+p[0],y+p[1]))
    return pts

def plate(d,x,y,t,amount=1,alpha=1):
    pts=plate_outline(x,y,amount)
    back=[(a+11*amount,b-4*amount) for a,b in pts]
    d.polygon(back,fill=col((27,62,88),alpha),outline=col((76,123,154),alpha),width=1)
    d.polygon(pts,fill=col((35,75,103),alpha),outline=col((108,177,209),alpha),width=1)
    # A subtle brushed-metal seam gives the plates thickness and material.
    for off in [-12,-5,4,12]:
        h=100*amount+13*(1-amount)
        m.line(d,[(x+off*amount,y-h),(x+off*amount,y+h)],col((46,89,119),alpha*.55),1)

def medium(im,t):
    d=m.Draw(im)
    morph=m.ramp(t,35.4,40.6)
    appearance=m.ramp(t,0,2.1)
    # Initially a diffuse field of atoms; rows emerge before the metaphor.
    diffuse=appearance*(1-m.ramp(t,30.5,36.2))*.42
    if diffuse>.005:
        for row in range(6):
            for j in range(9):
                x=460+j*139+(row%2)*37;y=165+row*147
                if abs(y-ROWS[0])<50 or abs(y-ROWS[1])<50:continue
                atom(d,x,y,t,diffuse,21)
    for k,y in enumerate(ROWS):
        opacity=appearance if k==0 else appearance*m.ramp(t,5.8,8.5)
        if 46<t<66.2 and k==1:opacity*=.48
        if t>67 and k==0:opacity*=.7
        for x in XS:
            if morph==0:atom(d,x,y,t,opacity)
            else:plate(d,x,y,t,morph,opacity)
    if 0<morph<1:
        lay=m.layer();ld=m.Draw(lay)
        for y in ROWS:
            for x in XS:atom(ld,x,y,t,1)
        m.composite(im,lay,1-morph)
    # Recalling the atom inside the plate explicitly connects the analogy.
    ghost=m.ramp(t,41.5,42.4)*(1-m.ramp(t,44.3,45.7))
    if ghost>0:
        lay=m.layer();ld=m.Draw(lay)
        for y in ROWS:
            for x in XS:atom(ld,x,y,t,1,30)
        m.composite(im,lay,ghost*.9)

def incoming(im,t):
    if t>12:return
    d=m.Draw(im)
    fade=1-m.ramp(t,8.5,12)
    # Several noninteracting neutrinos establish that the highlighted events are rare.
    for j,y in enumerate([183,491,575,961]):
        u=(t*.21+j*.22)%1;x=140+u*1630
        m.line(d,[(x-51,y),(x,y)],col((45,105,146),fade*.8),1)
        m.circle(d,x,y,2.6,col(ICE,fade*.8))
        if j==1:nu_symbol(d,x+15,y-28,None,fade*.6)
    for y,start,end,flavor in [(350,.6,4.25,'e'),(765,6.5,8.8,'μ')]:
        if t<start:continue
        u=m.clamp((t-start)/(end-start));x=m.mix(160,650,u)
        local=1-m.ramp(t,end+1.4,end+3.0)
        if local<=0:continue
        m.line(d,[(160,y),(x,y)],col((55,122,162),local),2)
        if t<end:
            m.glow(im,[(x,y,.9*local)],4);d=m.Draw(im)
            nu_symbol(d,x+12,y-34,flavor,local)
        else:
            dt=t-end
            for n in range(9):
                a=-1.15+n*.27
                distance=24+dt*(22+n*6)
                p=(650+distance*math.cos(a),y+distance*math.sin(a))
                m.line(d,[(650,y),p],col(ICE,local*.4),1)
                m.circle(d,*p,2,col(WHITE,local*.8))
            m.circle(d,650,y,12+dt*25,None,col(ICE,local),2)

def early_particles(im,t):
    if t<4.25 or t>=20.2:return
    d=m.Draw(im)
    for y,birth,s,c in [(350,4.25,'e⁻',ICE),(765,8.8,'μ⁻',MU)]:
        if t<birth:continue
        u=m.ramp(t,birth,birth+1.8);x=m.mix(650,870,u)
        m.line(d,[(650,y),(x,y)],col(c,.6),2)
        dot(d,x,y,7,c)
        alpha=1 if y==350 else .75 if t<14.5 else 1
        symbol(d,x+55,y-55,s,49,c,alpha)
        if y==350 and 11<t<14.5:
            a=.4+.6*math.sin((t-11)*1.8)**2
            m.circle(d,x,y,22+5*a,None,col(c,a),2)
        if y==765 and 14.5<t<20.2:
            a=.4+.6*math.sin((t-14.5)*1.5)**2
            m.circle(d,x,y,22+5*a,None,col(c,a),2)
    if t>=14.5:
        a=m.ramp(t,14.5,15.2)*(1-m.ramp(t,19.2,20.2))
        symbol(d,1180,551,'m(μ⁻) ≈ 207 m(e⁻)',49,WHITE,a)

E_PATH=np.array([(870,350),(980,350),(1030,375),(1080,314),(1120,381),(1143,361)],float)
def labels(d,t):
    if t<20.2:return
    shift=m.ramp(t,20.2,22)
    x=m.mix(925,310,shift);dy=m.mix(-55,-96,shift)
    symbol(d,x,ROWS[0]+dy,'e⁻',m.mix(49,53,shift),ICE)
    symbol(d,x,ROWS[1]+dy,'μ⁻',m.mix(49,53,shift),MU)

def physical_tracks(im,t):
    if t<20.2 or t>40.6:return
    alpha=1-m.ramp(t,35.4,39.5)
    if alpha<=0:return
    d=m.Draw(im)
    ue=m.ramp(t,20.2,24.1);um=m.clamp((t-20.2)/7.2)
    ep=segment(E_PATH,ue)
    m.line(d,ep,col(ICE,alpha*.8),2)
    # Short electron shower: many secondary paths and no persistent primary marker.
    for j in range(25):
        start=21.8+(j%6)*.28
        u=m.ramp(t,start,start+1.8)
        if u<=0:continue
        origin=np.array([1000+(j%4)*31,350+(j%3-1)*12])
        end=origin+np.array([74+(j*29)%170,math.sin(j*2.3)*121])
        mid=origin+(end-origin)*.45+np.array([0,math.cos(j)*20])
        pts=segment([origin,mid,end],u)
        m.line(d,pts,col(ICE,alpha*.3),1)
        if u<1:m.circle(d,*pts[-1],2.4,col(WHITE,alpha))
    if ue<1:dot(d,*ep[-1],5,col(ICE,alpha))
    mx=m.mix(870,1745,um)
    m.line(d,[(650,765),(mx,765)],col(MU,alpha*.87),3)
    if um<1:dot(d,mx,765,7,col(MU,alpha))
    else:
        d.polygon([(1745,765),(1727,758),(1727,772)],fill=col(MU,alpha))
    # Muons still transfer some energy, rather than ignoring matter entirely.
    for j,x in enumerate(XS[2:]):
        if mx>x:
            m.line(d,[(x,765),(x+24,765+(-1)**j*16)],col(MU,alpha*.27),1)

def projectile(d,x,y,size=1,angle=0,c=ICE,alpha=1):
    # Only a stylized projectile, no gun or operational weapon detail.
    pts=[(-38,-11),(-5,-11),(13,-6),(24,0),(13,6),(-5,11),(-38,11)]
    ca,sa=math.cos(angle),math.sin(angle)
    def transform(p):return (x+size*(p[0]*ca-p[1]*sa),y+size*(p[0]*sa+p[1]*ca))
    d.polygon([transform(p) for p in pts],fill=col(c,alpha*.7),outline=col(WHITE,alpha),width=1)
    m.line(d,[transform((-29,-7)),transform((7,-4))],col(WHITE,alpha*.8),1)

# Exactly the same plate positions are used by the atom layer and the analogy.
E_TIMES=[50.5,52.2,54.1,56.4,58.7]
E_POS=np.array([[380,350],[650,350],[805,386],[960,315],[1085,421]],float)
E_HITS=[(52.2,650,350),(54.1,805,386),(56.4,960,315)]
MU_START,MU_END=72.5,78.5
MU_HITS=[(MU_START+(x-380)/(1760-380)*(MU_END-MU_START),x,765) for x in XS]

def timed_path(times,points,t):
    if t<times[0]:return [tuple(points[0])],0
    out=[tuple(points[0])]
    for i,(a,b) in enumerate(zip(times,times[1:])):
        if t>=b:out.append(tuple(points[i+1]))
        else:
            u=m.clamp((t-a)/(b-a))
            p=points[i]+(points[i+1]-points[i])*u
            out.append(tuple(p));break
    direction=np.array(out[-1])-np.array(out[-2]) if len(out)>1 else np.array([1,0])
    return out,math.atan2(direction[1],direction[0])

def hole(d,x,y,large,alpha=1):
    if large:
        pts=[]
        for k in range(12):
            a=k*math.tau/12;r=12+(k*7%9)
            pts.append((x+math.cos(a)*r*.67,y+math.sin(a)*r))
        d.polygon(pts,fill=(8,25,43),outline=col((149,202,225),alpha),width=2)
    else:
        d.ellipse((x-5,y-10,x+6,y+10),fill=(8,25,43),outline=col(MU,alpha*.8),width=1)

def shard(d,x,y,size,angle,c,alpha):
    ca,sa=math.cos(angle),math.sin(angle)
    pts=[(-1,-.35),(.5,-.75),(1,.2),(-.45,.7)]
    d.polygon([(x+size*(px*ca-py*sa),y+size*(px*sa+py*ca)) for px,py in pts],fill=col(c,alpha),outline=col(WHITE,alpha*.55),width=1)

def debris(d,t,hit,index,big):
    start,x,y=hit;dt=t-start
    if dt<0:return
    if big:
        count=18+index*6
        # Slow drifting fragments remain visible while the narration describes
        # losing track of which fragment came from the projectile or plate.
        alpha=(1-m.ramp(dt,8,16))*(.75 if t>67 else 1)
        if alpha<=0:return
        travel=1-math.exp(-dt/2.25)
        for j in range(count):
            a=-1.38+(j/(count-1))*2.76
            distance=35+(j*71+index*23)%270
            px=x+math.cos(a)*distance*travel
            py=y+math.sin(a)*distance*travel+5*dt
            shard(d,px,py,3+(j*3%6),j*.91+dt*.7,ICE,alpha*.8)
    else:
        alpha=1-m.ramp(dt,.15,1.1)
        if alpha<=0:return
        for j in range(4):
            a=-.95+j*.63
            distance=dt*30
            shard(d,x+math.cos(a)*distance,y+math.sin(a)*distance,2.5,a+dt,MU,alpha*.65)

def analogy(im,t):
    if t<36:return
    d=m.Draw(im)
    # Morph a compact point into the projectile within the retained lane.
    show=m.ramp(t,37.8,40.8)
    if t<50.5:
        dot(d,380,350,6*(1-show)+.1,ICE)
        if show>0:projectile(d,380,350,.68,0,ICE,show)
    if t>=46:
        symbol(d,335,442,'9 mm',32,ICE,m.ramp(t,46,47))
    if 50.5<=t<58.0:
        path,angle=timed_path(E_TIMES,E_POS,t)
        m.line(d,path,col(ICE,.65),2)
        alpha=1-m.ramp(t,56.5,58)
        projectile(d,*path[-1],.68,angle,ICE,alpha)
    elif t>=58:
        path,_=timed_path(E_TIMES,E_POS,min(t,58.7))
        m.line(d,path,col(ICE,.23 if t<67 else .12),1)
    for i,hit in enumerate(E_HITS):
        if t>=hit[0]:
            hole(d,hit[1],hit[2],True,.8)
            debris(d,t,hit,i,True)
    # Projectile fragments share the plate fragments' shape and color.
    if t>=57.2:
        hit=(57.2,1005,356)
        debris(d,t,hit,3,True)
    if t>=66.2:
        a=m.ramp(t,66.2,67.7)
        symbol(d,335,861,'.50',35,MU,a)
        if t<MU_START:projectile(d,380,765,1.32,0,MU,a)
        else:
            u=m.clamp((t-MU_START)/(MU_END-MU_START));x=m.mix(380,1760,u)
            m.line(d,[(380,765),(x,765)],col(MU,.76),3)
            change=m.ramp(t,78.7,80.1)
            if change<1:projectile(d,x,765,1.32,0,MU,1-change)
            if change>0:
                dot(d,x,765,7*change,col(MU,change))
                symbol(d,1750,710,'μ⁻',48,MU,change)
        for i,hit in enumerate(MU_HITS):
            if t>=hit[0]:
                hole(d,hit[1],hit[2],False)
                debris(d,t,hit,i,False)
                dt=t-hit[0]
                if dt<.35:m.circle(d,hit[1],hit[2],11+dt*26,None,col(MU,1-dt/.35),2)

def frame(t):
    im=BG.copy();medium(im,t);incoming(im,t);early_particles(im,t)
    physical_tracks(im,t);labels(m.Draw(im),t);analogy(im,t)
    return im

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'timing.json').write_text(json.dumps(dict(duration=DURATION,fps=30,width=round(1920*m.S),height=round(1080*m.S),audio=False,narration_language="en",paragraph_boundary=36,transition=[35.4,40.6],timing='Editorial cue timings; English narration is not aligned to recorded audio.',cues=[dict(start=a,end=b,narration=s,visual=v) for a,b,s,v in CUES]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.en.srt')).write_text('\n\n'.join(f'{i+1}\n{old.stamp(a)} --> {old.stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(CUES))+'\n',encoding='utf-8')

def previews():
    times=[2,5.5,10,13,17,25,33,37,39.5,43,49,53,56,60,68,75,79.8,81.5]
    for i,t in enumerate(times):
        im=frame(t);im.save(OUT/f'frame_{t:05.1f}.jpg',quality=94)
    # Standard 16:9 tiles, no stretching or overlapping rows.
    sheet=Image.new('RGB',(1920,2160))
    for i,t in enumerate(times):
        im=Image.open(OUT/f'frame_{t:05.1f}.jpg')
        sheet.paste(im.resize((640,360),Image.Resampling.LANCZOS),(i%3*640,i//3*360))
    sheet.save(OUT/'contact_sheet.jpg',quality=94)
    frame(79.8).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',choices=[1,2],type=int,default=2);p.add_argument('--preview',action='store_true');args=p.parse_args()
    m.configure(args.scale);BG=m.bg();metadata();previews()
    if not args.preview:
        m.encode(frame,OUT/(NAME+('_4k' if args.scale==2 else '_1080p')+'.mp4'),DURATION,OUT/(NAME+'_1080p.mp4') if args.scale==2 else None)
