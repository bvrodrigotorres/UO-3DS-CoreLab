#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Fail a 3DS build when app stack frames exceed its audited limits."""
from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else 'build')
rows=[]
for path in root.glob('*.su'):
    for line in path.read_text().splitlines():
        fields=line.split('\t')
        if len(fields)>=3:
            rows.append((fields[0],int(fields[-2]),fields[-1]))
if not rows:raise SystemExit('No compiler stack usage reports found')
main=[r for r in rows if r[0].endswith(('int main()', 'int main(int, char**)'))]
if len(main)!=1:raise SystemExit('Missing or ambiguous main stack report')
for name,size,kind in rows:
    if size>16384:raise SystemExit(f'Excessive stack frame: {size} {name}')
    if any(n in name for n in ('drawEquipmentMenu(','drawAppearance(','equipmentEditor(')) and size>2048:raise SystemExit('Equipment UI stack regression')
    if 'OverheadFont::load(' in name and size>1024:raise SystemExit('Font loading stack regression')
    if kind=='dynamic':raise SystemExit(f'Unbounded dynamic stack use: {name}')
print(f'3DS stack audit: main {main[0][1]} bytes; largest frame {max(r[1] for r in rows)} bytes; all application frames <=16 KiB')
