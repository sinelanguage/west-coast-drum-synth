#!/usr/bin/env python3
"""Generate a 3D-style knob bitmap strip for VSTGUI CAnimKnob (Arturia-inspired)."""

import math
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Install Pillow: pip install Pillow")
    sys.exit(1)

FRAME_SIZE = 56
NUM_FRAMES = 128
OUTPUT = Path(__file__).parent.parent / "resource" / "knob.png"


def draw_knob_frame(draw: ImageDraw.ImageDraw, angle: float, size: int) -> None:
    """Draw a single knob frame at given rotation angle (radians)."""
    cx, cy = size / 2, size / 2
    r = size / 2 - 2

    # Recess shadow
    for i in range(3, 0, -1):
        draw.ellipse(
            [cx - r - i, cy - r + i, cx + r + i, cy + r + i + 2],
            fill=(20, 22, 25, 40),
            outline=None,
        )

    # Body gradient (simulated with concentric circles)
    for i in range(int(r), 0, -1):
        t = 1 - (r - i) / r
        base = 55 + int(45 * t)
        color = (base, base + 2, base + 5, 255)
        draw.ellipse([cx - i, cy - i, cx + i, cy + i], fill=color, outline=None)

    # Highlight arc (top-left)
    hl_angle = angle + math.pi * 0.3
    hl_x = cx + (r - 4) * math.cos(hl_angle)
    hl_y = cy - (r - 4) * math.sin(hl_angle)
    draw.ellipse(
        [hl_x - 4, hl_y - 4, hl_x + 4, hl_y + 4],
        fill=(180, 185, 195, 120),
        outline=None,
    )

    # Center dot / indicator
    ind_angle = angle
    ind_r = r * 0.65
    ind_x = cx + ind_r * math.cos(ind_angle)
    ind_y = cy - ind_r * math.sin(ind_angle)
    draw.ellipse(
        [ind_x - 2, ind_y - 2, ind_x + 2, ind_y + 2],
        fill=(45, 48, 55, 255),
        outline=(80, 85, 95, 255),
    )


def main() -> None:
    total_height = FRAME_SIZE * NUM_FRAMES
    img = Image.new("RGBA", (FRAME_SIZE, total_height), (0, 0, 0, 0))
    draw = ImageDraw.ImageDraw(img)

    # 270 degree range, start from left
    start_angle = math.pi * 0.75
    end_angle = -math.pi * 0.75
    for i in range(NUM_FRAMES):
        t = i / (NUM_FRAMES - 1) if NUM_FRAMES > 1 else 1
        angle = start_angle + t * (end_angle - start_angle)
        y = i * FRAME_SIZE
        sub = img.crop((0, y, FRAME_SIZE, y + FRAME_SIZE))
        sub_draw = ImageDraw.ImageDraw(sub)
        draw_knob_frame(sub_draw, angle, FRAME_SIZE)
        img.paste(sub, (0, y))

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    img.save(OUTPUT)
    print(f"Wrote {OUTPUT} ({FRAME_SIZE}x{total_height}, {NUM_FRAMES} frames)")


if __name__ == "__main__":
    main()
