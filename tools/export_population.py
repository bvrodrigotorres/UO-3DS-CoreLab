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
 'Regina':(401,0x3EE,[(0x170B,0),(0x230E,0)],902,0x599),
 'Naturalist':(400,0x3EA,[(0x170B,0),(0x1539,0x59B),(0x1FA1,0x598)],701,0x455)}

def profile(kind):
    k=kind.lower()
    if k in ANIMALS:return k
    for name in ('Emilio','Evan','Sarakki','Verity','Naturalist','Regina'):
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

def creature_definition(kind,directory):
    files={p.stem.lower():p for p in directory.rglob('*.cs')};path=files.get(kind.lower())
    if not path:return None
    text=path.read_text();body=re.search(r'(?:Body|BodyValue)\s*=\s*(0x[0-9A-Fa-f]+|[0-9]+)',text)
    if not body:body=re.search(r'(?:Body|BodyValue)\s*=\s*Utility.RandomList\(\s*(0x[0-9A-Fa-f]+|[0-9]+)',text)
    if not body and re.search(r'class\s+\w+\s*:\s*BaseMount',text):body=re.search(r':\s*base\([^,]+,\s*(0x[0-9A-Fa-f]+|[0-9]+)\s*,',text)
    if not body:return None
    body=int(body[1],0)
    if body>=400:return None
    def pair(method,default):
        match=re.search(r'\b'+method+r'\(\s*(\d+)(?:\s*,\s*(\d+))?\s*\)',text)
        return (int(match[1]),int(match[2] or match[1])) if match else default
    hp=pair('SetHits',(12,12));damage=pair('SetDamage',(2,4))
    return dict(body=body,aggressive=bool(re.search(r':\s*base\([^)]*FightMode\.Closest',text,re.S)),hits=(hp[0]+hp[1])//2,damage=list(damage),swims=bool(re.search(r'CanSwim\s*=\s*true',text)),waterOnly=bool(re.search(r'CantWalk\s*=\s*true',text)),stationary=bool(re.search(r'DisallowAllMoves\s*=\s*true|override bool DisallowAllMoves.*?return true',text,re.S)))

def composition_layers(body,skin,clothes,hair,hair_hue,animation,item_layer):
    # Hair remains above normal clothes; headgear must cover the scalp last.
    ordinary=[(animation(art),hue) for art,hue in clothes if item_layer(art)!=6]
    headgear=[(animation(art),hue) for art,hue in clothes if item_layer(art)==6]
    return [(body,skin)]+ordinary+([(hair,hair_hue)] if hair else [])+headgear

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
        image=Image.new('RGBA',(1,1)) if w==0 and h==0 else decode_frame(data,pos,shared_palette=colors,max_size=512)
        result.append((image,cx,cy))
    return result

def main():
    from PIL import Image,ImageOps
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('client','reference','scene','out'):p.add_argument('--'+key,type=Path,required=True)
    p.add_argument('--require-complete',action='store_true');p.add_argument('--previous',type=Path);p.add_argument('--creatures',type=Path);p.add_argument('--regions',type=Path)
    a=p.parse_args()
    if a.out.exists():raise FileExistsError(a.out)
    raw=a.scene.read_bytes();h=struct.unpack_from('<4sHHHHIiiHHhH',raw);rows,excluded,duplicates=spawn_requests(a.reference,(h[6],h[7]),h[2])
    aliases={}
    for line in (a.client/'Body.def').read_text().splitlines():
        m=re.match(r'\s*(\d+)\s+\{(\d+)\}\s+(\d+)',line)
        if m:aliases[int(m[1])]=(int(m[2]),int(m[3]))
    creatures={};aquatic=[]
    if a.creatures:
        kept=[]
        for row in rows:
            kind=row['type'].lower()
            definition=creature_definition(kind,a.creatures)
            if definition:
                definition['source_body']=definition['body'];definition['hue']=0
                if definition['body'] in aliases:definition['body'],definition['hue']=aliases[definition['body']]
                creatures[kind]=definition;row.update(animal=True,profile=kind,aggressive=definition['aggressive'],hits=definition['hits'],damage=definition['damage'],stationary=definition['stationary'],swims=definition['swims'],waterOnly=definition['waterOnly'])
            kept.append(row)
        rows=kept
    if a.previous:
        previous=json.loads((a.previous/'requests.json').read_text());pending={}
        for row in rows:pending.setdefault((row['spawner'],row['type'].lower()),[]).append(row)
        first=[]
        for old in previous:
            key=(old['spawner'],old['type'].lower())
            if not pending.get(key):raise ValueError('Previous mobile missing: '+str(key))
            first.append(pending[key].pop(0))
        used={id(r) for r in first};rows=first+[r for r in rows if id(r) not in used]
    guards=[]
    if a.regions:
        for facet in ET.parse(a.regions).getroot():
            if facet.attrib.get('name')!='Felucca':continue
            for region in facet.iter('region'):
                if region.attrib.get('name') in ('Britain','Blackthorn Castle'):
                    for rect in region.findall('rect'):guards.append([int(rect.attrib[n]) for n in ('x','y','width','height')])
    names=sorted(set(r['profile'] for r in rows));a.out.mkdir(parents=True)
    (a.out/'guarded.json').write_text(json.dumps(guards))
    tile=(a.client/'tiledata.mul').read_bytes();n=8;start=512*(4+32*(n+22));stride=4+32*(n+33)
    def anim(art):return struct.unpack_from('<H',tile,start+(art//32)*stride+4+(art%32)*(n+33)+n+6)[0]
    def item_layer(art):return tile[start+(art//32)*stride+4+(art%32)*(n+33)+n+1]
    hues=(a.client/'hues.mul').read_bytes();cache={};styles=[]
    for style,name in enumerate(names):
        animal=name in ANIMALS or name in creatures
        if animal:body,skin,clothes,hair,hairhue=(creatures[name]['body'] if name in creatures else ANIMALS[name]),creatures[name].get('hue',0) if name in creatures else 0,[],None,0
        else:body,skin,clothes,hair,hairhue=PROFILES[name]
        layers=composition_layers(body,skin,clothes,hair,hairhue,anim,item_layer)
        idle=1 if body<200 else 2 if body<400 else 4;frames=[]
        for direction,mirror in DIRECTION_MAP:
            for phase in range(5):
                action=idle if phase==0 else 0;canvas=Image.new('RGBA',(192,192))
                for graphic,hue in layers:
                    key=(graphic,action,direction)
                    if key not in cache:
                        try:cache[key]=read_layer(a.client,graphic,action,direction)
                        except ValueError:
                            if not a.creatures or not animal or action!=0:raise
                            cache[key]=read_layer(a.client,graphic,idle,direction)
                    seq=cache[key];image,cx,cy=seq[0 if phase==0 else (phase-1)*len(seq)//4]
                    if hue:image=hue_image(image,hues,hue&0x3FFF)
                    canvas.alpha_composite(image,(96-cx,187-image.height-cy))
                if mirror:canvas=ImageOps.mirror(canvas)
                frames.append(canvas)
        if a.creatures:
            for direction,mirror in DIRECTION_MAP:
                canvas=Image.new('RGBA',(192,192))
                for graphic,hue in layers:
                    action=2 if body<200 else 8 if body<400 else 21
                    try:seq=read_layer(a.client,graphic,action,direction)
                    except ValueError:
                        if graphic==body:raise
                        continue
                    image,cx,cy=seq[-1]
                    death_scale=min(1,124/image.width,100/image.height)
                    if death_scale<1:image=image.resize((max(1,round(image.width*death_scale)),max(1,round(image.height*death_scale))),Image.Resampling.NEAREST);cx=round(cx*death_scale);cy=round(cy*death_scale)
                    if hue:image=hue_image(image,hues,hue&0x3FFF)
                    canvas.alpha_composite(image,(96-cx,187-image.height-cy))
                if mirror:canvas=ImageOps.mirror(canvas)
                frames.append(canvas)
        boxes=[im.getbbox() for im in frames];bounds=(min(b[0] for b in boxes),min(b[1] for b in boxes),max(b[2] for b in boxes),max(b[3] for b in boxes))
        folder=a.out/f'style{style:02d}';folder.mkdir()
        scale=min(1,128/(bounds[2]-bounds[0]),144/(bounds[3]-bounds[1]));size=(max(1,round((bounds[2]-bounds[0])*scale)),max(1,round((bounds[3]-bounds[1])*scale)))
        for i,im in enumerate(frames):im.crop(bounds).resize(size,Image.Resampling.NEAREST).save(folder/f'{i:02d}.png')
        (folder/'npc.t3s').write_text('--atlas -f rgba5551 -z none\n'+''.join(f'{i:02d}.png\n' for i in range(len(frames))))
        target=(a.out/f'pop_{style:02d}.t3x').resolve()
        subprocess.run([shutil.which('tex3ds') or '/opt/devkitpro/tools/bin/tex3ds','-i','npc.t3s','-o',str(target)],cwd=folder,check=True)
        # Mirroring 128-wide canvas changes the 64 anchor by one pixel, as in
        # the legacy client mirroring convention; the common footprint is fixed.
        styles.append(dict(name=name,body=body,pivot=[round((96-bounds[0])*scale),round((187-bounds[1])*scale)],size=list(size),scale=scale,frames=len(frames),bytes=target.stat().st_size,checksum=fnv(target.read_bytes()),layers=layers))
    for row in rows:row['style']=names.index(row['profile'])
    (a.out/'requests.json').write_text(json.dumps(rows,indent=2))
    with (a.out/'requests.bin').open('wb') as f:
        for row in rows:f.write(struct.pack('<HHhHHH32s',row['x'],row['y'],row['z'],row['style'],row['roam'],int(row['animal'])|(2 if row.get('aggressive') else 0)|(4 if row.get('stationary') else 0)|(8 if row.get('swims') else 0)|(16 if row.get('waterOnly') else 0),row['type'].upper().encode('ascii')[:31].ljust(32,b'\0'))+struct.pack('<4H',*row['bounds']))
    report=dict(preserved_npc_indices=len(previous) if a.previous else 0,creatures=creatures,aquatic_excluded=[r['type'] for r in aquatic],guarded=guards,count=len(rows),humans=sum(not r['animal'] for r in rows),animals=sum(r['animal'] for r in rows),styles=styles,excluded_objects=excluded,duplicate_spawners=duplicates,source_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),source_revision='ServUO pub57 d76bf4443cf76d081ddaf8f57c87ff33749256af',scene_fnv=fnv(raw))
    (a.out/'manifest.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k!='styles'},indent=2))
    finalize_population(a.out,a.scene,bool(a.creatures),a.require_complete)

def finalize_population(out,scene,extended=True,require_complete=False):
    rows=json.loads((out/'requests.json').read_text());report=json.loads((out/'manifest.json').read_text());styles=report['styles'];raw=scene.read_bytes()
    with (out/'requests.bin').open('wb') as f:
        for row in rows:f.write(struct.pack('<HHhHHH32s',row['x'],row['y'],row['z'],row['style'],row['roam'],int(row['animal'])|(2 if row.get('aggressive') else 0)|(4 if row.get('stationary') else 0)|(8 if row.get('swims') else 0)|(16 if row.get('waterOnly') else 0),row['type'].upper().encode('ascii')[:31].ljust(32,b'\0'))+struct.pack('<4H',*row['bounds']))
    placer=out/'place-population';root=Path(__file__).resolve().parents[1]
    subprocess.run(['c++','-std=c++17','-I'+str(root/'include'),str(root/'tools/place_population.cpp'),str(root/'source/core.cpp'),str(root/'source/scene_io.cpp'),str(root/'source/npcs.cpp'),str(root/'source/doors.cpp'),'-o',str(placer.resolve())],check=True)
    placed=out/'placed.bin';args=[str(placer.resolve()),str(scene.resolve()),str((out/'requests.bin').resolve()),str(placed.resolve())]
    if extended:args.append(str((out/'guarded.json').resolve()))
    subprocess.run(args,check=True)
    if extended:
        indices=[int(i) for i in Path(str(placed)+'.indices').read_text().split()];accepted=set(indices)
        report['unplaced']=report.get('unplaced',[])+[row for i,row in enumerate(rows) if i not in accepted];rows=[rows[i] for i in indices]
    header=struct.unpack_from('<4sHHHHIiiHHhH',raw);inside=[r for r in report.get('unplaced',[]) if header[6]<=r['source_center'][0]<header[6]+header[2] and header[7]<=r['source_center'][1]<header[7]+header[3]]
    if require_complete and inside:raise ValueError('Unplaced interior spawns: '+str(inside))
    report['unplaced_interior']=len(inside)
    placed_raw=placed.read_bytes()
    if extended:placed_raw=b''.join(placed_raw[i*44:(i+1)*44]+struct.pack('<4H',row.get('hits',30 if row['type'].lower()=='dog' else 20 if row['type'].lower()=='cat' else 18 if row['type'].lower()=='rat' else 12),*row.get('damage',[2,4]),60 if row.get('aggressive') else 20) for i,row in enumerate(rows))
    payload=b''.join(struct.pack('<hhHHII',*s['pivot'],*s['size'],s['bytes'],s['checksum']) for s in styles)+placed_raw
    (out/'population.bin').write_bytes(struct.pack('<4sHHII',b'UNP3' if extended and any(r.get('swims') for r in rows) else b'UNP2' if extended else b'UNP1',len(rows),len(styles),fnv(raw),fnv(payload))+payload)
    report.update(count=len(rows),humans=sum(not row['animal'] for row in rows),animals=sum(row['animal'] for row in rows),hostile=sum(bool(row.get('aggressive')) for row in rows))
    (out/'manifest.json').write_text(json.dumps(report,indent=2));(out/'requests.json').write_text(json.dumps(rows,indent=2));print({k:report[k] for k in ('count','humans','animals','hostile')})
if __name__=='__main__':main()
