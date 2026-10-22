#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export an unfiltered private UO scene. Never commit the output."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

HEAD = struct.Struct('<4sHHHHIiiHHhH')
LAND = struct.Struct('<HHQB')
OBJECT = struct.Struct('<HHbBHHHQIH')
NONE = 65535


def find(client, name, required=True):
    p = next((p for p in client.iterdir() if p.is_file() and p.name.casefold() == name.casefold()), None)
    if p is None and required:
        raise FileNotFoundError(name)
    return p


def tiledata(path):
    data = path.read_bytes()
    for n in (8, 4):
        start = 512 * (4 + 32 * (n + 22))
        stride = 4 + 32 * (n + 33)
        if len(data) < start or (len(data)-start) % stride:
            continue
        lands, objects = [], []
        pos = 0
        for _ in range(512):
            pos += 4
            for _ in range(32):
                lands.append((int.from_bytes(data[pos:pos+n], 'little'), struct.unpack_from('<H', data, pos+n)[0]))
                pos += n+22
        for _ in range((len(data)-start)//stride):
            pos += 4
            for _ in range(32):
                objects.append((int.from_bytes(data[pos:pos+n], 'little'), data[pos+n+12]))
                pos += n+33
        return lands, objects
    raise ValueError('Unsupported tiledata layout')


def differences(client, prefix, index_name=None):
    listing = find(client, prefix+'l0.mul', False)
    data = find(client, prefix+'0.mul', False)
    if listing is None or listing.stat().st_size == 0:
        return {}
    if data is None:
        raise ValueError('Incomplete patch set: '+prefix)
    raw = listing.read_bytes()
    if len(raw) % 4:
        raise ValueError('Invalid patch list: '+prefix)
    ids = [v[0] for v in struct.iter_unpack('<I', raw)]
    # Like ClassicUO MapLoader.ApplyPatches: later entries overwrite earlier
    # entries for the same block. Some real client patch lists contain repeats.
    if index_name:
        idx = find(client, index_name).read_bytes()
        if len(idx) != len(ids)*12:
            raise ValueError('Invalid static patch index')
        blob = data.read_bytes()
        out = {}
        for i, block in enumerate(ids):
            offset, length, _ = struct.unpack_from('<III', idx, i*12)
            if offset == 0xffffffff or length in (0, 0xffffffff):
                out[block] = b''
            elif offset+length <= len(blob) and length % 7 == 0:
                out[block] = blob[offset:offset+length]
            else:
                raise ValueError('Invalid patched static block')
        return out
    blob = data.read_bytes()
    if len(blob) != len(ids)*196:
        raise ValueError('Invalid map patch data')
    return {block: blob[i*196:(i+1)*196] for i, block in enumerate(ids)}


class MapReader:
    def __init__(self, client):
        self.file = None
        path = find(client, 'map0.mul', False)
        if path:
            self.file = path.open('rb')
        else:
            from ultima_sdk.uop import UopFile
            self.uop = UopFile(find(client, 'map0LegacyMUL.uop'), 'build/map0legacymul/{0:D8}.dat')
            self.uop.parse()
        self.cache = {}
        self.patches = differences(client, 'mapdif')

    def get(self, x, y):
        if not 0 <= x < 7168 or not 0 <= y < 4096:
            raise ValueError('Map coordinate outside supported Felucca bounds')
        block = (x//8)*512+y//8
        offset = 4+((y % 8)*8+x % 8)*3
        if block in self.patches:
            raw = self.patches[block][offset:offset+3]
        elif self.file:
            self.file.seek(block*196+offset)
            raw = self.file.read(3)
        else:
            segment = block//4096
            if segment not in self.cache:
                self.cache[segment] = self.uop.read_raw(segment)
            blob = self.cache[segment]
            if not blob:
                raise ValueError('Missing map segment')
            offset += (block % 4096)*196
            raw = blob[offset:offset+3]
        if len(raw) != 3:
            raise ValueError('Truncated map cell')
        return struct.unpack('<Hb', raw)

    def close(self):
        if self.file:
            self.file.close()


def statics(client, ox, oy, side):
    patches = differences(client, 'stadif', 'stadifi0.mul')
    result = []
    with find(client, 'staidx0.mul').open('rb') as index, find(client, 'statics0.mul').open('rb') as data:
        length = find(client, 'statics0.mul').stat().st_size
        for bx in range(ox//8, (ox+side-1)//8+1):
            for by in range(oy//8, (oy+side-1)//8+1):
                block = bx*512+by
                if block in patches:
                    raw = patches[block]
                else:
                    index.seek(block*12)
                    entry = index.read(12)
                    if len(entry) != 12:
                        raise ValueError('Truncated staidx')
                    offset, size, _ = struct.unpack('<III', entry)
                    if offset == 0xffffffff or size in (0, 0xffffffff):
                        continue
                    if size % 7 or offset+size > length:
                        raise ValueError('Invalid static block')
                    data.seek(offset)
                    raw = data.read(size)
                for art, x, y, z, hue in struct.iter_unpack('<HBBbH', raw):
                    if x > 7 or y > 7:
                        raise ValueError('Invalid coordinate in static block')
                    x, y = bx*8+x-ox, by*8+y-oy
                    if 0 <= x < side and 0 <= y < side:
                        result.append((x, y, z, art, hue))
    return result


def pixels555(raw, width, height):
    from PIL import Image
    if len(raw) != width*height*2:
        raise ValueError('Invalid 555 pixel payload')
    rgba = bytearray()
    for (v,) in struct.iter_unpack('<H', raw):
        rgba.extend((((v >> 10) & 31)*255//31, ((v >> 5) & 31)*255//31, (v & 31)*255//31, 255 if v else 0))
    return Image.frombytes('RGBA', (width, height), bytes(rgba))


def land_image(art, graphic):
    from PIL import Image
    raw = art._index.read_raw(graphic)
    if not raw or len(raw) != 2024:
        raise ValueError(f'Missing original land image 0x{graphic:04x}')
    values = struct.unpack('<1012H', raw)
    image = Image.new('RGBA', (44, 44))
    p = 0
    for y, size in enumerate(list(range(2, 46, 2))+list(range(44, 0, -2))):
        for x in range(size):
            v = values[p];p += 1
            image.putpixel(((44-size)//2+x, y), (((v >> 10) & 31)*255//31, ((v >> 5) & 31)*255//31, (v & 31)*255//31, 255))
    return image


def static_image(art, graphic):
    image = art.get_static(graphic) if hasattr(art, 'get_static') else None
    if image is not None and hasattr(image, 'to_image'):
        return image.to_image().convert('RGBA')
    # SDK versions differ in whether the generic accessor expects +0x4000.
    image = art.get_art(graphic+0x4000)
    if image is not None and hasattr(image, 'to_image'):
        return image.to_image().convert('RGBA')
    # Modern local UOP payload supported by the existing client installation.
    raw = art._index.read_raw(graphic)
    if raw and len(raw) >= 4:
        width, height = struct.unpack_from('<HH', raw)
        if 0 < width <= 1024 and 0 < height <= 1024 and len(raw) == 4+width*height*2:
            return pixels555(raw[4:], width, height)
    raise ValueError(f'Missing original static image 0x{graphic:04x}')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--decorations', type=Path, help='Private audited ServUO region manifest')
    p.add_argument('--client', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--x', type=int, default=1488)
    p.add_argument('--y', type=int, default=1596)
    p.add_argument('--side', type=int, default=32)
    p.add_argument('--spawn-x', type=int, default=1501)
    p.add_argument('--spawn-y', type=int, default=1612)
    p.add_argument('--spawn-z', type=int, default=20)
    args = p.parse_args()
    if not 1 <= args.side <= 512:
        p.error('--side must be 1..512')
    if not args.x <= args.spawn_x < args.x+args.side or not args.y <= args.spawn_y < args.y+args.side or not -127 <= args.spawn_z <= 127:
        p.error('Spawn outside scene or invalid height')
    out = args.out.resolve()
    if out.exists():
        p.error('Output directory must not exist')
    client = args.client.expanduser().resolve()
    lands, data = tiledata(find(client, 'tiledata.mul'))
    reader = MapReader(client)
    grid = []
    try:
        for y in range(args.side+1):
            grid.append([reader.get(args.x+x, args.y+y) for x in range(args.side+1)])
    finally:
        reader.close()
    rows = statics(client, args.x, args.y, args.side)
    base_count=len(rows);decoration=None
    if args.decorations:
        decoration=json.loads(args.decorations.read_text())
        if decoration['origin']!=[args.x,args.y] or decoration['side']!=args.side or decoration['base_objects']!=base_count:
            raise ValueError('Decoration manifest belongs to another region')
        for obj in decoration['placements']+[d for d in decoration['doors'] if 'base_index' not in d]:
            rows.append(tuple(obj[k] for k in ('x','y','z','art','hue')))
    if len(rows) > 131072:
        raise ValueError('Scene exceeds 131072 objects; choose a smaller region')
    from ultima_sdk import Files, Art
    Files.set_directory(str(client))
    if not Art.initialize():
        raise ValueError('Cannot initialize Ultima SDK Art')
    palette, images, identical = {}, [], {};missing_lands=set()
    texture_files = (find(client, 'texidx.mul', False), find(client, 'texmaps.mul', False))
    def picture(kind, graphic):
        key = (kind, graphic)
        if key in palette:
            return palette[key]
        if len(images) >= 8192:
            raise ValueError('More than 8192 source graphics; choose a smaller region')
        if kind == 'static':
            image = static_image(Art, graphic)
        elif kind == 'land':
            try:image = land_image(Art, graphic)
            except ValueError:
                from PIL import Image
                image=Image.new('RGBA',(44,44),(0,0,0,255));missing_lands.add(graphic)
        else:
            idx, tex = texture_files
            if not idx or not tex:
                raise ValueError('Sloped terrain requires texidx.mul and texmaps.mul')
            with idx.open('rb') as index, tex.open('rb') as pixels:
                index.seek(graphic*12)
                raw = index.read(12)
                if len(raw) != 12:
                    raise ValueError('Missing texmap index')
                offset, length, _ = struct.unpack('<III', raw)
                side = {8192: 64, 32768: 128}.get(length)
                if offset == 0xffffffff or side is None:
                    raise ValueError(f'Missing terrain texture {graphic}')
                pixels.seek(offset)
                image = pixels555(pixels.read(length), side, side)
        if image is None or image.getbbox() is None or max(image.size) > 1024:
            raise ValueError(f'Invalid image {key}')
        digest=(image.size,hashlib.sha256(image.tobytes()).digest())
        if digest in identical:palette[key]=identical[digest];return palette[key]
        palette[key] = len(images);identical[digest]=len(images)
        images.append(image)
        return palette[key]
    cells = [];terrain_fallbacks=set()
    for y in range(args.side):
        for x in range(args.side):
            art, z = grid[y][x]
            flags, texid = lands[art]
            corners = (z, grid[y][x+1][1], grid[y+1][x+1][1], grid[y+1][x][1])
            slope = len(set(corners)) > 1
            mode = int(slope and texid > 0)
            try:image = NONE if art == 2 else picture('texture' if mode else 'land', texid if mode else art)
            except ValueError:
                if not mode:raise
                terrain_fallbacks.add((art,texid));mode=0;image=picture('land',art)
            cells.append(LAND.pack(art, image, flags, mode))
    objects = []
    for order, (x, y, z, art, hue) in enumerate(rows):
        if art >= len(data):
            raise ValueError(f'Tiledata missing static {art}')
        flags, height = data[art]
        objects.append(OBJECT.pack(x, y, z, height, art, hue, picture('static', art), flags, order, 0))
    door_records=[]
    if decoration:
        append_index=base_count+len(decoration['placements'])
        for n,door in enumerate(decoration['doors']):
            if 'base_index' in door:index=door['base_index']
            else:index=append_index;append_index+=1
            opened=picture('static',door['open_art'])
            pair=door['pair'] if door['pair'] is not None else 65535
            door_records.append(struct.pack('<IHHbbH',index,door['open_art'],opened,door['dx'],door['dy'],pair))
    # Group similarly sized images before packing. Remap only texture indices;
    # tile art, placement, flags and draw order remain identical.
    permutation=sorted(range(len(images)),key=lambda i:(max(images[i].size),images[i].width*images[i].height,images[i].width))
    # Eight real sprites per GPU page reduce the unrelated pixels needed by
    # a mixed terrain/building view. Logical slots remain 32 for compatibility.
    from PIL import Image
    packed=[];remap={}
    for group in range(0,len(permutation),8):
        if packed:
            while len(packed)%32:packed.append(Image.new('RGBA',(1,1)))
        for old in permutation[group:group+8]:remap[old]=len(packed);packed.append(images[old])
    images=packed
    if len(images)>32768:raise ValueError('Paged image limit exceeded')
    next_cells=[]
    for cell in cells:
        values=list(LAND.unpack(cell));values[1]=remap.get(values[1],NONE);next_cells.append(LAND.pack(*values))
    cells=next_cells
    next_objects=[]
    for obj in objects:
        values=list(OBJECT.unpack(obj));values[6]=remap[values[6]];next_objects.append(OBJECT.pack(*values))
    objects=next_objects
    next_doors=[]
    for record in door_records:
        values=list(struct.unpack('<IHHbbH',record));values[2]=remap[values[2]];next_doors.append(struct.pack('<IHHbbH',*values))
    door_records=next_doors
    # Base data is preserved; audited server additions follow it.

    vertices = bytes(z & 255 for row in grid for art, z in row)
    raw = HEAD.pack(b'UCL1', 1, args.side, args.side, len(images), len(rows), args.x, args.y, args.spawn_x-args.x, args.spawn_y-args.y, args.spawn_z, 0)+vertices+b''.join(cells)+b''.join(objects)
    tex3ds = shutil.which('tex3ds') or '/opt/devkitpro/tools/bin/tex3ds'
    if not Path(tex3ds).exists():
        raise FileNotFoundError('Install tex3ds first')
    out.mkdir(parents=True)
    sources = out/'_private_sources'
    sources.mkdir()
    for page in range((len(images)+31)//32):
        folder = sources/f'{page:02d}'
        folder.mkdir()
        group = images[page*32:page*32+32]
        for i, image in enumerate(group):
            image.save(folder/f'{i:02d}.png')
        (folder/'page.t3s').write_text('--atlas -f rgba5551 -z none\n'+''.join(f'{i:02d}.png\n' for i in range(len(group))))
        subprocess.run([tex3ds, '-i', 'page.t3s', '-o', str(out/f'atlas_{page:02d}.t3x')], cwd=folder, check=True)
    (out/'image_sizes.bin').write_bytes(struct.pack('<4sHH',b'UOI1',len(images),(len(images)+31)//32)+b''.join(struct.pack('<HH',*im.size) for im in images)+b''.join(struct.pack('<I',(out/f'atlas_{page:02d}.t3x').stat().st_size) for page in range((len(images)+31)//32)))
    (out/'scene.bin').write_bytes(raw)
    if decoration:
        scene_hash=2166136261
        for byte in raw:scene_hash=((scene_hash^byte)*16777619)&0xffffffff
        (out/'doors.bin').write_bytes(struct.pack('<4sHHII',b'UOD1',1,len(door_records),base_count,scene_hash)+b''.join(door_records))
        (out/'decoration_manifest.json').write_text(json.dumps(decoration,indent=2)+'\n')
    report = dict(missing_land_graphics=sorted(missing_lands),terrain_fallbacks=sorted(terrain_fallbacks),origin=[args.x, args.y], side=args.side, spawn=[args.spawn_x, args.spawn_y, args.spawn_z], objects=len(rows), images=len(images), roofs=sum(bool(data[r[3]][0] & 0x10000000) for r in rows), hued_objects=sum(r[4] != 0 for r in rows), sha256=hashlib.sha256(raw).hexdigest(), limitations=['Hues retained but not rendered in v0.1', 'Dynamic objects and multis not included', 'Experimental height ceiling, not full CrossUO roof connectivity'], source_files={name:hashlib.sha256(find(client,name).read_bytes()).hexdigest() for name in ['tiledata.mul','staidx0.mul']})
    if decoration:
        report['base_objects']=base_count
        report['server_decoration']=len(decoration['placements'])
        report['interactive_doors']=len(door_records)
        report['limitations']=['Only the audited ServUO region additions; no NPC population, container contents or running server', 'Hues retained but not rendered', 'Experimental height ceiling, not full roof connectivity']
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    (out/'placements.json').write_text(json.dumps([dict(x=x+args.x,y=y+args.y,z=z,art=art,hue=hue,order=i) for i,(x,y,z,art,hue) in enumerate(rows)], indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
