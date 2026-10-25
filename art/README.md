# Title illustration v0.42

Original background generated using the built-in `image_gen` tool on 2026-10-25.
No official Ultima Online artwork was copied. Project integration downsamples
and adds UI lettering; `hud.cpp` draws the version and loading state at runtime.

- `title-source-v042.png`: generated source painting.
- `title-v042.png`: console-resolution title (400×240).
- `../include/title_art.hpp`: embedded RGB565 data; no image dependency at build time.
- `../tools/prepare_title.py`: preparation script (Pillow and macOS Baskerville).

Final generation prompt:

> Use case: stylized-concept. Asset type: background illustration for a Nintendo 3DS fantasy role-playing game title screen, wide landscape composition 5:3. Create an evocative hand-painted late-1990s Ultima Online-inspired medieval fantasy illustration: ancient Britannian stone castle and harbor at dusk, distant mountains, dark blue sky, warm amber windows, weathered parchment and restrained antique gold ornamental edging. Beautiful traditional fantasy game box art, readable large shapes at only 400x240 pixels, refined painterly detail, atmospheric and inviting. Keep center upper-middle mostly dark open sky to overlay the game title in code, keep bottom center dark for a small loading message. No lettering, no words, no logos, no UI, no watermarks. Full bleed opaque background.
