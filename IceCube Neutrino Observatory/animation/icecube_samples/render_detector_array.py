# Copyright (c) 2026 Ogetay Kayali. See the repository LICENSE.md.
"""Text-free revision of the 32-second array paragraph; preserves v1 outputs."""
from pathlib import Path
import argparse, json, math
import numpy as np
from PIL import Image, ImageDraw
import legacy_rendering_helpers as shared

OUT=shared.ROOT/'exports'/'01_icecube_detector_array'
NAME='01_icecube_detector_array'
W,H=shared.W,shared.H
ease,ramp,mix=shared.ease,shared.ramp,shared.mix
line,circle=shared.line,shared.circle
ICE,WHITE=shared.ICE,shared.WHITE
POS=shared.positions
# Two actual neighboring standard strings in the front row.
front=sorted((x,y) for x,y,dense in POS if not dense and y==max(p[1] for p in POS))
PAIR=front[len(front)//2-1:len(front)//2+1]
assert len(PAIR)==2 and abs(PAIR[1][0]-PAIR[0][0]-1)<.001

def bg():
    y,x=np.mgrid[0:H,0:W]
    g=np.exp(-(((x-960)/930)**2+((y-530)/740)**2))
    a=np.zeros((H,W,3),np.uint8)
    for c,v in enumerate(shared.NAVY): a[:,:,c]=v+g*[4,14,21][c]
    return Image.fromarray(a)
BG=bg()

def blend_color(c,a):
    return tuple(int(mix(b,v,a)) for b,v in zip((9,28,48),c))

def snow(im,opacity):
    """Only an open ice surface, never a box or volume boundary."""
    layer=Image.new('RGBA',(W,H)); d=ImageDraw.Draw(layer)
    pts=[(x,104+7*math.sin(x/430)+3*math.sin(x/130)) for x in range(100,1821,8)]
    d.polygon(pts+[(1820,143),(100,143)],fill=(100,176,210,int(12*opacity)))
    d.line(pts,fill=(156,220,243,int(165*opacity)),width=2)
    im.paste(layer,(0,0),layer)

def single_string(t):
    im=BG.copy(); d=ImageDraw.Draw(im)
    # All 60 modules, then a continuous camera move into the same two modules.
    zoom_in=ramp(t,3.35,4.6); zoom_out=ramp(t,6.5,8.6)
    close=zoom_in*(1-zoom_out)
    separation=math.exp(mix(math.log(12.2),math.log(330),close))
    depth=ramp(t,7.8,9.3)
    separation=mix(separation,326/59,depth)
    center=mix(540,753,depth)
    top=center-29.5*separation; bottom=center+29.5*separation
    radius=mix(4.5,69,close)
    radius=mix(radius,3.5,depth)
    surface=ramp(t,7.9,9)
    if surface: snow(im,surface); d=ImageDraw.Draw(im)
    line(d,[(960,105 if surface else 0),(960,H)],(67,128,158),max(2,int(mix(2,5,close))))
    for j in range(60):
        y=top+j*separation
        if y<-radius-8 or y>H+radius+8: continue
        # At the opening, a traveling glint reveals the repeated modules.
        a=.48+.52*ramp(t,.12+j*.032,.5+j*.032)
        if radius>10:
            shared.dom(im,960,y,int(radius)); d=ImageDraw.Draw(im)
        else:
            circle(d,960,y,radius,blend_color(ICE,a))
            circle(d,959,y-1,max(1,radius*.3),WHITE)
    # A soft traveling light emphasizes equal vertical spacing without a ruler.
    if 4.8<t<6.25:
        u=ramp(t,4.8,6.25)
        for j in [29,30]:
            p=top+j*separation
            pulse=math.exp(-((u-(j-29))/.25)**2)
            if pulse>.03:
                circle(d,960,p,radius+12+pulse*6,None,blend_color(ICE,pulse),2)
    # Depth is conveyed by a long empty cable below the surface before the DOMs.
    if 9.3<t<11.5:
        u=ramp(t,9.3,10.9)
        y=mix(top,bottom,u)
        shared.glow(im,[(960,y,.8)],6)
    return im

def camera(t):
    a=ramp(t,11.5,14.4)
    yaw=.26+.16*ramp(t,19.2,23.8)
    # Gentle parallax gives the array volume without a enclosing geometry.
    yaw+=.045*ramp(t,28.2,32)
    return a,yaw,mix(590,243,a),mix(326,585,a)

def project(x,y,z,t):
    spread,yaw,top,height=camera(t)
    xx=x*math.cos(yaw)+y*math.sin(yaw)
    yy=-x*math.sin(yaw)+y*math.cos(yaw)
    perspective=1/(1+yy*.025*spread)
    return np.array([960+xx*118*spread*perspective,top+yy*30*spread+z*height*perspective])

def array(t):
    im=BG.copy(); d=ImageDraw.Draw(im)
    spread,yaw,top,height=camera(t)
    surf=1-ramp(t,11.5,12.7)
    if surf>0: snow(im,surf); d=ImageDraw.Draw(im)
    focus=ramp(t,15.2,16.1)*(1-ramp(t,18.4,19.4))
    # Hide newly added strings until their separation is visible.
    order=sorted(enumerate(POS),key=lambda k:-k[1][0]*math.sin(yaw)+k[1][1]*math.cos(yaw))
    for k,(x,y,dense) in order:
        a=ramp(t,11.65+(k/86)*.65,13.5+(k/86)*.65)
        emphasis=(x,y) in PAIR
        light=mix(1,.25,focus) if not emphasis else 1
        p=project(x,y,0,t); q=project(x,y,1,t)
        wire=blend_color((50,105,137),a*light)
        if emphasis and focus: wire=blend_color(ICE,focus)
        line(d,[(p[0],mix(104,p[1]-58,spread)),(q[0],q[1]+12)],wire,2 if emphasis and focus>.2 else 1)
        for j in range(60):
            z=(.35+.65*j/59) if dense else j/59
            px,py=project(x,y,z,t)
            col=(108,196,226) if dense else (82,161,194)
            if emphasis: col=tuple(int(mix(c,v,focus)) for c,v in zip(col,ICE))
            circle(d,px,py,2.7 if dense else 3,blend_color(col,a*light))
    # Retain the first cable through the transition; fade as array takes over.
    hero=1-ramp(t,12,13.5)
    if hero>0:
        line(d,[(960,mix(104,top-58,spread)),(960,top+height+10)],blend_color(ICE,hero),2)
        for j in range(60): circle(d,960,top+height*j/59,3.5,blend_color(ICE,hero))
    # Sequential glints across neighboring strings convey horizontal spacing.
    if 16<t<18.8:
        u=ramp(t,16,18.8)
        for n,(x,y) in enumerate(PAIR):
            pulse=math.exp(-((u-(.22+.52*n))/.21)**2)
            if pulse>.015:
                pts=[(*project(x,y,j/59,t),pulse*.85) for j in range(0,60,3)]
                shared.glow(im,pts,4)
    if t>=23.8:
        d=ImageDraw.Draw(im)
        u=ramp(t,24.1,27.65)
        start=np.array([-4,-2,.93]); end=np.array([4,2,.08])
        head=start+(end-start)*u
        # The reconstructed path remains thin and secondary to recorded light.
        p=project(*start,t); q=project(*head,t)
        line(d,[tuple(p),tuple(q)],(113,208,242),3)
        hits=[]
        for j in range(15):
            fraction=j/14
            if fraction>=u: continue
            source=start+(end-start)*fraction
            # Pick a real DOM on the nearby cable, then draw the light path.
            x,y,dense=min(POS,key=lambda pt:(pt[0]-source[0])**2+(pt[1]-source[1])**2)
            z0=.35 if dense else 0
            idx=int(np.clip(round((source[2]-z0)/(1-z0)*59),0,59))
            sensor=np.array([x,y,z0+(1-z0)*idx/59])
            ep=project(*source,t); sp=project(*sensor,t)
            line(d,[tuple(ep),tuple(sp)],(44,97,126),1)
            strength=.35+.65*math.exp(-((u-fraction)/.11)**2)
            hits.append((*sp,strength))
        if hits: shared.glow(im,hits,8)
    return im

def scene(t):
    return single_string(t) if t<11.5 else array(t)

VISUALS=[
 'A single string appears with 60 DOMs lighting up in sequence.',
 'The camera moves closer to two DOMs on the same string, emphasizing their spacing.',
 'The camera pulls back to reveal the ice surface, the long cable, and the deeply buried modules.',
 'One string expands into a schematic array of 86 strings.',
 'Two neighboring standard strings light up while the surrounding array remains dim.',
 'A slow camera rotation reveals the volume through parallax, without an enclosing cube.',
 'A representative track advances, followed by signals at nearby DOM positions in the schematic array.',
 'The completed track and detector array hold for the transition.'
]

def metadata():
    OUT.mkdir(parents=True,exist_ok=True)
    cues=[dict(start=a,end=b,narration=s,visual=v) for (a,b,s,_),v in zip(shared.CUES['01_icecube_detector_array_legacy'],VISUALS)]
    (OUT/'timing.json').write_text(json.dumps(dict(version=2,width=W,height=H,fps=30,duration=32,audio=False,narration_language="en",on_screen_text=False,enclosing_cube=False,timing='Editorial cue timings; English narration is not aligned to recorded audio.',cues=cues),ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/(NAME+'.en.srt')).write_text('\n\n'.join(f'{i+1}\n{shared.stamp(c["start"])} --> {shared.stamp(c["end"])}\n{c["narration"]}' for i,c in enumerate(cues))+'\n',encoding='utf-8')

def previews():
    ts=[2.8,5.2,10,14.8,17.3,22,26,30]
    sheet=Image.new('RGB',(1536,864))
    for i,t in enumerate(ts):
        im=scene(t)
        im.save(OUT/f'frame_{t:04.1f}.jpg',quality=93)
        sheet.paste(im.resize((384,216),Image.Resampling.LANCZOS),(i%4*384,i//4*432))
    # Two full rows, no timestamp lettering in the visual assets.
    cropped=Image.new('RGB',(1536,432))
    cropped.paste(sheet.crop((0,0,1536,216)),(0,0))
    cropped.paste(sheet.crop((0,432,1536,648)),(0,216))
    cropped.save(OUT/'contact_sheet.jpg',quality=94)
    scene(25.8).save(OUT/'poster.jpg',quality=94)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--preview',action='store_true');a=p.parse_args()
    metadata();previews()
    if not a.preview:
        shared.OUT=OUT
        shared.SCENES[NAME+'_1080p']=scene
        shared.CUES[NAME+'_1080p']=shared.CUES['01_icecube_detector_array_legacy']
        shared.render(NAME+'_1080p')
