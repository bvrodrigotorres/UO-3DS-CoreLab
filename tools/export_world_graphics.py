#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Private full-map graphics for the experimental read-only expedition mode."""
import argparse,struct,json,hashlib,subprocess,concurrent.futures,shutil
from pathlib import Path
from export_scene import find,land_image,static_image,pixels555

def main():
 p=argparse.ArgumentParser();p.add_argument('--client',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
 if a.out.exists():raise FileExistsError(a.out)
 from PIL import Image
 from ultima_sdk import Files,Art
 Files.set_directory(str(a.client));assert Art.initialize()
 with a.archive.open('rb') as f:
  h=struct.unpack('<4sHH8I3Q',f.read(64));assert h[:3]==(b'UOW1',1,64)
  nl,ns,count=h[7:10];f.seek(64);meta=[struct.unpack('<QHH',f.read(12)) for _ in range(nl)]
  f.seek(h[12]);es=[struct.unpack('<QIIII',f.read(24)) for _ in range(count)]
  lands=set();statics=set()
  for off,length,crc,n,first in es:
   f.seek(off);b=f.read(length)
   for y in range(64):
    for x in range(64):lands.add(struct.unpack_from('<H',b,(y*65+x)*3)[0])
   for art,x,y,z,hue in struct.iter_unpack('<HBBbH',b[12675:]):statics.add(art)
 images=[];dedup={};mapping={};missing=[]
 def add(kind,art,image):
  if image is None or image.getbbox() is None:raise ValueError(f'Empty graphic {kind}/{art}')
  key=(image.size,hashlib.sha256(image.tobytes()).digest())
  if key not in dedup:dedup[key]=len(images);images.append(image)
  mapping[(kind,art)]=dedup[key]
 for i,art in enumerate(sorted(lands)):
  if art==2:continue
  try:add(0,art,land_image(Art,art))
  except ValueError:missing.append(['land',art]);add(0,art,Image.new('RGBA',(44,44),(35,45,35,255)))
 for art in sorted(statics):
  try:add(1,art,static_image(Art,art))
  except ValueError:missing.append(['static',art]);add(1,art,Image.new('RGBA',(8,8),(110,65,45,255)))
 # Texture files are optional in this installed client; fall back to original land
 # diamonds, exactly as the bounded scene exporter does. Never use a static image
 # as the texture of a slope.
 idx,tex=find(a.client,'texidx.mul',False),find(a.client,'texmaps.mul',False)
 if idx and tex:
  with idx.open('rb') as fi,tex.open('rb') as ft:
   for tid in sorted({meta[n][1] for n in lands if meta[n][1]}):
    fi.seek(tid*12);r=fi.read(12)
    if len(r)!=12:continue
    off,length,_=struct.unpack('<III',r);side={8192:64,32768:128}.get(length)
    if off==0xffffffff or not side:continue
    ft.seek(off);add(2,tid,pixels555(ft.read(length),side,side))
 a.out.mkdir(parents=True);src=a.out/'_sources';src.mkdir()
 permutation=sorted(range(len(images)),key=lambda i:(max(images[i].size),images[i].width*images[i].height,images[i].width))
 remap={};sizes=[];pages=[]
 for n in range(0,len(permutation),8):
  folder=src/str(n//8);folder.mkdir();group=permutation[n:n+8]
  for i,old in enumerate(group):remap[old]=len(sizes);im=images[old];im.save(folder/f'{i:02d}.png');sizes.append(im.size)
  # Every page has 32 logical slots. This keeps existing renderer addressing.
  for i in range(len(group),32):Image.new('RGBA',(1,1)).save(folder/f'{i:02d}.png');sizes.append((1,1))
  (folder/'page.t3s').write_text('--atlas -f rgba5551 -z none\n'+''.join(f'{i:02d}.png\n' for i in range(32)))
  pages.append((folder,a.out.resolve()/f'atlas_{n//8:02d}.t3x'))
 if len(sizes)>=65535:raise ValueError('Local graphics handle overflow')
 exe=shutil.which('tex3ds') or '/opt/devkitpro/tools/bin/tex3ds'
 def convert(pair):
  folder,out=pair;subprocess.run([exe,'-i','page.t3s','-o',str(out)],cwd=folder,check=True,stdout=subprocess.DEVNULL)
 with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
  for n,_ in enumerate(pool.map(convert,pages)):
   if n%100==0:print(f'{n+1}/{len(pages)} atlas',flush=True)
 costs=[out.stat().st_size for folder,out in pages]
 (a.out/'image_sizes.bin').write_bytes(struct.pack('<4sHH',b'UOI1',len(sizes),len(pages))+b''.join(struct.pack('<HH',*s) for s in sizes)+b''.join(struct.pack('<I',c) for c in costs))
 records=sorted((kind,art,remap[old]) for (kind,art),old in mapping.items())
 body=b''.join(struct.pack('<HHH',*r) for r in records)
 def fnv(b):
  h=2166136261
  for c in b:h=((h^c)*16777619)&0xffffffff
  return h
 (a.out/'graphics.bin').write_bytes(struct.pack('<4sIII',b'UWG1',len(sizes),len(records),fnv(body))+body)
 shutil.copy2(find(a.client,'radarcol.mul'),a.out/'radar.bin')
 report={'unique_images':len(images),'logical_images':len(sizes),'atlas_pages':len(pages),'atlas_bytes':sum(costs),'missing_original_graphics':missing,'slope_texture_files_available':bool(idx and tex),'geometry_catalogue':h[10],'limitations':['Original statics and terrain only; no server population or dynamic decoration','Hues retained by geometry but use original uncolored sprite, matching existing scene exporter']}
 (a.out/'graphics-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
 shutil.rmtree(src)
if __name__=='__main__':main()
