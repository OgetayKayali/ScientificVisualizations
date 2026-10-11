"""Sonic-wave analogy and a continuous rearward 3D wavefront envelope."""
from pathlib import Path
import argparse, json, math
from PIL import Image
import motion_common as m
import render as old
from render_light_speed import droplet

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'exports'/'cherenkov_v2'
NAME='06_sonik_patlama_cherenkov'
DURATION=32
BG=None
ICE=m.ICE; WHITE=m.WHITE; MU=(167,187,255)
Q=5/6  # sound: Mach 1.2; optical example: 0.75c / 0.90c.
SINE=math.sqrt(1-Q*Q)
CUES=[
 (0,9.5,'Tıpkı süpersonik bir uçağın oluşturduğu ses dalgalarının bir araya gelerek sonik patlamaya sebep olması gibi,','Uçak büyüyen ses dalgalarını geride bırakır; dalga cephelerinin ortak zarfı belirginleşir.'),
 (9.5,19.5,'müon da suyun içerisinden geçerken elektromanyetik dalgaların belirli doğrultularda üst üste binip yoğunlaşmasına yol açıyor.','Uçak müona dönüşür, su belirir. Geriye açılan ortak dalga zarfı korunur.'),
 (19.5,28.5,'Bu sayede, hareket doğrultusu etrafında bir koni oluşturan özel bir ışınım ortaya çıkıyor.','Aynı tepe noktası ve geriye açılan zarf üç boyutlu yüzeye dönüşür. Kamera hafifçe döner; koni üzerinde hareketli işaret yoktur.'),
 (28.5,32,'Buna Cherenkov radyasyonu diyoruz.','Müonun arkasına açılan sade koni yüzeyi sabit açıyla tutulur.'),
]

def col(c,a=1):return tuple(round(m.mix(b,v,m.clamp(a))) for b,v in zip((10,29,47),c))
def txt(d,x,y,s,size=40,c=WHITE,a=1):
    if a>.001:d.text((x,y),s,size,col(c,a),anchor='mm')
def arrow(d,p,q,c=ICE,width=2,a=1,head=10):
    m.line(d,[p,q],col(c,a),width)
    angle=math.atan2(q[1]-p[1],q[0]-p[0])
    d.polygon([q,(q[0]-head*math.cos(angle-.4),q[1]-head*math.sin(angle-.4)),(q[0]-head*math.cos(angle+.4),q[1]-head*math.sin(angle+.4))],col(c,a))

def plane(d,x,y,scale=1,a=1):
    upper=[(65,0),(39,-8),(7,-10),(-24,-57),(-38,-57),(-18,-10),(-44,-7),(-56,-25),(-64,-25),(-58,0)]
    pts=upper+[(px,-py) for px,py in reversed(upper[:-1])]
    pts=[(x+px*scale,y+py*scale) for px,py in pts]
    d.polygon(pts,col((144,185,208),a),col(WHITE,a*.85),1)
    m.line(d,[(x-48*scale,y),(x+45*scale,y)],col((58,106,140),a),2)
    d.ellipse((x+16*scale,y-4*scale,x+35*scale,y+4*scale),col((30,73,105),a))

def medium(d,t):
    a=m.ramp(t,8.8,11)
    if a<=0:return
    d.rounded_rectangle((145,110,1775,970),24,col((11,36,55),a),col((52,99,128),a*.6),1)
    wave=[(x,119+2*math.sin(x*.027+t*.55)) for x in range(170,1750,7)]
    m.line(d,wave,col(ICE,a*.2),1)
    droplet(d,200,160,a)
    txt(d,251,161,'H₂O',30,ICE,a*.8)
    for row in range(6):
        for j in range(13):
            x=214+j*116+(row%2)*24;y=243+row*121
            # Small representative molecules do not imply a literal lattice.
            x+=7*math.sin(row*8+j*2.8+t*.14);y+=5*math.cos(j+row*3.8+t*.18)
            m.circle(d,x,y,3.8,None,col(ICE,a*.12),1)
            for angle in [.5,2.35]:
                hx=x+9*math.cos(angle);hy=y+9*math.sin(angle)
                m.line(d,[(x,y),(hx,hy)],col(ICE,a*.12),1)
                m.circle(d,hx,hy,1.5,col(ICE,a*.14))

def clipped_circle(d,cx,cy,r,c,width=1):
    segment=[]
    for j in range(241):
        th=j*math.tau/240;x=cx+r*math.cos(th);y=cy+r*math.sin(th)
        if 160<=x<=1760 and 130<=y<=952:segment.append((x,y))
        else:
            if len(segment)>1:m.line(d,segment,c,width)
            segment=[]
    if len(segment)>1:m.line(d,segment,c,width)

def wave_scene(im,t):
    a=m.ramp(t,0,.7)*(1-m.ramp(t,18.5,21.2))
    if a<=0:return
    d=m.Draw(im);clock=t+.3;v=108.;u=v*Q
    source_x=min(1250,435+v*clock);y=540
    optic=m.ramp(t,9,11)
    base=tuple(round(m.mix(a0,b,optic)) for a0,b in zip((138,176,198),ICE))
    # Wave centers remain at their emission positions, with a tracking camera.
    age_max=4.7;dt=.48
    events=[]
    for j in range(math.floor(clock/dt)+1):
        age=clock-j*dt
        if not 0<age<age_max:continue
        cx=source_x-v*age;r=u*age
        fade=m.ramp(age,0,.24)*(1-m.ramp(age,3.5,age_max))
        clipped_circle(d,cx,y,r,col(base,a*fade*.34),1)
        tx=cx+r*Q;ty=r*SINE
        events.append((age,cx,tx,ty,fade))
    # Keep this same backward wavefront envelope in the final 3D scene.
    extent=min(v*clock,440)
    dx=extent*(1-Q*Q);dy=extent*Q*SINE
    strength=a*m.ramp(t,3.6,6)
    for sign in [-1,1]:
        pts=[(source_x-dx,y+sign*dy),(source_x,y)]
        m.line(d,pts,col(base,strength*.10),17)
        m.line(d,pts,col(base,strength*.30),7)
        m.line(d,pts,col(base,strength*.95),2)
    # Repeated wavefront contacts, rather than an explosion at Mach 1.
    for age,cx,tx,ty,fade in events:
        for sign in [-1,1]:
            r=3.5+2*optic
            if optic<1:m.circle(d,tx,y+sign*ty,r,col(base,strength*fade*(1-optic)))
    # Smoothly shrink the aircraft into the charged particle.
    shape=1-m.ramp(t,9.1,10.8)
    if shape>.002:plane(d,source_x,y,.18+.82*shape,a*shape)
    if optic>0:
        m.circle(d,source_x,y,13,col(MU,a*optic*.18))
        m.circle(d,source_x,y,6,col(MU,a*optic))
        m.circle(d,source_x-1,y-1,2,col(WHITE,a*optic))
        txt(d,source_x,601,'μ⁻',42,MU,a*optic)
    arrow(d,(source_x+85,540),(source_x+185,540),MU if optic>.5 else WHITE,2,a*.65,11)

def projection(p,t):
    yaw=math.radians(23)*m.ramp(t,21.2,26)
    pitch=math.radians(-5)*m.ramp(t,21.2,26)
    x,y,z=p
    xx=x*math.cos(yaw)+z*math.sin(yaw)
    zz=-x*math.sin(yaw)+z*math.cos(yaw)
    yy=y*math.cos(pitch)-zz*math.sin(pitch)
    origin_x=m.mix(1250,1160,m.ramp(t,21.2,26))
    return (origin_x+xx,540+yy),zz

def wavefront_cone(im,t):
    a=m.ramp(t,18.8,21.2)
    if a<=0:return
    d=m.Draw(im);sweep=m.ramp(t,19.2,23.5)
    # The initial section is identical to the existing 2D wavefront envelope.
    L=m.mix(440*(1-Q*Q),255,m.ramp(t,18.8,24.5))
    origin,_=projection((0,0,0),t)
    def ring(phi,l=L):
        r=l*Q/SINE
        return projection((-l,r*math.cos(phi),r*math.sin(phi)),t)
    # The surface remains behind the muon, with no photon trajectories.
    if sweep>.001:
        overlay=m.layer();od=m.Draw(overlay)
        facets=[]
        for i in range(96):
            p=i*math.tau/96;q=(i+1)*math.tau/96
            (p0,z0)=ring(p);(p1,z1)=ring(q)
            shade=round(24+13*(.5+.5*math.sin(p)))
            facets.append(((z0+z1)/2,[origin,p0,p1],(48,157,218,round(shade*a*sweep))))
        for _,points,color in sorted(facets):od.polygon(points,color)
        m.composite(im,overlay)
        d=m.Draw(im)
    rim=[ring(j*math.tau/192)[0] for j in range(193)]
    m.line(d,rim,col(ICE,a*sweep*.64),2)
    # Only the silhouette generators: no interior radial or spiral markings.
    slope=lambda p:(p[1]-origin[1])/(p[0]-origin[0])
    for endpoint in [min(rim,key=slope),max(rim,key=slope)]:
        m.line(d,[origin,endpoint],col(ICE,a*.8),2)
    start,_=projection((85,0,0),t)
    end,_=projection((185,0,0),t)
    arrow(d,start,end,MU,2,a*.65,11)
    m.circle(d,*origin,14,col(MU,a*.15));m.circle(d,*origin,6.5,col(MU,a))
    m.circle(d,origin[0]-1,origin[1]-1,2.4,col(WHITE,a))
    txt(d,origin[0],origin[1]+62,'μ⁻',42,MU,a)

def frame(t):
    im=BG.copy();medium(m.Draw(im),t);wave_scene(im,t);wavefront_cone(im,t)
    return im

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'timing.json').write_text(json.dumps(dict(duration=DURATION,fps=30,width=round(m.W*m.S),height=round(m.H*m.S),audio=False,timing='Estimated narration; no recorded voice supplied.',cone_representation='Rearward wavefront envelope; no photon trajectories or moving surface markers.',wave_to_source_speed=Q,wavefront_half_angle_degrees=math.degrees(math.asin(Q)),cues=[dict(start=a,end=b,narration=s,visual=v) for a,b,s,v in CUES]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.srt')).write_text('\n\n'.join(f'{i+1}\n{old.stamp(a)} --> {old.stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(CUES))+'\n',encoding='utf-8')

def previews():
    times=[1.5,4,7.5,9.8,11.5,14,17,20.5,23,25.5,28.5,31]
    sheet=Image.new('RGB',(1920,1440))
    for i,t in enumerate(times):
        im=frame(t);im.save(OUT/f'kare_{t:05.1f}.jpg',quality=94)
        sheet.paste(im.resize((640,360),Image.Resampling.LANCZOS),(i%3*640,i//3*360))
    sheet.save(OUT/'kontrol.jpg',quality=94)
    frame(28.5).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',type=int,choices=[1,2],default=2);p.add_argument('--preview',action='store_true');args=p.parse_args()
    m.configure(args.scale);BG=m.bg();metadata();previews()
    if not args.preview:m.encode(frame,OUT/(NAME+('_4k' if args.scale==2 else '_1080p')+'.mp4'),DURATION,OUT/(NAME+'_onizleme_1080p.mp4') if args.scale==2 else None)
