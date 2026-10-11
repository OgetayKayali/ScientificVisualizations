# Copyright (c) 2026 Ogetay Kayali. See the repository LICENSE.md.
"""Photon -> Standard Model table -> neutrinos -> cosmic detection."""
from pathlib import Path
import argparse, json, math
import numpy as np
from PIL import Image
import motion_common as m
import legacy_rendering_helpers as old
from render_neutrino_direction_and_ice_calibration import line, dot, col, txt, vec
from render_electromagnetic_radiation_applications import wave
from render_icecube_history import array_art, ARRAY_POINTS

OUT=Path(__file__).resolve().parents[2]/'exports'/'10_standard_model_and_cosmic_neutrinos'
NAME='10_standard_model_and_cosmic_neutrinos'
DURATION=38
BG=None
ICE=m.ICE;WHITE=m.WHITE;MU=(167,187,255)
QUARK=(171,176,238);LEPTON=(125,222,235);BOSON=(121,185,243);HIGGS=(223,207,160)
COLS=[475,710,945,1245,1480]
ROWS=[252,442,654,844]
# Seventeen conventional slots; g groups gluon states and W± groups W charges.
PARTICLES=[
 ('u',0,0,QUARK),('c',1,0,QUARK),('t',2,0,QUARK),
 ('d',0,1,QUARK),('s',1,1,QUARK),('b',2,1,QUARK),
 ('e⁻',0,2,LEPTON),('μ⁻',1,2,LEPTON),('τ⁻',2,2,LEPTON),
 ('nu_e',0,3,LEPTON),('nu_mu',1,3,LEPTON),('nu_tau',2,3,LEPTON),
 ('g',3,0,BOSON),('γ',3,1,BOSON),('Z⁰',3,2,BOSON),('W±',3,3,BOSON),
 ('H',4,0,HIGGS),
]
CUES=[
 (0,3,'All of this is possible because of light.','A light wave and a γ symbol appear.'),
 (3,8,'Light consists of photons, which are elementary particles.','The γ symbol is highlighted inside a large particle tile.'),
 (8,15,'But there are many other elementary particles, each with different properties.','The photon tile moves into the Standard Model table as the other 16 tiles appear; particle families are highlighted.'),
 (15,21,'Different particles can provide different kinds of information about the universe.','The full table remains visible while the photon and three neutrino tiles stand out.'),
 (21,26,'This leads to the central idea of neutrino astronomy:','The three neutrino tiles move from their table positions to the foreground.'),
 (26,38,'detecting neutrinos from some of the most energetic events in the universe.','A schematic cosmic source emits a muon neutrino, which produces a muon in ice and lights nearby DOMs.'),
]

def symbol(d,p,key,size=72,c=WHITE,a=1):
    x,y=p
    if key.startswith('nu_'):
        sub={'nu_e':'e','nu_mu':'μ','nu_tau':'τ'}[key]
        txt(d,x-size*.12,y,'ν',size,c,a)
        txt(d,x+size*.28,y+size*.27,sub,size*.43,c,a)
    else:txt(d,x,y,key,size,c,a)

def tile(d,p,key,c,w=190,h=164,a=1,highlight=0,show_symbol=True):
    if a<.001:return
    x,y=p
    d.rounded_rectangle((x-w/2,y-h/2,x+w/2,y+h/2),16,col(c,a*(.045+.055*highlight)),col(c,a*(.45+.55*highlight)),2+highlight)
    line(d,[(x-w/2+21,y-h/2+13),(x+w/2-21,y-h/2+13)],c,2,a*.36)
    if show_symbol:symbol(d,p,key,min(w,h)*.43,c if highlight<.5 else WHITE,a)
    if highlight>.001:
        d.rounded_rectangle((x-w/2-7,y-h/2-7,x+w/2+7,y+h/2+7),21,None,col(c,a*highlight*.22),2)

def photon(t):
    im=BG.copy();d=m.Draw(im)
    a=m.ramp(t,0,.65);u=m.ramp(t,2.1,4.2)
    wave(d,(350,540),(1570,540),t,113,27,ICE,a*(1-u))
    p=(960,540)
    # The symbol is a visual identifier, not a literal photon shape.
    tile(d,p,'γ',BOSON,m.mix(205,272,u),m.mix(184,238,u),u,1,False)
    symbol(d,(960,m.mix(419,540,u)),'γ',m.mix(89,238*.43,u),WHITE,a)
    if t>4.2:
        wave(d,(445,540),(775,540),t,94,19,ICE,.65*(1-m.ramp(t,7,8)))
        wave(d,(1145,540),(1475,540),t,94,19,ICE,.65*(1-m.ramp(t,7,8)))
    return im

def table(t):
    im=BG.copy();d=m.Draw(im)
    settle=m.ramp(t,8,10.8)
    for i,(key,cc,rr,c) in enumerate(PARTICLES):
        target=np.array([COLS[cc],ROWS[rr]],float)
        if key=='γ':
            p=vec((960,540),target,settle)
            w=m.mix(272,190,settle);h=m.mix(238,164,settle)
            a=1;highlight=1-m.ramp(t,11.1,12)
        else:
            delay=8.9+.105*i
            a=m.ramp(t,delay,delay+.85)
            p=target+np.array([0,25*(1-a)])
            w=190;h=164;highlight=0
        # Brief family highlights, never fictitious particle trajectories/spins.
        if 11.3<t<15.3:
            family=0 if rr<2 and cc<3 else 1 if cc<3 else 2 if cc==3 else 3
            center=11.7+family*.88
            highlight=max(highlight,math.exp(-((t-center)/.42)**2)*.72)
        if t>=15.3:
            focus=m.ramp(t,15.3,18)
            messenger=key=='γ' or key.startswith('nu_')
            if messenger:highlight=max(highlight,focus)
            else:a*=1-focus*.64
        tile(d,p,key,c,w,h,a,highlight)
    # Restrained separators preserve familiar table grouping without headings.
    fade=m.ramp(t,10.7,12)*(1-m.ramp(t,15.3,18)*.65)
    line(d,[(355,548),(1065,548)],ICE,1,fade*.21)
    line(d,[(1116,156),(1116,938)],ICE,1,fade*.21)
    return im

def select_neutrinos(t):
    im=BG.copy();d=m.Draw(im);u=m.ramp(t,21,24.2)
    # Draw the entire background first, including its divider lines.
    for key,cc,rr,c in PARTICLES:
        if key.startswith('nu_'):continue
        p=np.array([COLS[cc],ROWS[rr]],float)
        tile(d,p,key,c,190,164,(1-u)*(1 if key=='γ' else .36),1-u if key=='γ' else 0)
    line(d,[(355,548),(1065,548)],ICE,1,(1-u)*.21*.35)
    line(d,[(1116,156),(1116,938)],ICE,1,(1-u)*.21*.35)
    # Moving neutrino cards form the foreground, above Z, W and all dividers.
    for key,cc,rr,c in PARTICLES:
        if not key.startswith('nu_'):continue
        p=vec((COLS[cc],ROWS[rr]),(625+cc*335,531),u)
        tile(d,p,key,c,m.mix(190,241,u),m.mix(164,215,u),1,1)
    return im

SOURCE=np.array([360.,247.5])
VERTEX=np.array([1020.,547.])
END=np.array([1670.,842.])
DIR=(END-VERTEX)/np.linalg.norm(END-VERTEX)
LENGTH=float(np.linalg.norm(END-VERTEX));SPEED=LENGTH/4.7
DETECTOR_CENTER=np.array([1320.,695.]);SCALE=.62
DETECTOR_POINTS=[(p-np.array([960,550]))*SCALE+DETECTOR_CENTER for _,p,j in ARRAY_POINTS]
HITS=[]
_rng=np.random.default_rng(102026)
for p in DETECTOR_POINTS:
    w=p-VERTEX;along=float(w@DIR);near=np.clip(along,0,LENGTH)
    distance=float(np.linalg.norm(w-DIR*near))
    if distance<91 and _rng.random()<.98*math.exp(-(distance/29)**1.8):
        HITS.append(dict(p=p,distance=distance,arrival=30+near/SPEED+distance/(SPEED*.7576)))

def source_art(d,t,a):
    x,y=SOURCE
    # Generic energetic jet source, not an identification of a particular object.
    for j in range(4):
        r=53+j*16
        d.ellipse((x-r,y-r*.29,x+r,y+r*.29),None,col(ICE,a*(.53-j*.08)),2)
    dot(d,SOURCE,17,ICE,a*.18);dot(d,SOURCE,7,WHITE,a)
    axis=np.array([.91,.415]);normal=np.array([-axis[1],axis[0]])
    for side in [-1,1]:
        for j in [-1,0,1]:
            p=SOURCE+axis*side*26
            q=SOURCE+axis*side*156+normal*j*14
            line(d,[p,q],ICE,2,a*(.65 if j==0 else .24))
    for px,py,r in [(230,224,2),(590,289,1.5),(170,419,2),(655,167,2),(710,483,1.5),(475,191,1.5),(262,550,1.5)]:
        dot(d,(px,py),r,WHITE,a*.45)

def cosmic(t):
    im=BG.copy();d=m.Draw(im)
    a=m.ramp(t,26,27.3)
    source_art(d,t,a)
    array_art(d,1,DETECTOR_CENTER,SCALE,a*.72)
    line(d,[(978,453),(1671,453)],ICE,1,a*.19)
    # One incoming muon neutrino; it does not light DOMs before interacting.
    u=m.clamp((t-27)/3)
    p=vec(SOURCE+DIR*160,VERTEX,u)
    if 27<=t<30.6:
        vis=a*(1-m.ramp(t,30,30.6))
        dot(d,p,6,ICE,vis)
        symbol(d,p+np.array([0,-39]),'nu_mu',43,ICE,vis)
        line(d,[SOURCE+DIR*160,p],ICE,1,vis*.28)
    if t>=30:
        u=m.clamp((t-30)/4.7);head=vec(VERTEX,END,u)
        line(d,[VERTEX,head],MU,3,.95)
        vis=1-m.ramp(t,34.7,35.6)
        dot(d,head,6,MU,vis)
        symbol(d,head+np.array([12,-31]),'μ⁻',35,MU,vis)
        burst=math.exp(-((t-30)/.36)**2)
        m.circle(d,*VERTEX,12+(t-30)*11,None,col(ICE,burst),2)
        for h in HITS:
            age=t-h['arrival']
            if age<0:continue
            pulse=math.exp(-(age/.31)**2)
            intensity=(.79+.21*pulse)*math.exp(-h['distance']/75)
            dot(d,h['p'],2.2+.65*pulse,ICE,intensity)
            dot(d,h['p'],.85,WHITE,intensity)
    return im

SCENES=[(0,photon),(8,table),(21,select_neutrinos),(26,cosmic)]
def frame(t):
    # Photon/table and table/neutrino transitions share exact starting geometry.
    if t<8:return photon(t)
    if t<21:return table(t)
    if t<26:return select_neutrinos(t)
    if t<27:return Image.blend(select_neutrinos(26),cosmic(t),m.ramp(t,26,27))
    return cosmic(t)

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'timing.json').write_text(json.dumps(dict(duration=DURATION,fps=30,width=round(1920*m.S),height=round(1080*m.S),audio=False,narration_language="en",on_screen_text='Universal particle symbols only',timing='Estimated narration timing; no voice recording supplied',table_slots=[p[0] for p in PARTICLES],cues=[dict(start=a,end=b,narration=s,visual=v) for a,b,s,v in CUES]),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.en.srt')).write_text('\n\n'.join(f'{i+1}\n{old.stamp(a)} --> {old.stamp(b)}\n{s}' for i,(a,b,s,v) in enumerate(CUES))+'\n',encoding='utf-8')

def previews():
    times=[1.5,5.5,9.5,11.5,13,15,18.5,21.5,24.5,27.5,29,30.5,32.5,34.5,37]
    sheet=Image.new('RGB',(1920,1800))
    for i,t in enumerate(times):
        im=frame(t);im.save(OUT/f'frame_{t:04.1f}.jpg',quality=94)
        sheet.paste(im.resize((640,360),Image.Resampling.LANCZOS),(i%3*640,i//3*360))
    sheet.save(OUT/'contact_sheet.jpg',quality=94)
    frame(13).save(OUT/'poster.jpg',quality=95)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--scale',type=int,choices=[1,2],default=2);p.add_argument('--preview',action='store_true');args=p.parse_args()
    m.configure(args.scale);BG=m.bg();metadata();previews()
    if not args.preview:m.encode(frame,OUT/(NAME+('_4k' if args.scale==2 else '_1080p')+'.mp4'),DURATION,OUT/(NAME+'_1080p.mp4') if args.scale==2 else None)
