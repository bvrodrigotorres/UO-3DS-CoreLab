#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Software preview of the actual C++ command list. NOT a 3DS screenshot."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]


def triangle(canvas, source, p, uv):
    # CPU reference rasterization, independent of the native GPU backend.
    det=(p[1][1]-p[2][1])*(p[0][0]-p[2][0])+(p[2][0]-p[1][0])*(p[0][1]-p[2][1])
    if abs(det)<1e-8:return
    left=max(0,int(min(x for x,y in p)));right=min(399,int(max(x for x,y in p))+1)
    top=max(0,int(min(y for x,y in p)));bottom=min(239,int(max(y for x,y in p))+1)
    pix=canvas.load();src=source.load()
    for y in range(top,bottom+1):
        for x in range(left,right+1):
            fx,fy=x+.5,y+.5
            a=((p[1][1]-p[2][1])*(fx-p[2][0])+(p[2][0]-p[1][0])*(fy-p[2][1]))/det
            b=((p[2][1]-p[0][1])*(fx-p[2][0])+(p[0][0]-p[2][0])*(fy-p[2][1]))/det
            c=1-a-b
            if min(a,b,c)<-1e-8:continue
            u=a*uv[0][0]+b*uv[1][0]+c*uv[2][0];v=a*uv[0][1]+b*uv[1][1]+c*uv[2][1]
            color=src[min(source.width-1,max(0,int(u*source.width))),min(source.height-1,max(0,int(v*source.height)))]
            if color[3]:pix[x,y]=color


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('scene',type=Path)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--x',type=int,default=13);p.add_argument('--y',type=int,default=16);p.add_argument('--z',type=int,default=20)
    p.add_argument('--all-heights',action='store_true')
    args=p.parse_args()
    with tempfile.TemporaryDirectory() as t:
        tool=Path(t)/'trace'
        subprocess.run(['c++','-std=c++17','-O2','-I'+str(ROOT/'include'),str(ROOT/'tools/trace.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),'-o',str(tool)],check=True)
        raw=subprocess.check_output([str(tool),str(args.scene),str(args.x),str(args.y),str(args.z),str(int(not args.all_heights))],text=True)
    trace=json.loads(raw)
    canvas=Image.new('RGBA',(400,240),(20,33,47,255))
    def project(x,y,z):return 200+(x-args.x-y+args.y)*22,126+(x-args.x+y-args.y)*22-(z-args.z)*4
    cache={}
    for entry in trace['entries']:
        kind=entry['kind'];x,y,z=entry['x'],entry['y'],entry['z']
        if kind==2:
            sx,sy=project(args.x,args.y,args.z);draw=ImageDraw.Draw(canvas)
            draw.rectangle((sx-5,sy-24,sx+4,sy),fill=(51,115,242));draw.rectangle((sx-4,sy-32,sx+3,sy-25),fill=(242,191,140));continue
        image_id=entry['image']
        if image_id==65535:continue
        if image_id not in cache:cache[image_id]=Image.open(args.scene.parent/'_private_sources'/f'{image_id//32:02d}'/f'{image_id%32:02d}.png').convert('RGBA')
        image=cache[image_id]
        if kind==1:
            sx,sy=project(x,y,z);canvas.alpha_composite(image,(round(sx-image.width/2),round(sy+22-image.height)))
        else:
            h=entry['corners'];vertices=[project(x,y,h[0]),project(x+1,y,h[1]),project(x+1,y+1,h[2]),project(x,y+1,h[3])];vertices=[(px,py-22) for px,py in vertices]
            uv=[(0,0),(1,0),(1,1),(0,1)] if entry['texture'] else [(.5,0),(1,.5),(.5,1),(0,.5)]
            if abs(h[0]-h[2])>abs(h[3]-h[1]):vertices=vertices[1:]+vertices[:1];uv=uv[1:]+uv[:1]
            for ids in ((0,1,2),(2,3,0)):triangle(canvas,image,[vertices[i] for i in ids],[uv[i] for i in ids])
    result=Image.new('RGB',(800,510),(20,33,47));result.paste(canvas.resize((800,480),Image.Resampling.NEAREST),(0,30))
    ImageDraw.Draw(result).text((10,10),'CoreLab software preview | C++ list | not a 3DS capture',fill='white')
    args.out.parent.mkdir(parents=True,exist_ok=True);result.save(args.out)
    args.out.with_suffix('.json').write_text(json.dumps(trace,indent=2)+'\n')
    print(json.dumps({'entries':len(trace['entries']),'available_directions':trace['available_directions'],'preview':str(args.out)}))


if __name__=='__main__':main()
