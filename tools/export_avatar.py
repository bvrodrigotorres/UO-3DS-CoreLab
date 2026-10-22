#!/usr/bin/env python3
"""Export a *genuine* UO human walk/idle atlas from AnimationFrame*.uop.

Produces ONLY local files, not committed client assets. Based on the publicly
documented UOP frame layout in ClassicUO AnimationsLoader.ReadUOPAnimationFrames.
No original artwork or client archives are shipped with this repository.

Current scope: one base human body (no clothes, hair, weapons or equipment
layers). Supports 8 directions by mirroring the 5 original animation directions.

Requires Python 3.10+, Pillow, ultima-sdk-python (UopFile), and tex3ds.
"""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess

DIRECTION_NAMES = ("N", "NE", "E", "SE", "S", "SW", "W", "NW")
# From ClassicUO GetAnimDirection: five stored directions, others mirrored.
DIRECTION_MAP = ((3, True), (2, True), (1, True), (0, False),
                 (1, False), (2, False), (3, False), (4, False))
CANVAS_SIZE = (96, 112)
FRAMES_PER_DIRECTION = 5
ANIMATIONS_PER_SHEET = 40
MAX_PAYLOAD = 48 * 1024 * 1024


def find_uop_files(client):
    """Preserve numeric ordering; support case-sensitive macOS disks."""
    listing = {p.name.casefold(): p for p in client.iterdir() if p.is_file()}
    files = [listing.get(f"animationframe{i}.uop") for i in range(1, 11)]
    return [p for p in files if p is not None]


def find_classic_anim_pair(client):
    """Return (anim.idx, anim.mul) if both classic animation files exist."""
    files = {p.name.casefold(): p for p in client.iterdir() if p.is_file()}
    idx = files.get("anim.idx")
    mul = files.get("anim.mul")
    return (idx, mul) if idx is not None and mul is not None else None


def human_mul_entry_id(body, action, direction):
    """Classic UO human index: 35 actions * 5 directions per body.

    ClassicUO AnimationsLoader.CalculatePeopleGroupOffset uses
    ((body-400)*175+35000)*12 for the offset into anim.idx.
    """
    if not (400 <= body <= 599):
        raise ValueError(f"Not a classic human body: {body}")
    if not (0 <= action < 35 and 0 <= direction < 5):
        raise ValueError(f"Invalid human MUL action/direction: {action}/{direction}")
    return 35000 + (body - 400) * 175 + action * 5 + direction


def decode_mul_animation(idx_path, mul_path, body, action, direction):
    """Read one directional animation from classic anim.idx/anim.mul.

    The entry stores a shared 256xuint16 palette and frame offsets relative to
    the position of the frame-count field immediately after that palette.
    """
    entry_id = human_mul_entry_id(body, action, direction)
    with idx_path.open("rb") as f:
        f.seek(entry_id * 12)
        record = f.read(12)
    if len(record) != 12:
        return None
    offset, length, extra = struct.unpack("<iii", record)
    if offset < 0 or length <= 0 or length > MAX_PAYLOAD:
        return None
    if offset + length > mul_path.stat().st_size:
        raise ValueError(
            f"Classic anim.mul truncated: offset={offset}, length={length}, size={mul_path.stat().st_size}"
        )
    with mul_path.open("rb") as f:
        f.seek(offset)
        data = f.read(length)
    if len(data) != length or len(data) < 520:
        raise ValueError(f"Truncated classic animation entry {entry_id}")
    palette = struct.unpack_from("<256H", data, 0)
    frame_count = struct.unpack_from("<I", data, 512)[0]
    if not 1 <= frame_count <= 200 or 516 + frame_count * 4 > len(data):
        raise ValueError(f"Bad MUL animation frame table: {frame_count}")
    # Unlike UOP, classic offsets are relative to the frame count at 512.
    offsets = struct.unpack_from(f"<{frame_count}I", data, 516)
    return [decode_frame(data, 512 + off, shared_palette=palette)
            for off in offsets]


def read_classic_human_sequences(client, body, walk_action, stand_action):
    pair = find_classic_anim_pair(client)
    if pair is None:
        raise FileNotFoundError(
            "Missing anim.idx + anim.mul (classic base-human animations). "
            "Check the local files; do not redownload the whole client."
        )
    idx, mul = pair
    print(f"Reading classic human frames from {idx.name} / {mul.name}")
    walking = [decode_mul_animation(idx, mul, body, walk_action, d)
               for d in range(5)]
    standing = [decode_mul_animation(idx, mul, body, stand_action, d)
                for d in range(5)]
    if any(not frames for frames in walking + standing):
        missing = []
        for action, label, rows in ((walk_action, "walk", walking),
                                    (stand_action, "stand", standing)):
            missing.extend(f"{label}:{d}" for d, frames in enumerate(rows) if not frames)
        raise ValueError(
            f"Classic anim.mul missing body {body} direction/action entries: "
            + ", ".join(missing)
        )
    print(f"Decoded MUL body {body} with "
          f"{sum(len(x) for x in walking)} walk and "
          f"{sum(len(x) for x in standing)} idle frames")
    return walking, standing


def read_animation_uop(client, body, action, files=None):
    """Find body/action among animation archives; return decompressed payload."""
    from ultima_sdk.uop import UopFile
    files = find_uop_files(client) if files is None else files
    if not files:
        raise FileNotFoundError("Missing AnimationFrame1.uop ... AnimationFrame6.uop in client directory")
    for path in files:
        # UopFile takes one numeric field: insert body in literal pattern.
        # The filename itself is build/animationlegacyframe/000400/00.bin.
        pattern = f"build/animationlegacyframe/{body:06d}/{{0:D2}}.bin"
        container = UopFile(path, pattern)
        # Parse only one file at a time; read_raw uses zlib or zlib+BWT as needed.
        payload = container.read_raw(action)
        if payload:
            if len(payload) > MAX_PAYLOAD:
                raise ValueError(f"Unsafe UOP animation payload size: {len(payload)}")
            print(f"Found body={body} action={action} in {path.name}: {len(payload)} decoded bytes")
            return payload
    raise ValueError(
        f"Body {body}, action {action} not found in {len(files)} UOP archives. "
        "Classic human bodies are often in anim.mul/anim.idx instead; "
        "use --source auto to prefer those files."
    )


def decode_frame(blob, start, *, shared_palette=None, max_size=256):
    """Decode 256-color UOP frame with ClassicUO packed signed-10-bit RLE."""
    from PIL import Image
    data_pos = start + (512 if shared_palette is None else 0)
    if start < 0 or data_pos + 8 > len(blob):
        raise ValueError(f"Frame palette or header truncated at offset {start}")
    palette = (struct.unpack_from("<256H", blob, start)
               if shared_palette is None else shared_palette)
    cx, cy, w, h = struct.unpack_from("<hhhh", blob, data_pos)
    if not (0 < w <= max_size and 0 < h <= max_size):
        raise ValueError(f"Invalid UOP frame dimensions {w}x{h}")
    pixels = bytearray(w * h * 4)
    pos = data_pos + 8
    terminated = False
    for _ in range(w * h + 4096):
        if pos + 4 > len(blob):
            raise ValueError("Truncated UOP RLE command stream")
        (command,) = struct.unpack_from("<I", blob, pos)
        pos += 4
        if command == 0x7FFF7FFF:
            terminated = True
            break
        count = command & 0xFFF
        raw_x = (command >> 22) & 0x3FF
        raw_y = (command >> 12) & 0x3FF
        x = (raw_x - 1024 if raw_x & 0x200 else raw_x) + cx
        y = (raw_y - 1024 if raw_y & 0x200 else raw_y) + cy + h
        if count == 0 or pos + count > len(blob):
            raise ValueError(f"Invalid UOP RLE run length {count}")
        if y < 0 or y >= h or x < 0 or x + count > w:
            raise ValueError(f"RLE pixels outside {w}x{h} at x={x} y={y} run={count}")
        for i in range(count):
            value = palette[blob[pos + i]]
            if value:
                off = ((y * w) + x + i) * 4
                pixels[off] = ((value >> 10) & 31) * 255 // 31
                pixels[off + 1] = ((value >> 5) & 31) * 255 // 31
                pixels[off + 2] = (value & 31) * 255 // 31
                pixels[off + 3] = 255
        pos += count
    if not terminated:
        raise ValueError("UOP RLE sentinel not found")
    return Image.frombytes("RGBA", (w, h), bytes(pixels))


def decode_animation(blob, stored_direction):
    """Extract one of five directional frame sequences from a UOP action."""
    if len(blob) < 40:
        raise ValueError("UOP animation action is too short")
    (count, offset) = struct.unpack_from("<II", blob, 32)
    if not 5 <= count <= 5000 or offset >= len(blob) or offset + count * 16 > len(blob):
        raise ValueError(f"Invalid frame table count={count}, offset={offset}, size={len(blob)}")
    # UOP stores frame IDs 1-based; missing IDs can be represented as gaps.
    entries = []
    for i in range(count):
        entry_pos = offset + i * 16
        group, frame_id = struct.unpack_from("<HH", blob, entry_pos)
        (pixel_offset,) = struct.unpack_from("<I", blob, entry_pos + 12)
        if frame_id > 0:
            entries.append((frame_id, group, entry_pos + pixel_offset))
    if not entries:
        raise ValueError("No usable frame table entries")
    estimated_frames_per_direction = max(1, round(max(x[0] for x in entries) / 5.0))
    result = []
    for frame_id, group, pix_offset in entries:
        direction = (frame_id - 1) // estimated_frames_per_direction
        if direction == stored_direction:
            try:
                frame = decode_frame(blob, pix_offset)
            except ValueError as exc:
                raise ValueError(f"Malformed frame id={frame_id} group={group}: {exc}") from exc
            result.append(((frame_id - 1) % estimated_frames_per_direction, frame))
    result.sort(key=lambda x: x[0])
    if not result:
        raise ValueError(f"No frames for stored direction {stored_direction}")
    return [f for _, f in result]


def select_frames(sequence, count=4):
    """Deterministic walk cycle sampling, independent of source frame count."""
    if not sequence:
        raise ValueError("No animation frames")
    return [sequence[(i * len(sequence)) // count] for i in range(count)]


def place_on_canvas(frame, mirror=False, canvas_size=CANVAS_SIZE):
    """Foot-aligned consistent atlas cell, with transparent padding."""
    from PIL import Image, ImageOps
    image = ImageOps.mirror(frame) if mirror else frame
    box_w, box_h = canvas_size
    if image.width > box_w or image.height > box_h:
        raise ValueError(
            f"Frame {image.width}x{image.height} exceeds {box_w}x{box_h} canvas. "
            "Try a different body or increase CANVAS_SIZE."
        )
    canvas = Image.new("RGBA", canvas_size, (0, 0, 0, 0))
    # Same canvas and anchor for all 40 frames (feet at canvas bottom).
    canvas.alpha_composite(image, ((box_w - image.width) // 2, box_h - image.height - 5))
    return canvas


def build_frames_from_sequences(walking, standing, running=None):
    """40 standard sprites, optionally 32 authentic running sprites appended."""
    if len(walking) != 5 or len(standing) != 5:
        raise ValueError("Expected exactly five directional sequences")
    frames = []
    for d, mirrored in DIRECTION_MAP:
        idle = standing[d][0]
        steps = select_frames(walking[d])
        frames.append(place_on_canvas(idle, mirrored))
        frames.extend(place_on_canvas(step, mirrored) for step in steps)
    assert len(frames) == ANIMATIONS_PER_SHEET
    if running is not None:
        if len(running) != 5 or any(not seq for seq in running):
            raise ValueError("Expected five non-empty classic run directions")
        for d, mirrored in DIRECTION_MAP:
            frames.extend(place_on_canvas(step, mirrored)
                          for step in select_frames(running[d]))
    assert len(frames) in (40, 72)
    return frames


def build_frames(walk_blob, stand_blob):
    """Backward-compatible UOP payload adapter."""
    return build_frames_from_sequences(
        [decode_animation(walk_blob, d) for d in range(5)],
        [decode_animation(stand_blob, d) for d in range(5)]
    )


def write_atlas(frames, out_dir, tex3ds):
    from PIL import Image, ImageDraw
    out_dir.mkdir(parents=True, exist_ok=True)
    filenames = []
    for i, img in enumerate(frames):
        name = f"{i:03d}.png"
        img.save(out_dir / name)
        filenames.append(name)
    manifest = out_dir / "player_anim.t3s"
    manifest.write_text("--atlas -f rgba5551 -z none\n" +
                        "".join(f"{name}\n" for name in filenames), encoding="utf-8")
    subprocess.run([tex3ds, "-i", manifest.name, "-o", "player_anim.t3x"],
                   cwd=out_dir, check=True)
    if not (out_dir / "player_anim.t3x").is_file():
        raise RuntimeError("tex3ds did not write player_anim.t3x")
    # A local-only labelled preview for checking directions and walk frames.
    preview = Image.new("RGB", (5 * CANVAS_SIZE[0],
                   (8 if len(frames)==40 else 16) * (CANVAS_SIZE[1] + 16)),
                   (51, 54, 65))
    draw = ImageDraw.Draw(preview)
    for i, img in enumerate(frames):
        if i < 40:
            row, col = divmod(i, 5)
            label = f"{DIRECTION_NAMES[row]} {'idle' if col == 0 else 'walk'+str(col)}"
        else:
            direction, frame = divmod(i-40,4)
            row, col = direction+8, frame
            label = f"{DIRECTION_NAMES[direction]} run{frame+1}"
        x, y = col * CANVAS_SIZE[0], row * (CANVAS_SIZE[1] + 16)
        preview.paste(img, (x, y), img)
        draw.text((x + 3, y + CANVAS_SIZE[1] + 1),
                  label, fill=(255, 255, 255))
    preview.save(out_dir / "preview_avatar.png")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client", type=Path, default=Path.home() / "UO_Client")
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--body", type=int, choices=(400, 401), default=400,
                   help="Classic UO human base model: 400 male, 401 female")
    p.add_argument("--walk-action", type=int, default=0,
                   help="ClassicUO human WalkUnarmed=0")
    p.add_argument("--stand-action", type=int, default=4,
                   help="ClassicUO human Stand=4")
    p.add_argument("--include-run", action="store_true",
                   help="Append 32 authentic UO run frames (72-frame atlas)")
    p.add_argument("--run-action", type=int, default=2,
                   help="Classic human RunUnarmed=2; RunArmed=3; WalkArmed=1")
    p.add_argument("--source", choices=("auto", "mul", "uop"), default="auto",
                   help="Auto prefers classic anim.mul+anim.idx for bodies 400/401")
    args = p.parse_args()
    client = args.client.expanduser().resolve()
    out_dir = args.out.expanduser().resolve()
    if out_dir.exists():
        p.error("Output must be a new directory")
    if not client.is_dir():
        p.error(f"Client directory missing: {client}")
    files = find_uop_files(client)
    print("Animation archives:", ", ".join(x.name for x in files) or "NONE")
    pair = find_classic_anim_pair(client)
    print("Classic anim.idx + anim.mul:", "BOTH FOUND" if pair else "MISSING")
    tex = shutil.which("tex3ds") or "/opt/devkitpro/tools/bin/tex3ds"
    if not Path(tex).is_file():
        p.error("tex3ds missing. Install devkitPro 3ds-dev.")
    source = args.source
    if source == "auto":
        source = "mul" if pair else "uop"
    print(f"Selected human animation source: {source}")
    if source == "mul":
        walking, standing = read_classic_human_sequences(
            client, args.body, args.walk_action, args.stand_action
        )
        running = None
        if args.include_run:
            pair = find_classic_anim_pair(client)
            running = [decode_mul_animation(pair[0],pair[1],args.body,
                                           args.run_action,d) for d in range(5)]
        frames = build_frames_from_sequences(walking, standing, running)
    else:
        if not files:
            p.error(
                "No AnimationFrame*.uop archives found and classic anim.mul/anim.idx "
                "are missing. Check the client directory before downloading anything."
            )
        walk = read_animation_uop(client, args.body, args.walk_action, files)
        stand = read_animation_uop(client, args.body, args.stand_action, files)
        frames = build_frames(walk, stand)
        if args.include_run:
            run_blob = read_animation_uop(client, args.body, args.run_action, files)
            sequences = [decode_animation(run_blob,d) for d in range(5)]
            frames = build_frames_from_sequences(
                [decode_animation(walk,d) for d in range(5)],
                [decode_animation(stand,d) for d in range(5)],
                sequences)
    write_atlas(frames, out_dir, tex)
    payload = (out_dir/'player_anim.t3x').read_bytes()
    checksum = 2166136261
    for byte in payload:
        checksum = ((checksum ^ byte) * 16777619) & 0xffffffff
    manifest = dict(body=args.body, source=source, walk_action=args.walk_action,
                    stand_action=args.stand_action, run_action=args.run_action if args.include_run else None,
                    frames=len(frames), atlas_fnv1a=f'{checksum:08x}')
    (out_dir/'avatar_manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    (out_dir/'player_anim.meta').write_text(f'UOA1 {args.body} {args.walk_action} {args.stand_action} {args.run_action if args.include_run else 0} {len(frames)} {checksum:08x}\n')
    print(f"Avatar sprite frames: {len(frames)}; "
          f"{'authentic run enabled' if len(frames)==72 else 'running uses faster walk frames'}")
    print("Generated genuine UO body art at", out_dir)
    print("Preview:", out_dir / "preview_avatar.png")
    print("SD copy: player_anim.t3x AND player_anim.meta -> SD:/3ds/UO-3DS-CoreLab/")
    print("No original client assets were uploaded to GitHub.")


if __name__ == "__main__":
    main()
