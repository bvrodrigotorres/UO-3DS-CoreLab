#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Private deterministic ServUO regional population and original MUL sprite export."""
import argparse,collections,hashlib,json,re,shutil,struct,subprocess,xml.etree.ElementTree as ET
from pathlib import Path
from export_avatar import decode_frame,DIRECTION_MAP
from export_npcs import hue_image
from export_journal import fnv

# Mobile classes in the pinned reference. These book spawners are not mobiles.
BOOKS={'trades','britanniawaters','maceandblade','lightandmight','artssection','foldedsteel','oilandoubliette','wizardscompendium','understandinganimals','songsofnote','skeletonkey','pasttreasures'}
ANIMALS={'bird':6,'cat':201,'dog':217,'rat':238,'chicken':208}
# Deterministic original-client clothes, grouped by profession. Runtime random
# outfit state of a live server cannot be recovered from static spawn records.
PROFILES={
 'merchant':(400,0x3EA,[(0x170B,0x455),(0x1539,0x59B),(0x1EFD,0x517)],701,0x455),
 'female':(401,0x3EF,[(0x170F,0x455),(0x1516,0x53C),(0x1EFD,0x72C)],701,0x3B3),
 'smith':(400,0x3EA,[(0x170B,0x455),(0x1539,0x455),(0x1517,0x901),(0x153B,0x455)],701,0x455),
 'healer':(400,0x3EA,[(0x170D,0),(0x1F03,0x481)],701,0x455),
 'mage':(400,0x3EA,[(0x170D,0),(0x1F03,0x59)],701,0x455),
 'innkeeper':(400,0x3EA,[(0x170B,0x455),(0x1539,0x59B),(0x1517,0x21),(0x153D,0x47E)],701,0x455),
 'tailor':(401,0x3EF,[(0x170F,0x455),(0x1516,0x53C),(0x1517,0x3F)],701,0x455),
 'bard':(400,0x3EA,[(0x170B,0x455),(0x1539,0x59B),(0x1EFD,0x3F),(0x1713,0x21)],701,0x455),
 'Emilio':(400,0x3EB,[(0x170D,0x721),(0x1539,0x51B),(0x1EFD,0x517),(0x1713,0x584),(0x1541,0x13)],701,0x470),
 'Evan':(400,0x41B,[(0x170F,0x737),(0x152E,0x74C),(0x1EFD,0x535)],701,0x451),
 'Sarakki':(401,0x41E,[(0x170F,0x740),(0x1516,0x53C),(0x1EFD,0x72C)],701,0x1BB),
 'Verity':(401,0x3EF,[(0x170F,0x754),(0x1537,0x901),(0x1517,0x653),(0x1715,0x901)],701,0x3B3),
 'Naturalist':(400,0x3EA,[(0x170B,0),(0x1539,0x59B),(0x1FA1,0x598)],701,0x455)}

def profile(kind):
    k=kind.lower()
    if k in ANIMALS:return k
    for name in ('Emilio','Evan','Sarakki','Verity','Naturalist'):
        if k==name.lower():return name
    if any(t in k for t in ('healer','veterinarian')):return 'healer'
    if any(t in k for t in ('mage','alchemist','herbal')):return 'mage'
    if any(t in k for t in ('smith','armorer','miner','tinker')):return 'smith'
    if any(t in k for t in ('innkeeper','tavern','waiter','cook','barkeeper','baker','butcher')):return 'innkeeper'
    if any(t in k for t in ('tailor','weaver','hairstylist')):return 'tailor'
    if any(t in k for t in ('bard','sirhareus','sirberran','sirfelean')):return 'bard'
    return 'merchant'

def spawn_requests(path,origin,side):
    rows=[];excluded=[];duplicates=0;seen={}
    for node in ET.parse(path).getroot().findall('Points'):
        if node.findtext('Map','Felucca')!='Felucca' or node.findtext('IsRunning','True')=='False' or node.findtext('InContainer','False')=='True':continue
        x,y,z=(int(node.findtext(k)) for k in ('CentreX','CentreY','CentreZ'))
        ax,ay,width,height=(int(node.findtext(k,str(v))) for k,v in zip(('X','Y','Width','Height'),(x,y,0,0)))
        if width<0 or height<0:raise ValueError('Negative spawn rectangle')
        left,top=max(ax,origin[0]),max(ay,origin[1]);right,bottom=min(ax+width,origin[0]+side-1),min(ay+height,origin[1]+side-1)
        if left>right or top>bottom:continue
        uid=node.findtext('UniqueId')
        signature=ET.tostring(node)
        if uid and uid in seen:
            if seen[uid]!=signature:raise ValueError('Conflicting spawner UniqueId')
            duplicates+=1;continue
        if uid:seen[uid]=signature
        parts=[]
        for text in node.findtext('Objects2','').split(':OBJ='):
            fields=text.split(':');kind=fields[0];mx=next((int(f[3:]) for f in fields if f.startswith('MX=')),1)
            if kind.lower() in BOOKS:excluded.append(kind);continue
            if kind and mx>0:parts.append((kind,mx))
        if not parts:continue
        count=max(0,min(int(node.findtext('MaxCount')),sum(n for k,n in parts)))
        counts=[0]*len(parts)
        for slot in range(count):
            # Deterministic fair initial fill; live servers choose entries randomly.
            i=next(i for i in range(len(parts)) if counts[i]<parts[i][1] and counts[i]==min(counts[j] for j in range(len(parts)) if counts[j]<parts[j][1]))
            kind,maximum=parts[i];counts[i]+=1;animal=kind.lower() in ANIMALS
            rows.append(dict(type=kind,x=max(left,min(right,x))-origin[0],y=max(top,min(bottom,y))-origin[1],z=z,roam=max(0,min(120,int(node.findtext('Range')))),animal=animal,profile=profile(kind),source=node.findtext('Name'),spawner=uid,source_center=[x,y,z],bounds=[left-origin[0],top-origin[1],right-origin[0],bottom-origin[1]],original_bounds=[ax,ay,ax+width,ay+height],max_count=int(node.findtext('MaxCount')),min_delay=node.findtext('MinDelay'),max_delay=node.findtext('MaxDelay'),relative_home=node.findtext('IsHomeRangeRelative','True')=='True'))
    return rows,excluded,duplicates

def read_layer(client,body,action,direction):
    entry=(body*110 if body<200 else 22000+(body-200)*65 if body<400 else 35000+(body-400)*175)+action*5+direction
    with (client/'anim.idx').open('rb') as f:f.seek(entry*12);raw=f.read(12)
    if len(raw)!=12:raise ValueError(f'Missing animation index {body}/{action}')
    at,size,_=struct.unpack('<iii',raw)
    if at<0 or not 520<=size<=48*1024*1024:raise ValueError(f'Missing animation {body}/{action}/{direction}')
    with (client/'anim.mul').open('rb') as f:f.seek(at);data=f.read(size)
    if len(data)!=size:raise ValueError('Truncated animation')
    colors=struct.unpack_from('<256H',data);count=struct.unpack_from('<I',data,512)[0]
    if not 1<=count<=200:raise ValueError('Invalid frame count')
    result=[]
    for i in range(count):
        pos=512+struct.unpack_from('<I',data,516+4*i)[0];cx,cy,w,h=struct.unpack_from('<hhhh',data,pos)
        from PIL import Image
        image=Image.new('RGBA',(1,1)) if w==0 and h==0 else decode_frame(data,pos,shared_palette=colors)
        result.append((image,cx,cy))
    return result

def main():
    from PIL import Image,ImageOps
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('client','reference','scene','out'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args()
    if a.out.exists():raise FileExistsError(a.out)
    raw=a.scene.read_bytes();h=struct.unpack_from('<4sHHHHIiiHHhH',raw);rows,excluded,duplicates=spawn_requests(a.reference,(h[6],h[7]),h[2])
    names=sorted(set(r['profile'] for r in rows));a.out.mkdir(parents=True)
    tile=(a.client/'tiledata.mul').read_bytes();n=8;start=512*(4+32*(n+22));stride=4+32*(n+33)
    def anim(art):return struct.unpack_from('<H',tile,start+(art//32)*stride+4+(art%32)*(n+33)+n+6)[0]
    hues=(a.client/'hues.mul').read_bytes();cache={};styles=[]
    for style,name in enumerate(names):
        animal=name in ANIMALS
        if animal:body,skin,clothes,hair,hairhue=ANIMALS[name],0,[],None,0
        else:body,skin,clothes,hair,hairhue=PROFILES[name]
        layers=[(body,skin)]+[(anim(art),hue) for art,hue in clothes]+([(hair,hairhue)] if hair else [])
        idle=1 if body<200 else 2 if body<400 else 4;frames=[]
        for direction,mirror in DIRECTION_MAP:
            for phase in range(5):
                action=idle if phase==0 else 0;canvas=Image.new('RGBA',(128,144))
                for graphic,hue in layers:
                    key=(graphic,action,direction)
                    if key not in cache:cache[key]=read_layer(a.client,graphic,action,direction)
                    seq=cache[key];image,cx,cy=seq[0 if phase==0 else (phase-1)*len(seq)//4]
                    if hue:image=hue_image(image,hues,hue&0x3FFF)
                    canvas.alpha_composite(image,(64-cx,139-image.height-cy))
                if mirror:canvas=ImageOps.mirror(canvas)
                frames.append(canvas)
        boxes=[im.getbbox() for im in frames];bounds=(min(b[0] for b in boxes),min(b[1] for b in boxes),max(b[2] for b in boxes),max(b[3] for b in boxes))
        folder=a.out/f'style{style:02d}';folder.mkdir()
        for i,im in enumerate(frames):im.crop(bounds).save(folder/f'{i:02d}.png')
        (folder/'npc.t3s').write_text('--atlas -f rgba5551 -z none\n'+''.join(f'{i:02d}.png\n' for i in range(40)))
        target=(a.out/f'pop_{style:02d}.t3x').resolve()
        subprocess.run([shutil.which('tex3ds') or '/opt/devkitpro/tools/bin/tex3ds','-i','npc.t3s','-o',str(target)],cwd=folder,check=True)
        # Mirroring 128-wide canvas changes the 64 anchor by one pixel, as in
        # the legacy client mirroring convention; the common footprint is fixed.
        styles.append(dict(name=name,body=body,pivot=[64-bounds[0],139-bounds[1]],size=[bounds[2]-bounds[0],bounds[3]-bounds[1]],bytes=target.stat().st_size,checksum=fnv(target.read_bytes()),layers=layers))
    for row in rows:row['style']=names.index(row['profile'])
    (a.out/'requests.json').write_text(json.dumps(rows,indent=2))
    with (a.out/'requests.bin').open('wb') as f:
        for row in rows:f.write(struct.pack('<HHhHHH32s',row['x'],row['y'],row['z'],row['style'],row['roam'],int(row['animal']),row['type'].upper().encode('ascii')[:31].ljust(32,b'\0'))+struct.pack('<4H',*row['bounds']))
    report=dict(count=len(rows),humans=sum(not r['animal'] for r in rows),animals=sum(r['animal'] for r in rows),styles=styles,excluded_objects=excluded,duplicate_spawners=duplicates,source_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),source_revision='ServUO pub57 d76bf4443cf76d081ddaf8f57c87ff33749256af',scene_fnv=fnv(raw))
    (a.out/'manifest.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k!='styles'},indent=2))
    placer=a.out/'place-population'
    root=Path(__file__).resolve().parents[1]
    subprocess.run(['c++','-std=c++17','-I'+str(root/'include'),str(root/'tools/place_population.cpp'),str(root/'source/core.cpp'),str(root/'source/scene_io.cpp'),str(root/'source/npcs.cpp'),str(root/'source/doors.cpp'),'-o',str(placer.resolve())],check=True)
    placed=a.out/'placed.bin'
    subprocess.run([str(placer.resolve()),str(a.scene.resolve()),str((a.out/'requests.bin').resolve()),str(placed.resolve())],check=True)
    payload=b''.join(struct.pack('<hhHHII',*s['pivot'],*s['size'],s['bytes'],s['checksum']) for s in styles)+placed.read_bytes()
    (a.out/'population.bin').write_bytes(struct.pack('<4sHHII',b'UNP1',len(rows),len(styles),fnv(raw),fnv(payload))+payload)
if __name__=='__main__':main()
