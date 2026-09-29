
#!/usr/bin/env python3
from PIL import Image, ImageSequence
from pathlib import Path
import argparse, struct

MAGIC = b"AWP1"
W, H = 400, 240

def fit(im):
    im = im.convert("RGBA")
    scale = min(W / im.width, H / im.height)
    nw, nh = max(1, round(im.width * scale)), max(1, round(im.height * scale))
    im = im.resize((nw, nh), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (W, H), (0,0,0,255))
    canvas.alpha_composite(im, ((W-nw)//2, (H-nh)//2))
    return canvas.convert("RGB")

def rgb_to_bgr_bytes(im):
    raw = bytearray()
    for r,g,b in im.getdata():
        raw.extend((b,g,r))
    return bytes(raw)

ap = argparse.ArgumentParser()
ap.add_argument("gif")
ap.add_argument("output", nargs="?", default="wallpaper.awp")
ap.add_argument("--fps", type=float, default=None,
                help="Override GIF timing with fixed FPS")
args = ap.parse_args()

src = Image.open(args.gif)
frames = []
durations = []
for fr in ImageSequence.Iterator(src):
    frames.append(rgb_to_bgr_bytes(fit(fr)))
    durations.append(fr.info.get("duration", src.info.get("duration", 100)))

if not frames:
    raise SystemExit("No frames found")

if args.fps:
    fps = args.fps
else:
    avg_ms = max(1.0, sum(durations)/len(durations))
    fps = min(30.0, 1000.0/avg_ms)

fps_x100 = max(100, min(3000, round(fps*100)))
frame_bytes = W*H*3

with open(args.output, "wb") as f:
    f.write(struct.pack("<4sHHHHII", MAGIC, W, H, fps_x100, 0, len(frames), frame_bytes))
    for fr in frames:
        f.write(fr)

print(f"Wrote {args.output}: {len(frames)} frames @ {fps_x100/100:.2f} FPS")
