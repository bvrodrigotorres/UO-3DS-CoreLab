#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export original inventory icons privately; never commit the generated file."""
import argparse
from pathlib import Path
import struct
from export_scene import static_image

ARTS=(0xEED,0x9F1,0x9D0,0xE21,0xF0E,0xF51)

def main():
    from PIL import Image
    from ultima_sdk import Files,Art
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    args=p.parse_args()
    if args.out.exists():raise FileExistsError(args.out)
    Files.set_directory(str(args.client));Art.initialize()
    data=bytearray(b'UII1'+struct.pack('<I',len(ARTS)))
    for graphic in ARTS:
        image=static_image(Art,graphic)
        if image is None or image.getbbox() is None:raise ValueError(f'Missing art {graphic:04X}')
        image=image.crop(image.getbbox());image.thumbnail((24,22),Image.Resampling.LANCZOS)
        canvas=Image.new('RGBA',(24,24));canvas.alpha_composite(image,((24-image.width)//2,(24-image.height)//2))
        data.extend(canvas.tobytes())
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_bytes(data)
    print(f'{len(ARTS)} original icons; {len(data)} bytes; private output {args.out}')

if __name__=='__main__':main()
