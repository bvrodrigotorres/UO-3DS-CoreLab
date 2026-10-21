#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Software preview of the actual C++ command list. NOT a 3DS screenshot."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
from PIL import Image, ImageDraw, ImageChops

ROOT=Path(__file__).resolve().parents[1]


def triangle(canvas, source, p, uv,owner=None,owner_value=0,protected=None,support=False):
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
            if color[3]:
                if protected is not None:
                    if support and protected.getpixel((x,y)):continue
                    protected.putpixel((x,y),0)
                pix[x,y]=color
                if owner is not None:owner.putpixel((x,y),owner_value)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('scene',type=Path)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--x',type=int,default=13);p.add_argument('--y',type=int,default=16);p.add_argument('--z',type=int,default=20)
    p.add_argument('--all-heights',action='store_true')
    p.add_argument('--outfit-dir',type=Path)
    p.add_argument('--avatar-frame',type=int,default=0)
    p.add_argument('--from-x',type=int);p.add_argument('--from-y',type=int);p.add_argument('--from-z',type=int)
    p.add_argument('--progress',type=float,default=1)
    p.add_argument('--depth-mask',action='store_true')
    args=p.parse_args()
    startX=args.x if args.from_x is None else args.from_x
    startY=args.y if args.from_y is None else args.from_y
    startZ=args.z if args.from_z is None else args.from_z
    t=max(0,min(1,args.progress))
    cameraX=startX+(args.x-startX)*t;cameraY=startY+(args.y-startY)*t;footZ=startZ+(args.z-startZ)*t
    with tempfile.TemporaryDirectory() as tempdir:
        tool=Path(tempdir)/'trace'
        subprocess.run(['c++','-std=c++17','-O2','-I'+str(ROOT/'include'),str(ROOT/'tools/trace.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),'-o',str(tool)],check=True)
        raw=subprocess.check_output([str(tool),str(args.scene),str(args.x),str(args.y),str(args.z),str(int(not args.all_heights)),str(footZ)],text=True)
    trace=json.loads(raw)
    canvas=Image.new('RGBA',(400,240),(20,33,47,255))
    def project(x,y,z):return 200+(x-cameraX-y+cameraY)*22,126+(x-cameraX+y-cameraY)*22-(z-footZ)*4
    owner=Image.new("L",(400,240));avatarMask=Image.new("L",(400,240));afterActor=False
    protected=Image.new("L",(400,240)) if args.depth_mask else None
    cache={}
    for entry in trace['entries']:
        kind=entry['kind'];x,y,z=entry['x'],entry['y'],entry['z']
        if kind==2:
            afterActor=True
            sx,sy=200,126
            if args.outfit_dir:
                avatar=Image.open(args.outfit_dir/f'{args.avatar_frame:02d}.png').convert('RGBA')
                pos=(round(sx-avatar.width/2),round(sy+5-avatar.height))
                canvas.alpha_composite(avatar,pos);mask=avatar.getchannel('A').point(lambda v:255 if v>127 else 0)
                avatarMask.paste(mask,pos);owner.paste(1,pos,mask)
                if protected is not None:protected.paste(255,pos,mask)
                continue
            draw=ImageDraw.Draw(canvas)
            draw.rectangle((sx-5,sy-24,sx+4,sy),fill=(51,115,242));draw.rectangle((sx-4,sy-32,sx+3,sy-25),fill=(242,191,140));continue
        image_id=entry['image']
        if image_id==65535:continue
        if image_id not in cache:cache[image_id]=Image.open(args.scene.parent/'_private_sources'/f'{image_id//32:02d}'/f'{image_id%32:02d}.png').convert('RGBA')
        image=cache[image_id]
        if kind==1:
            sx,sy=project(x,y,z);pos=(round(sx-image.width/2),round(sy+22-image.height))
            flags=entry.get('flags',0);height=entry.get('height',0)
            # Same local character cutaway as the native renderer; no global floor pass.
            left,top=pos;right=left+image.width;bottom=top+image.height
            upper=bool(flags&((1<<28)|0x200)) and z>=footZ+14
            wall=bool(flags&0x10) and z+height>footZ+5
            alpha=.18 if afterActor and not flags&(1<<29) and (upper or wall) and left<213 and right>187 and top<131 and bottom>81 else 1
            if alpha<1:image=image.copy();image.putalpha(image.getchannel('A').point(lambda v:round(v*alpha)))
            mask=image.getchannel('A').point(lambda v:255 if v>0 else 0)
            if protected is not None:
                if entry.get('support'):
                    region=protected.crop((pos[0],pos[1],pos[0]+image.width,pos[1]+image.height))
                    mask=ImageChops.subtract(mask,region)
                    image=image.copy();image.putalpha(ImageChops.multiply(image.getchannel('A'),mask))
                    protected.paste(0,pos,mask)
                elif alpha==1:protected.paste(0,pos,mask)
            canvas.alpha_composite(image,pos)
            walkable=bool(flags&(0x200|0x400)) and not flags&(0x10|(1<<29))
            value=2 if walkable and entry['priority']+height<=max(startZ,args.z)+1 else 3 if walkable else 0
            owner.paste(value,pos,image.getchannel('A').point(lambda v:255 if v>127 else 0))
        else:
            h=entry['corners'];vertices=[project(x,y,h[0]),project(x+1,y,h[1]),project(x+1,y+1,h[2]),project(x,y+1,h[3])];vertices=[(px,py-22) for px,py in vertices]
            uv=[(0,0),(1,0),(1,1),(0,1)] if entry['texture'] else [(.5,0),(1,.5),(.5,1),(0,.5)]
            if abs(h[0]-h[2])>abs(h[3]-h[1]):vertices=vertices[1:]+vertices[:1];uv=uv[1:]+uv[:1]
            for ids in ((0,1,2),(2,3,0)):triangle(canvas,image,[vertices[i] for i in ids],[uv[i] for i in ids],owner,2 if entry["priority"]<=max(startZ,args.z)+1 else 3,protected,entry.get("support",False))
    trace['support_pixels_covering_avatar']=sum(ImageChops.multiply(owner.point(lambda v:255 if v==2 else 0),avatarMask).histogram()[1:])
    trace['higher_surface_pixels_covering_avatar']=sum(ImageChops.multiply(owner.point(lambda v:255 if v==3 else 0),avatarMask).histogram()[1:])
    result=Image.new('RGB',(800,510),(20,33,47));result.paste(canvas.resize((800,480),Image.Resampling.NEAREST),(0,30))
    ImageDraw.Draw(result).text((10,10),'CoreLab software preview | C++ list | not a 3DS capture',fill='white')
    args.out.parent.mkdir(parents=True,exist_ok=True);result.save(args.out)
    args.out.with_suffix('.json').write_text(json.dumps(trace,indent=2)+'\n')
    print(json.dumps({'entries':len(trace['entries']),'available_directions':trace['available_directions'],'preview':str(args.out),'support_pixels':trace['support_pixels_covering_avatar'],'higher_surface_pixels':trace['higher_surface_pixels_covering_avatar']}))


if __name__=='__main__':main()
