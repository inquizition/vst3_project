#!/usr/bin/env python3
from PIL import Image
import math
import argparse
from pathlib import Path

def make_strip(
    input_path: Path,
    output_path: Path,
    frames: int = 128,
    start_deg: float = -135.0,
    end_deg: float = 135.0,
    direction: str = "cw",
    layout: str = "vertical",
    resample: str = "bicubic",
    trim: bool = True,
    pad: int = 0,
):
    img0 = Image.open(input_path).convert("RGBA")

    # Optionally trim to content (useful if you exported with lots of transparent margin)
    if trim:
        bbox = img0.getbbox()
        if bbox:
            img0 = img0.crop(bbox)

    w, h = img0.size
    w += 2 * pad
    h += 2 * pad
    if pad:
        base = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        base.paste(img0, (pad, pad))
        img0 = base

    res_map = {
        "nearest": Image.Resampling.NEAREST,
        "bilinear": Image.Resampling.BILINEAR,
        "bicubic": Image.Resampling.BICUBIC,
        "lanczos": Image.Resampling.LANCZOS,
    }
    rs = res_map.get(resample.lower(), Image.Resampling.BICUBIC)

    total = end_deg - start_deg
    if direction.lower() == "ccw":
        total = -total

    # Create output canvas
    if layout.lower() == "horizontal":
        strip = Image.new("RGBA", (w * frames, h), (0, 0, 0, 0))
    else:
        strip = Image.new("RGBA", (w, h * frames), (0, 0, 0, 0))

    for i in range(frames):
        t = 0 if frames == 1 else i / (frames - 1)
        angle = start_deg + total * t

        # rotate about center; expand=False keeps identical frame size
        frame = img0.rotate(angle, resample=rs, expand=False)

        if layout.lower() == "horizontal":
            strip.paste(frame, (i * w, 0), frame)
        else:
            strip.paste(frame, (0, i * h), frame)

    strip.save(output_path)
    print(f"Saved: {output_path}  ({frames} frames, {layout})")

if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="Generate a knob filmstrip by rotating one knob image.")
    ap.add_argument("input", type=Path, help="Input knob image (PNG recommended)")
    ap.add_argument("output", type=Path, help="Output strip image (PNG)")
    ap.add_argument("--frames", type=int, default=128)
    ap.add_argument("--start", type=float, default=-135.0, help="Start angle in degrees")
    ap.add_argument("--end", type=float, default=135.0, help="End angle in degrees")
    ap.add_argument("--direction", choices=["cw", "ccw"], default="cw")
    ap.add_argument("--layout", choices=["vertical", "horizontal"], default="vertical")
    ap.add_argument("--resample", choices=["nearest", "bilinear", "bicubic", "lanczos"], default="bicubic")
    ap.add_argument("--no-trim", dest="trim", action="store_false")
    ap.add_argument("--pad", type=int, default=0, help="Transparent padding around knob before rotation")
    args = ap.parse_args()

    make_strip(
        args.input, args.output,
        frames=args.frames,
        start_deg=args.start,
        end_deg=args.end,
        direction=args.direction,
        layout=args.layout,
        resample=args.resample,
        trim=args.trim,
        pad=args.pad,
    )
