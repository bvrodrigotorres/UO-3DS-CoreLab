#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export private original blood art used by ServUO Blood.cs (seven variants)."""
import argparse,json,struct,subprocess,shutil
from pathlib import Path
from export_scene import static_image
from export_journal import fnv

def main():
    from ultima_sdk import Files,Art
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--client',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    if a.out.exists():raise FileExistsError(a.out)
    a.out.mkdir(parents=True);Files.set_directory(str(a.client));assert Art.initialize()
    ids=[0x1645,0x122A,0x122B,0x122C,0x122D,0x122E,0x122F]
    for i,art in enumerate(ids):
        im=static_image(Art,art);assert 0<im.width<=128 and 0<im.height<=128;im.save(a.out/f'{i}.png')
    (a.out/'blood.t3s').write_text('--atlas -f rgba5551 -z none\n'+''.join(f'{i}.png\n' for i in range(7)))
    subprocess.run([shutil.which('tex3ds') or '/opt/devkitpro/tools/bin/tex3ds','-i','blood.t3s','-o','blood.t3x'],cwd=a.out,check=True)
    raw=(a.out/'blood.t3x').read_bytes();assert len(raw)<=128*1024
    (a.out/'blood.bin').write_bytes(struct.pack('<4sIII',b'UBL1',7,len(raw),fnv(raw)))
    (a.out/'manifest.json').write_text(json.dumps(dict(art=ids,seconds=3,reference='ServUO pub57 Scripts/Items/Decorative/Blood.cs',bytes=len(raw)),indent=2)+'\n')
if __name__=='__main__':main()
