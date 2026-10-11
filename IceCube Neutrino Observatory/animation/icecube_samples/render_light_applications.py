"""Five linked uses of electromagnetic radiation, without on-screen text."""
from pathlib import Path
import argparse, json, math
import numpy as np
from PIL import Image
import motion_common as m
import render as old
from render_direction_calibration_v3 import line, dot, col, arrow, vec

OUT=Path(__file__).resolve().parents[2]/'exports'/'light_applications_v1'
NAME='09_isikla_anlamak'
DURATION=40
BG=None
ICE=m.ICE;WHITE=m.WHITE;MU=(167,187,255);GOLD=(250,213,150);CORAL=(248,144,114)
CUES=[
 (0,9,'Bir ortamdaki maddenin ne olduğunu anlamak için, o ortamdan geçen ışığın maruz kaldığı etkilere bakıyoruz.','Işık numuneden ve prizmadan geçer. Soğurma çizgileri bilinen üç spektral örnekten biriyle eşleşir.'),
 (9,16,'Bir yıldızın evrimini anlamak için ondan gelen ışığın değişimini inceliyoruz.','Temsili Güneş benzeri yıldız büyüyüp kızarır; gözlenen tayfın tepesi daha uzun dalga boylarına kayar.'),
 (16,24,'Ötegezegenleri görmek için yıldızının önünden geçip ışığını kesmesini bekliyoruz.','Gezegen yıldızın önünden geçer. Aynı örtüşme hesabından üretilen parlaklık eğrisi eşzamanlı düşer ve toparlanır.'),
 (24,28.5,'Hastalandığımızda hastanede röntgen,','Sabit X-ışını kaynağı, temsili gövde ve farklı soğurmayı gösteren radyografi.'),
 (28.5,33,'tomografi cihazları kullanıyoruz.','Kaynak gövde kesiti etrafında döner, karşısındaki dedektör ölçer; sağda kesit görüntüleri oluşur.'),
 (33,40,'Yemeklerimizi ısıtmak için bile mikrodalga kullanıyoruz.','Kapalı mikrodalga fırın içinde dalgalar, ısınan çorba ve hafif buhar.'),
]

def blendcolor(a,b,u):return tuple(round(m.mix(x,y,u)) for x,y in zip(a,b))
def wave(d,p,q,t,wavelength=60,amp=12,c=ICE,a=1,width=2):
    p=np.array(p,float);q=np.array(q,float);delta=q-p;length=float(np.linalg.norm(delta))
    tangent=delta/length;normal=np.array([-tangent[1],tangent[0]])
    pts=[]
    for x in np.linspace(0,length,max(16,round(length/3))):
        envelope=math.sin(math.pi*x/length)**.45
        pts.append(p+tangent*x+normal*amp*envelope*math.sin(math.tau*(x/wavelength-t*1.4)))
    line(d,pts,c,width,a)

def glowdisc(d,c,r,color,a=1):
    for j in range(12,0,-1):
        rr=r*(1+j*.027)
        dot(d,c,rr,color,a*.04*(13-j)/12)
    for j in range(48,0,-1):
        u=j/48
        color2=blendcolor(WHITE,color,u**.65)
        dot(d,c,r*u,color2,a*(.65+.35*(1-u)))
    m.circle(d,*c,r,None,col(color,a*.75),1)

def spectrum(d,x,y,w,h,notches=(),a=1):
    if a<=.001:return
    colors=[(135,143,245),(110,203,255),(136,219,206),(218,219,157),(247,170,137)]
    for j in range(125):
        f=j/124;z=min(3,int(f*4));c=blendcolor(colors[z],colors[z+1],f*4-z)
        d.rectangle((x+w*j/125,y,x+w*(j+1)/125+1,y+h),col(c,a*.8))
    for f in notches:
        xx=x+w*f;d.rectangle((xx-3,y,xx+3,y+h),col((7,21,36),a))

NOTCHES=[.23,.51,.76]
def material(t):
    im=BG.copy();d=m.Draw(im)
    # Source and a transparent sample cell.
    a=m.ramp(t,0,.8)
    glowdisc(d,(268,471),30,WHITE,a)
    d.rounded_rectangle((641,285,843,657),20,col(ICE,a*.035),col(ICE,a*.58),2)
    for j in range(21):
        x=666+(j*61)%151;y=321+(j*83)%287
        dot(d,(x+4*math.sin(t*.5+j),y+4*math.cos(t*.6+j)),3,ICE,a*.48)
    wave(d,(311,471),(624,471),t,47,13,WHITE,a)
    passed=m.ramp(t,1.8,3.1)
    wave(d,(860,471),(1025,471),t,47,13,ICE,passed)
    prism=[(1049,384),(1001,532),(1150,532),(1049,384)]
    line(d,prism,ICE,2,a*.7)
    # Dispersion after the prism maps wavelengths onto a visible band.
    for j,c in enumerate([MU,ICE,(157,220,207),GOLD,CORAL]):
        line(d,[(1091,470),(1275+j*91,363)],c,2,passed*.6)
    reveal=m.ramp(t,3,4.3)
    spectrum(d,1262,331,439,48,NOTCHES,reveal)
    # Compare the observed fingerprint to candidate reference spectra.
    patterns=[[.15,.42,.85],NOTCHES,[.30,.62,.88]]
    for i,pattern in enumerate(patterns):
        yy=507+i*109;b=m.ramp(t,4.4+i*.32,5.1+i*.32)
        spectrum(d,1262,yy,439,27,pattern,b*.6)
        if i==1:
            highlight=m.ramp(t,6,7.2)
            d.rounded_rectangle((1244,yy-14,1719,yy+41),8,None,col(ICE,highlight),2)
            for f in NOTCHES:
                xx=1262+439*f
                line(d,[(xx,383),(xx,yy-18)],ICE,1,highlight*.25)
    return im

def axes(d,x,y,w,h,a=1):
    arrow(d,(x,y),(x+w,y),ICE,1,a*.45,8)
    arrow(d,(x,y),(x,y-h),ICE,1,a*.45,8)

def stellar(t):
    im=BG.copy();d=m.Draw(im)
    u=m.ramp(t,9.8,15.2);r=m.mix(124,213,u);c=blendcolor(GOLD,CORAL,u)
    glowdisc(d,(575,512),r,c)
    for j in range(5):
        th=-.75+j*.37
        p=np.array([575,512])+np.array([math.cos(th),math.sin(th)])*(r+25)
        line(d,[p,p+np.array([math.cos(th),math.sin(th)])*45],c,1,.22)
    wave(d,(825,512),(1075,512),t,m.mix(41,77,u),11,c,.74)
    x=1170;y=693;w=530;h=335
    axes(d,x,y,w,h)
    # Visible light occupies only the short-wavelength end of this range.
    spectrum(d,x+(380-280)/1220*w,y+34,(750-380)/1220*w,13,(),.6)
    line(d,[(x+(750-280)/1220*w,y+41),(x+w,y+41)],CORAL,3,.25)
    # Shape-normalized thermal spectra: peak shifts toward longer wavelengths.
    def profile(z,peak):
        lam=280+z*1220
        return lam**-5/math.expm1(4.965114*peak/lam)
    def curve(peak):
        values=[profile(j/160,peak) for j in range(161)];mx=max(values)
        return [(x+w*j/160,y-h*.84*v/mx) for j,v in enumerate(values)]
    line(d,curve(510),GOLD,1,.24)
    peak=m.mix(510,890,u)
    line(d,curve(peak),c,3,1)
    peakx=x+w*(peak-280)/1220
    dot(d,(peakx,y-h*.84),5,WHITE)
    return im

TRANSIT_CENTER=np.array([657.,445.]);STAR_R=168.;PLANET_R=37.
def planet_position(t):return np.array([m.mix(353,961,m.clamp((t-17)/5.5)),TRANSIT_CENTER[1]+27])
def overlap_area(distance,r=STAR_R,R=PLANET_R):
    if distance>=r+R:return 0.
    if distance<=abs(r-R):return math.pi*min(r,R)**2
    a=math.acos(np.clip((distance*distance+r*r-R*R)/(2*distance*r),-1,1))
    b=math.acos(np.clip((distance*distance+R*R-r*r)/(2*distance*R),-1,1))
    term=(-distance+r+R)*(distance+r-R)*(distance-r+R)*(distance+r+R)
    return r*r*a+R*R*b-.5*math.sqrt(max(0,term))
def transit(t):
    im=BG.copy();d=m.Draw(im)
    glowdisc(d,TRANSIT_CENTER,STAR_R,GOLD)
    # Viewer-facing transit, with a common time parameter for disk and curve.
    p=planet_position(t)
    dot(d,p,PLANET_R,(9,28,46));m.circle(d,*p,PLANET_R,None,col(ICE,.75),2)
    # Telescope observes the system; no individual planet light is detected here.
    wave(d,(1050,445),(1300,445),t,64,8,ICE,.55)
    d.polygon([(1340,382),(1487,420),(1487,470),(1340,508)],col(ICE,.10),col(ICE,.8),2)
    d.ellipse((1320,382,1360,508),None,col(ICE,.9),2)
    line(d,[(1450,500),(1400,652),(1500,652)],ICE,2,.65)
    line(d,[(1450,500),(1450,653)],ICE,2,.65)
    x0=370;y0=882;w=1180;h=120
    axes(d,x0,y0,w,h+35)
    pts=[];elapsed=m.clamp((t-16.5)/6.5)
    for j in range(round(elapsed*260)+1):
        tt=16.5+6.5*j/260
        pp=planet_position(tt)
        loss=overlap_area(float(np.linalg.norm(pp-TRANSIT_CENTER)))/(math.pi*PLANET_R**2)
        pts.append((x0+w*j/260,y0-125+loss*91))
    if len(pts)>1:
        line(d,pts,ICE,3,1);dot(d,pts[-1],5,WHITE)
    # The dip amplitude is exaggerated for readability, never numerically labelled.
    return im

def ribcage(d,c,s=1,a=1):
    x,y=c
    line(d,[(x,y-117*s),(x,y+133*s)],WHITE,7*s,a*.7)
    for j in range(8):
        yy=y-105*s+j*27*s;w=(72+29*math.sin(j/8*math.pi))*s
        for side in [-1,1]:
            pts=[]
            for k in range(25):
                u=k/24
                pts.append((x+side*(8*s+w*math.sin(u*math.pi*.8)),yy+30*s*u))
            line(d,pts,WHITE,5*s,a*.8)
    line(d,[(x-83*s,y-130*s),(x-13*s,y-115*s),(x,y-101*s),(x+13*s,y-115*s),(x+83*s,y-130*s)],WHITE,5*s,a*.85)

def torso(d,c,s=1,a=1):
    x,y=c
    m.circle(d,x,y-233*s,43*s,None,col(ICE,a*.6),2)
    pts=[(x-24*s,y-187*s),(x-26*s,y-153*s),(x-102*s,y-125*s),(x-141*s,y+71*s),(x-113*s,y+82*s),(x-87*s,y-39*s),(x-80*s,y+152*s),(x+80*s,y+152*s),(x+87*s,y-39*s),(x+113*s,y+82*s),(x+141*s,y+71*s),(x+102*s,y-125*s),(x+26*s,y-153*s),(x+24*s,y-187*s)]
    d.polygon(pts,col(ICE,a*.055));line(d,pts,ICE,2,a*.7)

def radiograph(t):
    im=BG.copy();d=m.Draw(im)
    torso(d,(773,568),1.17)
    # Fixed source and a flat recording panel.
    d.rounded_rectangle((280,411,384,491),13,col(ICE,.10),col(ICE,.8),2)
    line(d,[(300,493),(300,751),(218,751)],ICE,3,.65)
    a=m.ramp(t,24.7,25.6)
    for yy in [407,465,523,581,639,697]:
        line(d,[(386,451),(1190,yy)],ICE,1,a*.27)
    d.rounded_rectangle((1235,273,1625,806),15,col(ICE,.06),col(ICE,.65),2)
    ribcage(d,(1430,518),1.32,m.ramp(t,25.3,27))
    # The projection brightens without showing a scanning beam in a fixed X-ray.
    return im

def slice_image(d,c,rx=119,ry=86,a=1):
    if a<=.001:return
    x,y=c
    d.ellipse((x-rx,y-ry,x+rx,y+ry),col(ICE,a*.25),col(WHITE,a*.6),2)
    for side in [-1,1]:
        d.ellipse((x+side*rx*.40-rx*.25,y-ry*.59,x+side*rx*.40+rx*.25,y+ry*.36),col((20,51,70),a),col(ICE,a*.35),1)
    dot(d,(x,y+ry*.58),ry*.16,WHITE,a*.85)
    dot(d,(x+rx*.08,y+ry*.16),ry*.18,ICE,a*.5)

def tomography(t):
    im=BG.copy();d=m.Draw(im)
    c=np.array([610.,504.]);a=m.ramp(t,28.5,29.2)
    # Front view of the gantry, with a schematic axial section in its opening.
    m.circle(d,*c,236,col(ICE,.06),col(ICE,.6),3)
    m.circle(d,*c,171,col((9,25,42)),col(ICE,.65),2)
    d.polygon([(510,633),(710,633),(810,806),(410,806)],col(ICE,.06),col(ICE,.5),2)
    slice_image(d,c,99,79,.6)
    th=(t-29)*2.7-.8
    source=c+np.array([math.cos(th),math.sin(th)])*206
    detector=c-np.array([math.cos(th),math.sin(th)])*206
    normal=np.array([-math.sin(th),math.cos(th)])
    for off in [-53,-26,0,26,53]:line(d,[source,detector+normal*off],ICE,1,a*.28)
    line(d,[detector-normal*59,detector+normal*59],ICE,8,a*.9)
    dot(d,source,14,ICE,a);dot(d,source,5,WHITE,a)
    # Reconstructed slices stack as angular measurements accumulate.
    for j in range(3):
        b=m.ramp(t,29.4+j*.75,30.1+j*.75)
        slice_image(d,(1380,388+j*158),151,106,b)
    arrow(d,(931,510),(1109,510),ICE,2,m.ramp(t,29.8,31),10)
    return im

def microwave(t):
    im=BG.copy();d=m.Draw(im)
    d.rounded_rectangle((421,282,1499,798),30,col(ICE,.025),col(ICE,.85),3)
    d.rounded_rectangle((466,326,1261,743),18,col((10,30,47)),col(ICE,.5),2)
    line(d,[(1235,420),(1235,654)],ICE,7,.7)
    d.rounded_rectangle((1284,325,1456,450),10,col(ICE,.025),col(ICE,.4),2)
    # A tiny lamp and two knobs are sufficient to identify the appliance.
    dot(d,(1370,385),7,ICE,.75)
    for yy in [537,661]:
        m.circle(d,1370,yy,37,None,col(ICE,.75),2)
        line(d,[(1370,yy),(1380,yy-22)],ICE,3,.7)
    for xx in [519,1401]:d.rounded_rectangle((xx,799,xx+42,820),5,col(ICE,.3))
    heat=m.ramp(t,34.3,39.2)
    for j in range(3):
        wave(d,(499,405+j*61),(1221,405+j*61),t,125,14,ICE,m.ramp(t,33.6,34.5)*(.30+j*.07),2)
    # No wave is drawn outside the enclosed cavity.
    d.ellipse((662,656,1120,704),None,col(ICE,.5),2)
    d.arc((671,657,1110,698),t*35,t*35+95,col(ICE,.8),2)
    soup=blendcolor(ICE,CORAL,heat)
    bowl=[(706,575),(1074,575),(1030,659),(1005,675),(775,675),(751,659),(706,575)]
    d.polygon(bowl,col(ICE,.10));line(d,bowl,ICE,3,.8)
    d.ellipse((706,551,1074,603),col(soup,.24),col(ICE,.7),2)
    for j in range(9):
        xx=762+(j*53)%253;yy=569+(j*13)%20
        dot(d,(xx,yy),4,soup,.75)
    # Warmth is conveyed by the food colour and restrained steam.
    for j in range(3):
        pts=[]
        for k in range(45):
            u=k/44
            pts.append((811+j*79+8*math.sin(u*7-t*1.4+j),541-u*104))
        line(d,pts,WHITE,2,heat*.42)
    return im

SCENES=[(0,material),(9,stellar),(16,transit),(24,radiograph),(28.5,tomography),(33,microwave)]
def frame(t):
    idx=max(i for i,(start,fn) in enumerate(SCENES) if t>=start)
    start,fn=SCENES[idx]
    if idx and t<start+.8:
        return Image.blend(SCENES[idx-1][1](start),fn(t),m.ramp(t,start,start+.8))
    return fn(t)

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'timing.json').write_text(json.dumps(dict(duration=DURATION,fps=30,width=round(1920*m.S),height=round(1080*m.S),audio=False,on_screen_text=False,timing='Estimated narration timing; no voice recording supplied',cues=[dict(start=a,end=b,narration=s,visual=v) for a,b,s,v in CUES]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.srt')).write_text('\n\n'.join(f'{i+1}\n{old.stamp(a)} --> {old.stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(CUES))+'\n',encoding='utf-8')

def previews():
    times=[2,5,8,10.5,13,15.5,17.3,19.7,22.5,26.8,30.6,32.7,34.7,37,39.5]
    sheet=Image.new('RGB',(1920,1800))
    for i,t in enumerate(times):
        im=frame(t);im.save(OUT/f'kare_{t:04.1f}.jpg',quality=94)
        sheet.paste(im.resize((640,360),Image.Resampling.LANCZOS),(i%3*640,i//3*360))
    sheet.save(OUT/'kontrol.jpg',quality=94)
    frame(19.7).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',type=int,choices=[1,2],default=2);p.add_argument('--preview',action='store_true');args=p.parse_args()
    m.configure(args.scale);BG=m.bg();metadata();previews()
    if not args.preview:m.encode(frame,OUT/(NAME+('_4k' if args.scale==2 else '_1080p')+'.mp4'),DURATION,OUT/(NAME+'_onizleme_1080p.mp4') if args.scale==2 else None)
