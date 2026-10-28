#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Private ServUO names/titles and original-client notoriety hue export."""
import argparse,json,re,struct,xml.etree.ElementTree as ET
from pathlib import Path
from export_journal import fnv

def class_files(reference):
    files={}
    for path in sorted(reference.rglob('*.cs'),key=lambda p:('/Mobiles/' not in str(p),str(p))):
        for kind in re.findall(r'\bclass\s+(\w+)',path.read_text(errors='replace')):files.setdefault(kind.lower(),path)
    return files

def describe(kind,reference,files=None):
    if files is None:files=class_files(reference)
    visited=set();scope='';base=kind;protected=None;direct=''
    while base.lower() not in ('mobile','basecreature'):
        if base.lower() in visited or base.lower() not in files:raise ValueError('Missing/cyclic NPC class '+base)
        visited.add(base.lower());raw=files[base.lower()].read_text(errors='replace')
        match=re.search(r'class\s+'+re.escape(base)+r'\s*:\s*(\w+)',raw,re.I)
        if not match:raise ValueError('Missing class declaration '+base)
        code=raw[match.start():];next_class=re.search(r'\n\s*public\s+(?:abstract\s+)?class\s+',code[match.end()-match.start():])
        if next_class:code=code[:match.end()-match.start()+next_class.start()]
        if not direct:direct=code
        if protected is None:
            m=re.search(r'override bool IsInvulnerable\s*\{.*?return (true|false);',code,re.S)
            if m:protected=m[1]=='true'
            elif re.search(r'Blessed\s*=\s*true',code) or re.search(r'override bool CanBeDamaged\(\).*?return false;',code,re.S):protected=True
        base=match[1]
    fixed=re.search(r'(?<![\w.])(?:this\.)?Name\s*=\s*"([^"]+)"',direct)
    titles=re.findall(r'(?<![\w.])(?:this\.)?Title\s*=\s*"([^"]+)"',direct)
    constructor=re.search(r':\s*base\("([^"]+)"(?:\s*,\s*"([^"]+)")?',direct)
    name=fixed[1] if fixed else None;title=titles[-1] if titles else ''
    if constructor:
        if constructor[2]:name,title=constructor[1],constructor[2]
        elif not title:title=constructor[1] if constructor[1].lower().startswith('the ') else 'the '+constructor[1]+' guildmaster'
    return name,title,bool(protected)

def generate(requests,reference,hues,scene,population):
    pools={n.attrib['type']:[x.strip() for x in (n.text or '').split(',') if x.strip()] for n in ET.parse(reference/'names.xml').getroot().findall('namelist')}
    records=[];report=[];files=class_files(reference)
    for i,row in enumerate(requests):
        name,title,protected=describe(row['type'],reference,files)
        female=row['profile'] in ('female','tailor','Sarakki','Verity','Regina')
        if female:title=title.replace('guildmaster','guildmistress')
        if row['animal']:title='';name=name or 'a '+re.sub(r'(?<!^)(?=[A-Z])',' ',row['type']).lower()
        elif not name:
            female=row['profile'] in ('female','tailor','Sarakki','Verity','Regina');pool=pools['female' if female else 'male']
            name=pool[fnv((row['spawner']+row['type']+str(i)).encode())%len(pool)]
        hue=0x3B2 if row['animal'] else 0x35 if protected else 0x59
        entry=hue-1;offset=(entry//8)*708+4+(entry%8)*88+62
        c=struct.unpack_from('<H',hues,offset)[0];rgb=tuple(((c>>shift)&31)*255//31 for shift in (10,5,0))
        clean=lambda s,n:s.encode('ascii','replace')[:n-1].ljust(n,b'\0')
        records.append(struct.pack('<32s48sH3B',clean(name,32),clean(title,48),hue,*rgb));report.append(dict(type=row['type'],name=name,title=title,hue=hue,rgb=rgb,protected=protected))
    body=b''.join(records)
    return struct.pack('<4sHHIII',b'UNL1',len(records),0,fnv(scene),fnv(population),fnv(body))+body,report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--previous',type=Path)
    for key in ('requests','reference','client','scene','population','out'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();requests=json.loads(a.requests.read_text());raw,report=generate(requests,a.reference,(a.client/'hues.mul').read_bytes(),a.scene.read_bytes(),a.population.read_bytes())
    if a.previous:
        old=json.loads((a.previous/'requests.json').read_text());names=(a.previous/'npc_names.bin').read_bytes();oldReport=json.loads((a.previous/'npc_names.json').read_text());body=bytearray(raw[20:])
        assert names[:4]==b'UNL1' and len(names)==20+85*len(old) and struct.unpack_from('<I',names,16)[0]==fnv(names[20:])
        for i,row in enumerate(old):
            assert (row['spawner'],row['type'].lower())==(requests[i]['spawner'],requests[i]['type'].lower())
            if row['animal']==requests[i]['animal']:body[i*85:(i+1)*85]=names[20+i*85:20+(i+1)*85];report[i]=oldReport[i]
        header=bytearray(raw[:20]);struct.pack_into('<I',header,16,fnv(body));raw=header+body
    a.out.write_bytes(raw);a.out.with_suffix('.json').write_text(json.dumps(report,indent=2));print('Exported',len(report),'names/titles; protected',sum(r['protected'] for r in report))
