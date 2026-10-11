"""Three linked narration paragraphs: pointing, timing, ice and calibration."""
from pathlib import Path
from functools import lru_cache
import argparse, json, math
import numpy as np
from PIL import Image
import motion_common as m
import render as old

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'exports'/'direction_calibration_v3'
NAME='07_yon_zamanlama_buz_kalibrasyon'
DURATION=132
BG=None; ARRAY=None; DEEP=None
ICE=m.ICE; WHITE=m.WHITE; MU=(167,187,255); GOLD=(248,207,142)
NODES=[(670,y) for y in [270,480,690]]+[(1250,y) for y in [270,480,690]]
COLORS=[(112,212,247),(134,222,255),(151,203,255),(166,192,250),(187,210,255),(174,200,255)]
A=np.array([300.,850.]);B=np.array([1680.,240.]);DIR=(B-A)/np.linalg.norm(B-A)
TRACK_LENGTH=float(np.linalg.norm(B-A));VP=165.;VL=VP*(.75/.99)
Q=VL/VP;SIN=math.sqrt(1-Q*Q)
CUES=[
 (0,10.5,'Bir kilometre küplük bir hacmi kapsayacak bir dedektör dizisi yapmanın ardındaki fikirlerden biri de, nötrinonun geldiği yönü tespit edebilmek.','Küpsüz üç boyutlu dizi. νμ buz içinde bir etkileşme noktasına yaklaşır.'),
 (10.5,21.5,'Eğer bir müon nötrinosu buz içerisindeki maddeyle etkileşerek bir müon üretirse, bu müon da yüksek enerjilerde nötrinoyla neredeyse aynı yönde ilerliyor.','νμ etkileşmesinden μ⁻ çıkar ve çok küçük bir açı farkıyla ilerler; DOM ışıkları belirir.'),
 (21.5,28.5,'Yani müonun yönünü bulursak, nötrinonun geliş yönünü de büyük bir hassasiyetle belirleyebiliriz.','Müon izi geriye doğru uzatılarak geliş doğrultusu vurgulanır; bir DOM yakın plana alınır.'),
 (28.5,36.5,'IceCube’daki dedektörler, DOM’lar nanosaniye, yani saniyenin milyarda biri zaman hassasiyetine sahip.','1 ns = 10⁻⁹ s altında daha büyük 0.000000001 s; sekiz ondalık sıfır sırayla belirir.'),
 (36.5,40,'Ayrıca birbirleriyle de senkron haldeler.','İki kablo üzerinde üçer DOM; altı saat aynı fazda çalışır.'),
 (40,45.5,'Birisi Cherenkov ışığını tespit ettiği anda bunu ölçüp kayıt altına alıyor.','Tek müon iki kablonun arasından geçer. Arkasına açılan Cherenkov konisi ilk DOM’a temas edince zaman kaydı alınır.'),
 (45.5,58.5,'Zaman içerisinde hangi dedektörün, ne zaman ışık tespit ettiğine bakacak olursak, parçacığın izlediği yolu bulabiliyoruz.','Aynı müon ve koni ilerler; altı temas altı sabit zaman kaydı üretir. Sonra aynı parçacık izi yeniden belirginleşir.'),
 (58.5,64,'Bütün bir dedektör dizisinin buzun içerisinde olma sebebi de bu.','Yakın DOM’lar diziye geri yerleşir; kamera buz yüzeyini ve derinliği gösterecek kadar uzaklaşır.'),
 (64,76,'Hem yüzeyden gelen ışıktan tamamen izole, hem de üzerindeki kilometrelerce kalınlıktaki buz tabakası, kozmik ışınların oluşturduğu istenmeyen parçacıkların önemli bir kısmını engelliyor.','Gün ışığı üst tabakalarda söner. Kozmik sağanağın çoğu derine ulaşmaz; az sayıda nüfuz eden müon korunur.'),
 (76,80.5,'Üstelik bu derinlikteki buz olağanüstü derecede saydam.','Kamera derin diziye iner; seçilmiş DOM büyüyerek optik yayılım sahnesine bağlanır.'),
 (80.5,88.5,'Böylece Cherenkov ışığı onlarca, hatta yüzlerce metre uzaklıktaki dedektörlere ulaşabiliyor.','Temsili ışık darbeleri geniş aralıklarla yerleştirilmiş DOM’lara ulaşır.'),
 (88.5,92,'Ama buz tamamen kusursuz değil.','Doğrudan yolun yanında iki farklı optik yol seçilir.'),
 (92,99,'Işığın bir kısmı soğuruluyor, bir kısmı da saçılarak yön değiştiriyor.','Bir darbe sönerek kaybolur; diğeri birkaç kez yön değiştirip daha geç ulaşır.'),
 (99,110,'Hatta buzun ışığı ne kadar soğurduğunu ve saçtığını anlamak için, her bir DOM\'un içerisinde 12 tane LED flaşör bulunuyor.','Alıcı DOM yakın plana gelir. İçteki temsili flaşör kartında 12 LED tek tek belirginleşir.'),
 (110,122,'Bunlar kontrollü ışık darbeleri gönderiyor, çevredeki DOM\'lar da bunları algılayarak ışığın buzda nasıl ilerlediğini ölçüyor.','Aynı DOM küçülerek bir vericiye dönüşür; tekrarlanan kontrollü darbeler çevredeki DOM’larda kaydedilir.'),
 (122,132,'Böylece hem buzun optik özelliklerini hem de dedektörlerimizin zamanlama yanıtını kalibre edebiliyoruz.','Doğrudan ve saçılmış yollar kalır. Darbe zaman dağılımı ölçüm noktalarıyla ve ona uyan yanıt eğrisiyle gösterilir.'),
]

def col(c,a=1):return tuple(round(m.mix(b,v,m.clamp(a))) for b,v in zip((9,27,45),c))
def txt(d,x,y,s,size=36,c=WHITE,a=1):
    if a>.001:d.text((x,y),s,size,col(c,a),anchor='mm')
def line(d,p,c=ICE,w=2,a=1):
    if a>.001:m.line(d,[tuple(v) for v in p],col(c,a),w)
def arrow(d,p,q,c=ICE,w=2,a=1,head=11):
    line(d,[p,q],c,w,a);th=math.atan2(q[1]-p[1],q[0]-p[0])
    d.polygon([tuple(q),(q[0]-head*math.cos(th-.4),q[1]-head*math.sin(th-.4)),(q[0]-head*math.cos(th+.4),q[1]-head*math.sin(th+.4))],col(c,a))
def dashed(d,p,q,c=ICE,a=1):
    p=np.array(p);q=np.array(q);length=float(np.linalg.norm(q-p))
    for u in np.arange(0,length,19):line(d,[p+(q-p)*u/length,p+(q-p)*min(u+8,length)/length],c,1,a)
def dot(d,p,r=5,c=WHITE,a=1):
    if a>.001:m.circle(d,*p,r,col(c,a))
def vec(p,q,u):return np.array(p)+(np.array(q)-p)*u

def project(x,y,z,deep=0):
    yaw=.34
    xx=x*math.cos(yaw)+y*math.sin(yaw);yy=-x*math.sin(yaw)+y*math.cos(yaw)
    spread=m.mix(105,65,deep);height=m.mix(540,250,deep)
    return np.array([960+xx*spread,m.mix(225,610,deep)+yy*m.mix(25,13,deep)+z*height])

def array_draw(im,deep=0,alpha=.6):
    d=m.Draw(im)
    for x,y,dense in old.positions:
        p=project(x,y,0,deep);q=project(x,y,1,deep)
        line(d,[(p[0],p[1]-40),(q[0],q[1]+8)],(58,112,145),1,alpha*.7)
        for j in range(60):
            z=.35+.65*j/59 if dense else j/59
            pp=project(x,y,z,deep)
            dot(d,pp,m.mix(2.45,1.7,deep),(106,187,221) if dense else (81,155,190),alpha)

def build_arrays():
    full=BG.copy();array_draw(full,0,.47)
    deep=BG.copy();array_draw(deep,1,.65)
    return full,deep

@lru_cache(None)
def sensor_radius(r):return old.dom_sprite(r)
def sensor(im,p,r=45,a=1,flash=0):
    if a<.001 or r<1:return
    x,y=p;d=m.Draw(im)
    line(d,[(x,y-r*2),(x,y+r*2)],(54,111,145),2,a*.65)
    sp=sensor_radius(round(r*m.S))
    if a<.999:
        sp=sp.copy();sp.putalpha(sp.getchannel('A').point(lambda v:round(v*a)))
    im.paste(sp,(round(x*m.S-sp.width/2),round(y*m.S-sp.height/2)),sp)
    if flash>0:
        d=m.Draw(im)
        m.circle(d,x,y,r+8,None,col(ICE,a*flash),3)
        m.circle(d,x,y,r+17,None,col(ICE,a*flash*.3),2)

def clockface(d,p,r,t,a=1,c=ICE):
    x,y=p;m.circle(d,x,y,r,None,col(c,a*.7),2)
    for j in range(12):
        th=j*math.tau/12
        line(d,[(x+math.cos(th)*(r-5),y+math.sin(th)*(r-5)),(x+math.cos(th)*(r-2),y+math.sin(th)*(r-2))],c,1,a*.45)
    th=t*.85-math.pi/2
    line(d,[(x,y),(x+math.cos(th)*r*.75,y+math.sin(th)*r*.75)],c,2,a)
    dot(d,p,2,c,a)

def waveform(d,p,w=115,h=30,a=1,peak=.35,c=ICE):
    x,y=p;line(d,[(x,y),(x+w,y)],c,1,a*.3)
    pts=[]
    for j in range(81):
        u=j/80;value=math.exp(-((u-peak)/.045)**2)-.12*math.exp(-((u-peak-.10)/.1)**2)
        pts.append((x+u*w,y-h*value))
    line(d,pts,c,2,a)

def pathpoint(path,u):
    lens=[float(np.linalg.norm(np.array(q)-p)) for p,q in zip(path,path[1:])]
    left=sum(lens)*m.clamp(u)
    for p,q,L in zip(path,path[1:],lens):
        if left<=L:return vec(p,q,left/L)
        left-=L
    return np.array(path[-1])

def pathlength(path):
    return sum(float(np.linalg.norm(np.array(q)-p)) for p,q in zip(path,path[1:]))

def lightpath(d,path,u,a=1,c=ICE,trail=.22):
    line(d,path,c,1,a*trail)
    if 0<=u<=1:
        p=pathpoint(path,u);tail=pathpoint(path,max(0,u-.045))
        line(d,[tail,p],c,3,a*.7);dot(d,p,4,WHITE,a)

# One continuous event. A rearward cone crosses six DOMs on two fixed strings.
EVENT_START=40.;EVENT_SPEED=83.;DOM_RADIUS=39.
EVENT_A=np.array([925.,935.]);EVENT_DIR=np.array([.08,-math.sqrt(1-.08**2)])
EVENT_NORMAL=np.array([-EVENT_DIR[1],EVENT_DIR[0]])
CONE_SLOPE=Q/SIN;CONE_LENGTH=740.
METRES_PER_PIXEL=.15
NS_PER_VIDEO_SECOND=EVENT_SPEED*METRES_PER_PIXEL/(.99*.299792458)
HITS=[]
for i,p in enumerate(NODES):
    w=np.array(p)-EVENT_A;s=float(w@EVENT_DIR);rho=abs(float(w@EVENT_NORMAL))
    # A finite drawn DOM flashes at first contact with its near surface.
    center_k=s+rho*SIN/Q
    contact_k=center_k-DOM_RADIUS/Q
    arrival=EVENT_START+contact_k/EVENT_SPEED
    HITS.append(dict(i=i,arrival=arrival,contact_k=contact_k,center_k=center_k,
                     saved_ns=round((arrival-EVENT_START)*NS_PER_VIDEO_SECOND)))
WORLD_ANCHORS=[]
for px,py,pz in [(-3,1,.8),(-1,-1,.28),(1,0,.78),(3,-1,.3),(4,1,.66)]:
    x,y,dense=min(old.positions,key=lambda p:(p[0]-px)**2+(p[1]-py)**2)
    z0=.35 if dense else 0
    j=round(m.clamp((pz-z0)/(1-z0))*59)
    WORLD_ANCHORS.append((x,y,z0+(1-z0)*j/59))
TIMING_ANCHORS=[]
for px in [-2.5,2.5]:
    x,y,dense=min((p for p in old.positions if not p[2]),key=lambda p:(p[0]-px)**2+(p[1]-1)**2)
    for j in [9,30,51]:TIMING_ANCHORS.append((x,y,j/59))

# Schematic event display: select existing DOMs by distance from the visible
# track, rather than five unrelated anchors. This is a visual attenuation
# model, not a detector-response simulation or a metric ice absorption length.
POINT_VERTEX=A+DIR*285
POINT_END=A+DIR*1370
POINT_LENGTH=float(np.linalg.norm(POINT_END-POINT_VERTEX))
POINT_SPEED=POINT_LENGTH/9
POINT_HITS=[]
POINT_CANDIDATES=[]
_rng=np.random.default_rng(2072026)
for x,y,dense in old.positions:
    for j in range(60):
        z=.35+.65*j/59 if dense else j/59
        p=project(x,y,z)
        w=p-POINT_VERTEX
        along=float(w@DIR)
        distance=float(np.linalg.norm(w-DIR*np.clip(along,0,POINT_LENGTH)))
        probability=.98*math.exp(-(distance/43)**1.8) if distance<130 else 0
        selected=_rng.random()<probability
        POINT_CANDIDATES.append(dict(distance=distance,selected=selected))
        if not selected:continue
        # The light arrives after the muon passes the nearest track point.
        arrival=10.5+float(np.clip(along,0,POINT_LENGTH))/POINT_SPEED+distance/(POINT_SPEED*Q)
        POINT_HITS.append(dict(p=p,world=(x,y,z),distance=distance,
                               arrival=arrival,energy=math.exp(-distance/90)))
POINT_HITS.sort(key=lambda h:h['arrival'])
POINT_FOCUS=min((h for h in POINT_HITS if h['distance']<12),
                key=lambda h:float(np.linalg.norm(h['p']-np.array([1040,520]))))

def neutrino(d,p,a=1):
    txt(d,p[0],p[1]-36,'ν',41,ICE,a);txt(d,p[0]+16,p[1]-23,'μ',20,ICE,a)

def pointing(t):
    fade=1-m.ramp(t,25.5,28.5)
    im=Image.blend(BG,ARRAY,m.ramp(t,0,1.7)*fade);d=m.Draw(im)
    vertex=A+DIR*285;end=A+DIR*1370
    nu_start=np.array([155.,925.]);u=m.clamp((t-3)/7.5)
    nu=vec(nu_start,vertex,u)
    if t<11.5:
        lightpath(d,[nu_start,vertex],u,fade*(1-m.ramp(t,10.5,11.3)),ICE,.32)
        neutrino(d,nu,fade*(1-m.ramp(t,10.5,11.3)))
    else:
        dashed(d,nu_start,vertex,ICE,fade*.55)
    if 10.3<t<12.2:
        a=math.sin(math.pi*m.clamp((t-10.3)/1.9))
        m.circle(d,*vertex,12+(t-10.3)*14,None,col(ICE,a*fade),2)
        for j in range(5):
            angle=-1+j*.55
            line(d,[vertex,vertex+np.array([math.cos(angle),math.sin(angle)])*(25+7*j)],ICE,1,a*fade*.6)
    if t>=10.5:
        u=m.clamp((t-10.5)/9);head=vec(vertex,end,u)
        line(d,[vertex,head],MU,3,fade*.85)
        head_alpha=fade*(1-m.ramp(t,19.5,20.5))
        dot(d,head,6,MU,head_alpha)
        txt(d,head[0]+15,head[1]+37,'μ⁻',35,MU,head_alpha)
        for hit in POINT_HITS:
            age=t-hit['arrival']
            if age<0:continue
            p=hit['p'];energy=hit['energy']
            pulse=math.exp(-(age/.43)**2)
            strength=(.80+.20*pulse)*energy*fade
            # Persistent cores preserve individually readable DOMs;
            # a brief increase in radius marks the detection front.
            dot(d,p,2.5+1.0*energy+.6*pulse,ICE,strength)
            dot(d,p,1.35,WHITE,strength*.9)
    if t>21.5:
        a=m.ramp(t,21.5,24)*fade
        arrow(d,end,nu_start,ICE,2,a*.9,13)
        m.circle(d,*nu_start,21,None,col(ICE,a),2)
    # The highlighted central DOM grows into the timing close-up.
    if t>25.5:
        f=m.ramp(t,25.5,28.5);p=vec(POINT_FOCUS['p'],(960,460),f)
        sensor(im,p,m.mix(5,100,f),f)
    return im

def decimal_seconds(d,t):
    fade=1-m.ramp(t,36.6,37.8)
    txt(d,960,737,'1 ns = 10⁻⁹ s',49,WHITE,m.ramp(t,29.8,31)*fade)
    # Reveal fixed-place digits instead of briefly displaying a wrong equality.
    value='0.000000001 s';size=84
    widths=[m.font(size).getlength(ch)/m.S for ch in value]
    x=960-sum(widths)/2
    for j,ch in enumerate(value):
        if j<2:start=31.1+j*.12
        elif j<=9:start=31.6+(j-2)*.30
        else:start=34.25+(j-10)*.16
        u=m.ramp(t,start,start+.32)
        if u>0:d.text((x,861+22*(1-u)),ch,size,col(ICE,u*fade),anchor='lm')
        x+=widths[j]

def event_head(t):
    return EVENT_A+EVENT_DIR*EVENT_SPEED*(t-EVENT_START)

def moving_cone(im,t):
    if t<EVENT_START or t>=54.4:return
    a=m.ramp(t,40,41)*(1-m.ramp(t,52.8,54.4))
    head=event_head(t);base=head-EVENT_DIR*CONE_LENGTH
    radius=CONE_LENGTH*CONE_SLOPE
    left=base-EVENT_NORMAL*radius;right=base+EVENT_NORMAL*radius
    lay=m.layer();d=m.Draw(lay)
    d.polygon([tuple(head),tuple(left),tuple(right)],(57,159,215,round(26*a)))
    for p in [left,right]:
        m.line(d,[tuple(head),tuple(p)],(100,211,255,round(30*a)),12)
        m.line(d,[tuple(head),tuple(p)],(125,222,255,round(190*a)),2)
    # A faint rear rim gives the cone depth. No travelling surface markers.
    rim=[]
    for j in range(161):
        th=j*math.tau/160
        p=base+EVENT_NORMAL*radius*math.cos(th)+EVENT_DIR*radius*.10*math.sin(th)
        rim.append(tuple(p))
    m.line(d,rim,(100,202,244,round(48*a)),1)
    # Keep the event separate from the saved-time display below it.
    d.rectangle((0,0,1920,45),fill=(0,0,0,0))
    d.rectangle((0,827,1920,1080),fill=(0,0,0,0))
    m.composite(im,lay)
    if 48<head[1]<825:
        d=m.Draw(im)
        dot(d,head,13,MU,a*.18);dot(d,head,6,MU,a)
        txt(d,head[0]+46,head[1]+13,'μ⁻',36,MU,a)

def timing(t):
    im=BG.copy();spread=m.ramp(t,37,39.6)
    positions=[vec((960,460),p,spread) for p in NODES]
    moving_cone(im,t)
    d=m.Draw(im)
    cables=m.ramp(t,38.1,39.6)
    for xx in [670,1250]:
        x=m.mix(960,xx,spread)
        line(d,[(x,m.mix(260,163,spread)),(x,m.mix(660,790,spread))],(63,133,170),2,cables)
    for i,p in enumerate(positions):
        a=1 if i==2 else m.ramp(t,38+i*.035,39.35+i*.035)
        if a<=.001:continue
        radius=m.mix(100,DOM_RADIUS,spread) if i==2 else DOM_RADIUS
        hit=HITS[i];arrived=t>=hit['arrival']
        flash=(.20+.80*math.exp(-((t-hit['arrival'])/.34)**2)) if arrived else 0
        sensor(im,p,radius,a,flash)
        d=m.Draw(im)
        side=-1 if i<3 else 1
        target_clock=np.array([NODES[i][0]+side*205,NODES[i][1]])
        cp=vec((960,610),target_clock,spread)
        clock_radius=m.mix(44,27,spread)
        if t>=40:m.circle(d,*cp,clock_radius+5,(9,27,45))
        clockface(d,cp,clock_radius,t,m.ramp(t,29,30.5)*a,COLORS[i])
        if spread>.8:txt(d,p[0]+side*(radius+20),p[1],str(i+1),23,COLORS[i],a)
        if arrived:
            # Saved values freeze; all six local clock hands keep running.
            d.rounded_rectangle((cp[0]-82,cp[1]+32,cp[0]+82,cp[1]+76),6,(9,27,45))
            txt(d,cp[0],cp[1]+54,f"{hit['saved_ns']} ns",31,COLORS[i],a)
    decimal_seconds(d,t)
    d=m.Draw(im)
    if 39<t<42:
        a=m.ramp(t,39,39.6)*(1-m.ramp(t,40.5,42))
        line(d,[(465,126),(1455,126)],ICE,1,a*.35)
        for xx in [465,1455]:line(d,[(xx,126),(xx,213)],ICE,1,a*.35)
    if t>=EVENT_START:
        now=round((t-EVENT_START)*NS_PER_VIDEO_SECOND)
        txt(d,352,106,f't = {now} ns',33,WHITE,m.ramp(t,40,41))
        for hit in HITS:
            i=hit['i'];y=874+i*23
            line(d,[(420,y),(1600,y)],COLORS[i],1,.15)
            txt(d,382,y,str(i+1),21,COLORS[i],.6)
            if t>=hit['arrival']:
                tx=420+hit['saved_ns']*2.40
                line(d,[(tx,y+5),(tx,y-12)],COLORS[i],2,1)
                dot(d,(tx,y-12),3,COLORS[i])
        arrow(d,(420,1023),(1635,1023),ICE,1,.5,9);txt(d,1688,1023,'t',28,ICE)
    if t>53.5:
        a=m.ramp(t,53.5,56.8)
        begin=EVENT_A+EVENT_DIR*117;end=EVENT_A+EVENT_DIR*865
        # The reconstruction is the very same particle trajectory, not a replay.
        line(d,[begin,end],MU,3,a*.9)
        arrow(d,end-EVENT_DIR*70,end,MU,3,a*.9,12)
    return im

def surface(d,a=1):
    pts=[(x,161+7*math.sin(x/440)+3*math.sin(x/113)) for x in range(80,1841,8)]
    d.polygon(pts+[(1840,190),(80,190)],col((32,72,98),a*.65))
    line(d,pts,ICE,2,a*.8)
    for y0,opacity in [(330,.08),(465,.10),(545,.07)]:
        pts=[(x,y0+11*math.sin(x/510+y0)) for x in range(80,1841,14)]
        line(d,pts,ICE,1,a*opacity)

def depth(t):
    im=DEEP.copy();d=m.Draw(im);surface(d)
    # Long uninstrumented cables above the array communicate overburden.
    for x,y,dense in old.positions[::6]:
        p=project(x,y,0,1);line(d,[(p[0],173),(p[0],p[1]-30)],(44,93,124),1,.35)
    if 64<t<70.5:
        a=m.ramp(t,64,65)*(1-m.ramp(t,69.3,70.5))
        m.circle(d,280,74,24,None,col(GOLD,a*.8),2)
        for j in range(8):
            th=j*math.tau/8;line(d,[(280+32*math.cos(th),74+32*math.sin(th)),(280+45*math.cos(th),74+45*math.sin(th))],GOLD,2,a*.7)
        for j in range(9):
            x=350+j*135;u=((t-64)*.4+j*.08)%1
            y=100+u*250;strength=a*(1-m.ramp(y,180,350))
            line(d,[(x,y-18),(x,y)],GOLD,2,strength)
    if t>=68:
        tt=t-68
        root=np.array([1440.,95.]);start=np.array([1500.,10.])
        lightpath(d,[start,root],tt/1.1,.65,WHITE,.2)
        for j in range(13):
            end=np.array([850+j*58.,300+(j*77)%180])
            mid=vec(root,end,.42)+np.array([(-1)**j*25,0])
            u=(tt-1.1-j*.06)/3.3
            if u>=0:
                line(d,[root,mid,end],WHITE,1,.10*(1-m.ramp(tt,6,8)))
                if u<=1:lightpath(d,[root,mid,end],u,.6*(1-m.ramp(u,.72,1)),WHITE,.1)
        # A residual high-energy atmospheric muon can reach the array.
        if tt>2:
            path=[root,(1300,430),(1150,875)];u=(tt-2)/6
            lightpath(d,path,u,.42,MU,.17)
            if 0<u<1:
                p=pathpoint(path,u);txt(d,p[0]+24,p[1],'μ⁻',26,MU,.55)
    if t>=74:
        a=m.ramp(t,74,75.5)
        for y,s in [(610,'≈ 1.5 km'),(860,'≈ 2.5 km')]:
            line(d,[(1640,y),(1660,y)],ICE,1,a*.6);txt(d,1740,y,s,27,ICE,a*.75)
    return im

RECEIVERS=[(1040,290),(1430,555),(1590,815)]
ORIGIN=(340,605)
DIRECT=[ORIGIN,RECEIVERS[1]]
ABSORBED=[ORIGIN,(825,700)]
SCATTERED=[ORIGIN,(640,450),(800,520),(1050,220),RECEIVERS[0]]

def optics(t):
    im=BG.copy();d=m.Draw(im)
    for i,p in enumerate(RECEIVERS):
        flash=0
        if 80.5<t<89:
            hit=80.8+i*.55+pathlength([ORIGIN,p])/320
            flash=.7*math.exp(-((t-hit)/.5)**2)
        if t>=90:
            if i==0:
                hit=92+pathlength(SCATTERED)/175
                flash=max(flash,.8*math.exp(-((t-hit)/.5)**2))
            if i==1:
                hit=90+pathlength(DIRECT)/175
                flash=max(flash,.7*math.exp(-((t-hit)/.5)**2))
        sensor(im,p,57,1,flash)
    d=m.Draw(im)
    # Light was generated by the passing muon; it then travels to remote DOMs.
    dot(d,ORIGIN,7,MU);txt(d,310,655,'μ⁻',37,MU)
    arrow(d,(210,665),(470,530),MU,2,.4,11)
    if t<89.5:
        for i,p in enumerate(RECEIVERS):
            start=80.8+i*.55;end=start+pathlength([ORIGIN,p])/320
            lightpath(d,[ORIGIN,p],(t-start)/(end-start),1-m.ramp(t,88.5,89.5),ICE,.25)
        if t>82:
            a=m.ramp(t,82,83)*(1-m.ramp(t,88,89.5))
            line(d,[(340,940),(1430,940)],ICE,1,a*.6)
            for x in [340,1430]:line(d,[(x,933),(x,947)],ICE,1,a*.6)
            txt(d,885,981,'100 m',34,ICE,a)
    if t>=89:
        for j in range(14):
            p=(535+(j*103)%680,335+(j*67)%400)
            dot(d,p,2,(73,121,149),m.ramp(t,89,92)*.4)
        lightpath(d,DIRECT,(t-90)/(pathlength(DIRECT)/175),.7,ICE,.18)
        u=(t-92)/(pathlength(ABSORBED)/175)
        a=1-m.ramp(u,.68,1)
        lightpath(d,ABSORBED,u,a,MU,.26)
        if .7<u<1.1:
            p=ABSORBED[-1];m.circle(d,*p,8+18*(u-.7),None,col(MU,(1-m.ramp(u,.7,1.1))*.3),1)
        # Same propagation speed; a bent path to this DOM arrives later.
        lightpath(d,[ORIGIN,RECEIVERS[0]],(t-92)/(pathlength([ORIGIN,RECEIVERS[0]])/175),.3,ICE,.07)
        arrival=92+pathlength(SCATTERED)/175
        lightpath(d,SCATTERED,(t-92)/(arrival-92),.95,WHITE,.35)
        if t>=arrival:waveform(d,(980,420),125,26,.7)
    return im

def leds(d,p,r,t,a=1):
    x,y=p;cy=y-r*.30;rx=r*.70;ry=r*.30
    d.ellipse((x-rx-12,cy-ry-10,x+rx+12,cy+ry+10),col((17,52,75),a),col((72,132,158),a*.8),2)
    # Twelve clearly separated schematic LED packages; every LED is visible.
    for j in range(12):
        th=j*math.tau/12;px=x+rx*math.cos(th);py=cy+ry*math.sin(th)
        reveal=m.ramp(t,102+j*.30,102.4+j*.30)
        flash=.45+.55*math.exp(-((t-(103+j*.30))/.26)**2)
        rr=max(3,r*.031)
        d.rounded_rectangle((px-rr-3,py-rr-3,px+rr+3,py+rr+3),2,col((62,121,153),a*.7))
        dot(d,(px,py),rr,ICE,a*reveal*flash)
    d.ellipse((x-rx*.65,cy-ry*.57,x+rx*.65,cy+ry*.57),None,col(ICE,a*.20),1)

def flasher_close(t):
    im=BG.copy();u=m.ramp(t,99,102)
    p=vec(RECEIVERS[1],(960,540),u);r=m.mix(57,225,u)
    sensor(im,p,r)
    leds(m.Draw(im),p,r,t,m.ramp(t,101.5,103))
    txt(m.Draw(im),960,897,'12',48,ICE,m.ramp(t,104.5,106)*(1-m.ramp(t,109,110.5)))
    return im

CAL_SOURCE=(420,530)
CAL_RECEIVERS=[(1010,280),(1300,525),(1530,745)]
CAL_PATHS=[ [CAL_SOURCE,CAL_RECEIVERS[0]], [CAL_SOURCE,(705,470),(860,610),(1020,465),CAL_RECEIVERS[1]], [CAL_SOURCE,CAL_RECEIVERS[2]] ]

def calibration(t):
    im=BG.copy();u=m.ramp(t,109.3,112.3);p=vec((960,540),CAL_SOURCE,u);r=m.mix(225,73,u)
    phase=(t-112.5)%4.0
    flash=math.exp(-(phase/.20)**2) if t>=112.5 else 0
    sensor(im,p,r,1,flash)
    leds(m.Draw(im),p,r,110,1)
    txt(m.Draw(im),960,897,'12',48,ICE,1-m.ramp(t,109,110.5))
    for i,node in enumerate(CAL_RECEIVERS):
        show=m.ramp(t,110+i*.2,112.3+i*.2)
        hit=pathlength(CAL_PATHS[i])/400
        reception=math.exp(-((phase-hit)/.16)**2) if t>=112.5 else 0
        sensor(im,node,53,show,reception*.9)
    d=m.Draw(im)
    if t>=112.5:
        for i,path in enumerate(CAL_PATHS):
            duration=pathlength(path)/400
            lightpath(d,path,phase/duration,1,COLORS[i],.20)
            if phase>=duration:
                x,y=CAL_RECEIVERS[i];waveform(d,(x-65,y+131),130,27,.8,COLORS.index(COLORS[i])*.07+.30,COLORS[i])
        # Some light is lost: calibration does not erase absorption or scattering.
        lost=[CAL_SOURCE,(735,690)]
        duration=pathlength(lost)/400
        lightpath(d,lost,phase/duration,1-m.ramp(phase,duration*.65,duration),MU,.15)
    # A measured arrival distribution and a fitted response retain the late tail.
    a=m.ramp(t,122,124)
    if a>0:
        x0=630;y0=983;w=970;h=105
        arrow(d,(x0,y0),(x0+w,y0),ICE,1,a*.6,9)
        txt(d,x0+w+31,y0,'Δt',27,ICE,a)
        def curve(z):
            main=.76*math.exp(-((z-.25)/.052)**2)
            tail=.33*math.exp(-(z-.28)/.23)*m.ramp(z,.23,.30) if z>.23 else 0
            return main+tail
        fit=m.ramp(t,125,129)
        pts=[(x0+w*j/150,y0-h*curve(j/150)) for j in range(151)]
        line(d,pts,ICE,2,a*fit)
        for j in range(36):
            z=.035+j*.026
            if z<=m.ramp(t,122.5,127.5):
                measured=curve(z)*(1+.025*math.sin(j*3.1))
                dot(d,(x0+w*z,y0-h*measured),3,WHITE,a)
        clockface(d,(394,933),37,t,a)
        if t>=129:
            b=m.ramp(t,129,130.5)
            line(d,[(470,934),(482,946),(509,917)],ICE,3,b)
    return im

def cross(f,g,t,start,end):
    u=m.ramp(t,start,end)
    return Image.blend(f(t),g(t),u)

def transition_to_depth(t):
    u=m.ramp(t,58.5,62)
    im=Image.blend(BG,DEEP,u);d=m.Draw(im);surface(d,u)
    for i,p in enumerate(NODES):
        q=project(*TIMING_ANCHORS[i],1)
        sensor(im,vec(p,q,u),m.mix(DOM_RADIUS,3,u),1-u)
    return im

def frame(t):
    if t<28.5:return pointing(t)
    if t<58.5:return timing(t)
    if t<62:return cross(timing,transition_to_depth,t,58.5,62)
    if t<78.5:return depth(t)
    if t<81:return cross(depth,optics,t,78.5,81)
    if t<99:return optics(t)
    if t<101:return cross(optics,flasher_close,t,99,101)
    if t<109.3:return flasher_close(t)
    return calibration(t)

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'timing.json').write_text(json.dumps(dict(version=3,duration=DURATION,fps=30,width=round(m.W*m.S),height=round(m.H*m.S),audio=False,enclosing_cube=False,paragraph_boundaries=[28.5,58.5],timing='Estimated Turkish narration; no audio recording supplied.',pointing_event=dict(model='schematic projected-distance attenuation',hits=len(POINT_HITS),probability='0.98 * exp(-(distance/43)^1.8)',note='Logical image distances, not metres or a calibrated detector response'),timing_event=dict(particles=1,strings=2,doms=6,cone='rearward wavefront envelope',saved_times='illustrative relative nanoseconds; not real IceCube data',dom_geometry='schematic; not engineering spacing'),cues=[dict(start=a,end=b,narration=s,visual=v) for a,b,s,v in CUES],illustrative_hit_times=[dict(sensor=h['i']+1,contact_video_seconds=h['arrival'],saved_ns=h['saved_ns']) for h in HITS]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.srt')).write_text('\n\n'.join(f'{i+1}\n{old.stamp(a)} --> {old.stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(CUES))+'\n',encoding='utf-8')

def previews():
    times=[10.8,16,20.8,24,28.5,32.3,33.5,35.3,39.8,43,44.95,45.85,47.65,48.2,50.35,50.55,53.5,57.5,61,75,85,97.5,106,111,115,126,131]
    sheet=Image.new('RGB',(1920,3240))
    for i,t in enumerate(times):
        im=frame(t);im.save(OUT/f'kare_{t:05.1f}.jpg',quality=94)
        sheet.paste(im.resize((640,360),Image.Resampling.LANCZOS),(i%3*640,i//3*360))
    sheet.save(OUT/'kontrol.jpg',quality=94)
    frame(20.8).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',type=int,choices=[1,2],default=2);p.add_argument('--preview',action='store_true');args=p.parse_args()
    m.configure(args.scale);BG=m.bg();ARRAY,DEEP=build_arrays();metadata();previews()
    if not args.preview:m.encode(frame,OUT/(NAME+('_4k' if args.scale==2 else '_1080p')+'.mp4'),DURATION,OUT/(NAME+'_onizleme_1080p.mp4') if args.scale==2 else None)
