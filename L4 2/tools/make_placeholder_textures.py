"""
Generates PLACEHOLDER textures into GLProject/textures so the project runs out of the box.
Replace the files listed in README.md (cards, poster, ...) with your own images - same file names.

Run:   python tools/make_placeholder_textures.py
Needs: pip install pillow numpy
"""
import os
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

OUT = os.path.join(os.path.dirname(__file__), "..", "GLProject", "textures")
os.makedirs(OUT, exist_ok=True)
rng = np.random.default_rng(7)


def save(img, name, q=92):
    path = os.path.join(OUT, name)
    if name.endswith(".png"):
        img.save(path)
    else:
        img.convert("RGB").save(path, quality=q)


def noise(h, w, scale):
    """smooth value noise in 0..1"""
    small = rng.random((max(2, h // scale), max(2, w // scale)))
    return np.asarray(Image.fromarray((small * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)) / 255.0


def font(size):
    for f in ("DejaVuSans-Bold.ttf", "arialbd.ttf", "Arial Bold.ttf"):
        try:
            return ImageFont.truetype(f, size)
        except OSError:
            pass
    return ImageFont.load_default()


def wood(w, h, base, planks=0, horizontal=True):
    """wood grain; optional planks (floor)"""
    y = np.linspace(0, 1, h)[:, None]
    x = np.linspace(0, 1, w)[None, :]
    grain = np.sin((y * 90 + noise(h, w, 24) * 6) * np.pi) * 0.5 + 0.5
    fine = noise(h, w, 3)
    v = 0.55 + 0.25 * grain + 0.2 * fine
    img = np.zeros((h, w, 3))
    for c in range(3):
        img[..., c] = v * base[c]
    if planks:
        pw = w // planks
        for i in range(planks):
            shade = 0.82 + 0.18 * rng.random()
            img[:, i * pw:(i + 1) * pw] *= shade
            img[:, i * pw:i * pw + 3] *= 0.35  # dark seam
    return Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))


# ---------------- floor, table, dark wood ------------------------------------
save(wood(1024, 1024, (200, 140, 80), planks=6), "floor.jpg")
save(wood(1024, 512, (120, 70, 36)), "table.jpg")
save(wood(512, 512, (70, 38, 20)), "dark_wood.jpg")

# ---------------- wallpaper (damask-like stripes) ------------------------------
w = h = 512
img = Image.new("RGB", (w, h), (108, 38, 52))
d = ImageDraw.Draw(img)
for x in range(0, w, 128):
    d.rectangle([x, 0, x + 10, h], fill=(150, 70, 80))
    d.rectangle([x + 64 - 2, 0, x + 64 + 2, h], fill=(86, 28, 40))
for cy in range(32, h, 128):
    for cx in range(64, w, 128):
        d.ellipse([cx - 16, cy - 24, cx + 16, cy + 24], outline=(212, 170, 100), width=3)
        d.ellipse([cx - 6, cy - 6, cx + 6, cy + 6], fill=(212, 170, 100))
        d.polygon([(cx, cy - 40), (cx + 8, cy - 26), (cx - 8, cy - 26)], fill=(212, 170, 100))
        d.polygon([(cx, cy + 40), (cx + 8, cy + 26), (cx - 8, cy + 26)], fill=(212, 170, 100))
arr = np.asarray(img).astype(float) * (0.9 + 0.1 * noise(h, w, 4))[..., None]
save(Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)), "wall.jpg")

# ---------------- felt (table mats) ---------------------------------------------
f = 0.75 + 0.25 * noise(256, 512, 2)
arr = np.stack([f * 235, f * 235, f * 235], -1)
save(Image.fromarray(arr.astype(np.uint8)), "felt.jpg")

# ---------------- slot outline (rounded rectangle, alpha) -------------------------
sw, sh = 256, 384
slot = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
d = ImageDraw.Draw(slot)
d.rounded_rectangle([6, 6, sw - 7, sh - 7], radius=26, outline=(235, 200, 120, 230), width=7)
d.rounded_rectangle([20, 20, sw - 21, sh - 21], radius=18, outline=(235, 200, 120, 110), width=3)
d.rounded_rectangle([6, 6, sw - 7, sh - 7], radius=26, fill=(0, 0, 0, 55))
save(slot, "slot.png")

# ---------------- brass ----------------------------------------------------------------
y = np.linspace(0, 1, 256)[:, None]
b = 0.65 + 0.3 * np.sin(y * 18 + noise(256, 256, 8) * 4) + 0.1 * noise(256, 256, 2)
arr = np.stack([b * 215, b * 165, b * 70], -1) * np.ones((256, 256, 1))
save(Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)), "brass.jpg")

# ---------------- clock face -------------------------------------------------------------
c = 512
img = Image.new("RGB", (c, c), (70, 40, 20))
d = ImageDraw.Draw(img)
d.ellipse([20, 20, c - 20, c - 20], fill=(238, 228, 200), outline=(200, 150, 60), width=14)
for i in range(60):
    a = i / 60 * 2 * np.pi
    r0 = c / 2 - 40 if i % 5 else c / 2 - 70
    d.line([c / 2 + r0 * np.sin(a), c / 2 - r0 * np.cos(a), c / 2 + (c / 2 - 36) * np.sin(a), c / 2 - (c / 2 - 36) * np.cos(a)],
           fill=(30, 20, 15), width=4 if i % 5 == 0 else 2)
numerals = ["XII", "I", "II", "III", "IIII", "V", "VI", "VII", "VIII", "IX", "X", "XI"]
fn = font(40)
for i, n in enumerate(numerals):
    a = i / 12 * 2 * np.pi
    d.text((c / 2 + (c / 2 - 105) * np.sin(a), c / 2 - (c / 2 - 105) * np.cos(a)), n, fill=(30, 20, 15), font=fn, anchor="mm")
d.line([c / 2, c / 2, c / 2 + 120, c / 2 - 80], fill=(20, 15, 10), width=12)   # hour hand
d.line([c / 2, c / 2, c / 2 - 30, c / 2 - 190], fill=(20, 15, 10), width=8)     # minute hand
d.ellipse([c / 2 - 14, c / 2 - 14, c / 2 + 14, c / 2 + 14], fill=(200, 150, 60))
save(img, "clock_face.jpg")

# ---------------- globe (equirectangular map: blobs of land on sea) ----------------------
gw, gh = 1024, 512
sea = np.zeros((gh, gw, 3))
sea[...] = (40, 90, 150)
land_mask = noise(gh, gw, 40) * 0.7 + noise(gh, gw, 12) * 0.3
land = land_mask > 0.55
arr = np.where(land[..., None], np.array([95, 140, 70]) * (0.8 + 0.4 * noise(gh, gw, 4))[..., None], sea)
arr[:20] = (235, 240, 245)
arr[-20:] = (235, 240, 245)
img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8))
d = ImageDraw.Draw(img)
for i in range(0, gh, gh // 12):
    d.line([0, i, gw, i], fill=(255, 255, 255), width=1)
for i in range(0, gw, gw // 24):
    d.line([i, 0, i, gh], fill=(255, 255, 255), width=1)
save(img, "globe.jpg")

# ---------------- POSTER placeholder (REPLACE with your artwork) --------------------------
pw, ph = 680, 920
img = Image.new("RGB", (pw, ph))
arr = np.zeros((ph, pw, 3))
yy = np.linspace(0, 1, ph)[:, None]
arr[..., 0] = 25 + 40 * yy
arr[..., 1] = 40 + 80 * (1 - yy)
arr[..., 2] = 110 + 90 * (1 - yy)
img = Image.fromarray(arr.astype(np.uint8))
d = ImageDraw.Draw(img)
d.ellipse([pw * 0.62, ph * 0.08, pw * 0.88, ph * 0.08 + pw * 0.26], fill=(250, 230, 120))
d.polygon([(0, ph), (0, ph * 0.72), (pw * 0.35, ph * 0.60), (pw * 0.62, ph * 0.74), (pw * 0.85, ph * 0.62), (pw, ph * 0.7), (pw, ph)], fill=(20, 30, 45))
d.rectangle([20, 20, pw - 20, ph - 20], outline=(240, 230, 200), width=8)
d.text((pw / 2, ph * 0.90), "REPLACE poster.jpg", fill=(240, 230, 200), font=font(46), anchor="mm")
save(img, "poster.jpg")

# ---------------- CARD placeholders (REPLACE with your fictional characters) ---------------
palette = [(150, 60, 60), (50, 100, 150), (70, 130, 80), (130, 90, 150), (170, 120, 50)]
names = ["TEAM MEMBER", "CHARACTER 2", "CHARACTER 3", "CHARACTER 4", "CHARACTER 5"]
for i in range(5):
    cw, ch = 512, 768
    base = np.array(palette[i], dtype=float)
    yy = np.linspace(0, 1, ch)[:, None, None]
    arr = (base * (0.55 + 0.6 * (1 - yy)) * np.ones((ch, cw, 1)))
    img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(img)
    cx = cw // 2
    d.ellipse([cx - 90, 190, cx + 90, 370], fill=(235, 205, 170))          # head
    d.rounded_rectangle([cx - 150, 380, cx + 150, 700], radius=60, fill=(30, 30, 40))   # body
    d.polygon([(cx - 95, 215), (cx, 120), (cx + 95, 215)], fill=(30, 30, 40))
    d.rectangle([0, 0, cw, 70], fill=(20, 20, 20))
    d.text((cx, 36), names[i], fill=(240, 220, 150), font=font(36), anchor="mm")
    d.text((cx, 740), f"card_{i + 1}.jpg", fill=(240, 240, 240), font=font(28), anchor="mm")
    save(img, f"card_{i + 1}.jpg")

print("textures written to", os.path.abspath(OUT))
