from PIL import Image, ImageDraw
from pathlib import Path

SIZE = 240

# Create a simple black-and-white CD placeholder.
img = Image.new("L", (SIZE, SIZE), 0)
d = ImageDraw.Draw(img)
cx = cy = SIZE // 2

# Outer disc
d.ellipse((4, 4, SIZE-5, SIZE-5), fill=255)

# Inner ring / hub
d.ellipse((cx-30, cy-30, cx+30, cy+30), fill=0)
d.ellipse((cx-13, cy-13, cx+13, cy+13), fill=255)
d.ellipse((cx-5, cy-5, cx+5, cy+5), fill=0)

# A few simple radial grooves/highlights
for r in (42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 102):
    d.ellipse((cx-r, cy-r, cx+r, cy+r), outline=0, width=1)

# Convert to RGB JPEG.
img_rgb = img.convert("RGB")
jpeg_path = Path("./cd_fallback.jpg")
img_rgb.save(jpeg_path, format="JPEG", quality=85, optimize=True)

# Convert JPEG bytes to a PROGMEM C array.
data = jpeg_path.read_bytes()
lines = []
for i in range(0, len(data), 16):
    chunk = data[i:i+16]
    lines.append("    " + ",".join(f"0x{b:02x}" for b in chunk) + ",")

header = """#pragma once
#include <Arduino.h>

// 160x160 black-and-white CD fallback image.
// JPEG data is stored in flash (PROGMEM).
const uint8_t cdImage[] PROGMEM = {
""" + "\n".join(lines) + f"""
}};

const size_t cdImageSize = sizeof(cdImage);
"""

h_path = Path("./cd_image.h")
h_path.write_text(header)

print(f"Created {h_path} ({len(data):,} JPEG bytes)")
print(f"Created {jpeg_path} ({len(data):,} bytes)")

