#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build tiny private RGBA4444 previews from the installed original art catalogue."""
import argparse,struct,json
from pathlib import Path
from export_scene import find,land_image,static_image,pixels555,tiledata,HEAD,LAND,OBJECT

def fnv(data):
 h=2166136261
 for v in data:h=((h^v)*16777619)&0xffffffff
 return h

def write_previews(images,metadata,out,opaque=()):
 from PIL import Image
 body=bytearray()
 for image,im in enumerate(images):
  # RGBA filtering uses premultiplied alpha, avoiding black edges on foliage.
  tiny=im.resize((4,4),Image.Resampling.BOX)
  pixels=list(tiny.getdata());weight=sum(p[3] for p in pixels)
  mean=[sum(p[c]*p[3] for p in pixels)//max(1,weight) for c in range(3)]
  for r,g,b,a in pixels:
   if image in opaque:
    if not a:r,g,b=mean
    a=255 # Geometry already clips land diamonds; avoid translucent seams.
   body.extend(struct.pack('<H',(r>>4)<<12|(g>>4)<<8|(b>>4)<<4|(a>>4)))
 out.parent.mkdir(parents=True,exist_ok=True)
 out.write_bytes(struct.pack('<4sIII',b'UPV1',len(images),fnv(metadata),fnv(body))+body)

def export(client,source,out):
 from PIL import Image
 from ultima_sdk import Files,Art
 Files.set_directory(str(client));assert Art.initialize()
 metadata=(source/'image_sizes.bin').read_bytes();count,pages=struct.unpack_from('<HH',metadata,4)
 assert metadata[:4]==b'UOI1' and 0<count<=32768 and len(metadata)==8+count*4+pages*4
 sizes=[struct.unpack_from('<HH',metadata,8+i*4) for i in range(count)];mapping={}
 if (source/'graphics.bin').exists():
  b=(source/'graphics.bin').read_bytes();magic,n,records,crc=struct.unpack_from('<4sIII',b);assert magic==b'UWG1' and n==count and fnv(b[16:])==crc
  for kind,art,image in struct.iter_unpack('<HHH',b[16:]):mapping.setdefault(image,(kind,art))
 else:
  b=(source/'scene.bin').read_bytes();head=HEAD.unpack_from(b);width,height,n,objects=head[2:6];assert n==count
  lands,_=tiledata(find(client,'tiledata.mul'));p=32+(width+1)*(height+1)
  for art,image,flags,mode in LAND.iter_unpack(b[p:p+width*height*LAND.size]):
   if image!=65535:mapping.setdefault(image,(2 if mode else 0,lands[art][1] if mode else art))
  p+=width*height*LAND.size
  for x,y,z,h,art,hue,image,flags,order,reserved in OBJECT.iter_unpack(b[p:]):mapping.setdefault(image,(1,art))
  if (source/'doors.bin').exists():
   for index,art,image,dx,dy,pair in struct.iter_unpack('<IHHbbH',(source/'doors.bin').read_bytes()[16:]):mapping.setdefault(image,(1,art))
 idx,tex=find(client,'texidx.mul',False),find(client,'texmaps.mul',False)
 report=json.loads((source/('graphics-report.json' if (source/'graphics-report.json').exists() else 'report.json')).read_text())
 known_missing={(0 if k=='land' else 1,a) for k,a in report.get('missing_original_graphics',[])}
 known_missing.update((0,a) for a in report.get('missing_land_graphics',[]))
 images=[];missing=[]
 for image,size in enumerate(sizes):
  if size==(1,1):images.append(Image.new('RGBA',(1,1)));continue
  if image not in mapping:
   assert size==(1,1),(image,size);images.append(Image.new('RGBA',(1,1)));continue
  kind,art=mapping[image]
  try:
   if (kind,art) in known_missing:raise ValueError('Missing in exported catalogue')
   if kind==0:im=land_image(Art,art)
   elif kind==1:im=static_image(Art,art)
   else:
    if not idx or not tex:raise ValueError('Missing texmaps')
    with idx.open('rb') as fi,tex.open('rb') as ft:
     fi.seek(art*12);off,length,_=struct.unpack('<III',fi.read(12));side={8192:64,32768:128}.get(length)
     if off==0xffffffff or not side:raise ValueError('Missing texmap')
     ft.seek(off);im=pixels555(ft.read(length),side,side)
   assert im.size==size,(image,kind,art,im.size,size)
  except ValueError:
   # Same explicit missing-art swatch as the world exporter; never mis-map IDs.
   im=Image.new('RGBA',size,(35,45,35,255) if kind==0 else (110,65,45,255));missing.append((kind,art))
  images.append(im)
  if image%2000==0:print(image,'/',count,flush=True)
 write_previews(images,metadata,out,{i for i,(kind,art) in mapping.items() if kind==0})
 print(json.dumps({'images':count,'bytes':out.stat().st_size,'missing':missing}),flush=True)
 return images

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--client',type=Path,required=True);p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();export(a.client,a.source,a.out)
