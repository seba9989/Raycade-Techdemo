"""Rysuje arkusze sprite'ów gry Rykoszet (tylko biblioteka standardowa).

Wynik:
- assets/sprites/arcade.png + arcade.json - arkusz w formacie eksportu Aseprite ("Array"):
  klatki z czasami (duration w ms) i tagi animacji (frameTags) z kierunkiem i powtórzeniami,
- assets/sprites/explosion.png - 8 klatek 64x64 w jednym wierszu (SpriteSheet::Grid w grze).

Użycie: python3 tools/generate_sprites.py assets/sprites
"""
import json, math, struct, sys, zlib

OUT = sys.argv[1] if len(sys.argv) > 1 else "assets/sprites"


# ---------------------------------------------------------------- obraz RGBA i zapis PNG

class Image:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.px = bytearray(w * h * 4)

    def put(self, x, y, c):
        """Nakłada kolor (r, g, b, a) z mieszaniem alfa."""
        if not (0 <= x < self.w and 0 <= y < self.h): return
        i = (y * self.w + x) * 4
        r, g, b = c[0], c[1], c[2]
        a = c[3] if len(c) > 3 else 255
        if a >= 255:
            self.px[i:i + 4] = bytes((r, g, b, 255)); return
        da = self.px[i + 3]
        oa = a + da * (255 - a) // 255
        if oa == 0: return
        for k, v in enumerate((r, g, b)):
            self.px[i + k] = min(255, (v * a + self.px[i + k] * da * (255 - a) // 255) // oa)
        self.px[i + 3] = oa

    def rect(self, x, y, w, h, c):
        for j in range(y, y + h):
            for i in range(x, x + w):
                self.put(i, j, c)

    def blit(self, src, x, y):
        for j in range(src.h):
            for i in range(src.w):
                k = (j * src.w + i) * 4
                if src.px[k + 3]:
                    self.put(x + i, y + j, tuple(src.px[k:k + 4]))

    def save(self, path):
        raw = b"".join(b"\x00" + bytes(self.px[y * self.w * 4:(y + 1) * self.w * 4])
                       for y in range(self.h))
        def chunk(kind, data):
            body = kind + data
            return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))
        png = b"\x89PNG\r\n\x1a\n"
        png += chunk(b"IHDR", struct.pack(">IIBBBBB", self.w, self.h, 8, 6, 0, 0, 0))
        png += chunk(b"IDAT", zlib.compress(raw, 9))
        png += chunk(b"IEND", b"")
        with open(path, "wb") as f:
            f.write(png)


def mix(a, b, t):
    return tuple(int(a[k] + (b[k] - a[k]) * t) for k in range(3)) + (255,)


def shade(c, f):
    """f > 0 rozjaśnia do bieli, f < 0 przyciemnia do czerni."""
    return mix(c, (255, 255, 255) if f > 0 else (0, 0, 0), abs(f))


def rounded(img, x, y, w, h, r, c):
    """Wypełniony prostokąt z zaokrąglonymi rogami (wygładzanie 4x4 próbki)."""
    for j in range(h):
        for i in range(w):
            hits = 0
            for sy in range(4):
                for sx in range(4):
                    px, py = i + (sx + 0.5) / 4, j + (sy + 0.5) / 4
                    cx = min(max(px, r), w - r); cy = min(max(py, r), h - r)
                    if (px - cx) ** 2 + (py - cy) ** 2 <= r * r: hits += 1
            if hits:
                img.put(x + i, y + j, c[:3] + (int((c[3] if len(c) > 3 else 255) * hits / 16),))


def disc(img, cx, cy, r, c):
    for j in range(int(cy - r) - 1, int(cy + r) + 2):
        for i in range(int(cx - r) - 1, int(cx + r) + 2):
            hits = sum(1 for sy in range(4) for sx in range(4)
                       if (i + (sx + .5) / 4 - cx) ** 2 + (j + (sy + .5) / 4 - cy) ** 2 <= r * r)
            if hits:
                img.put(i, j, c[:3] + (int((c[3] if len(c) > 3 else 255) * hits / 16),))


def ring(img, cx, cy, r, width, c):
    for j in range(int(cy - r) - 2, int(cy + r) + 3):
        for i in range(int(cx - r) - 2, int(cx + r) + 3):
            hits = sum(1 for sy in range(4) for sx in range(4)
                       if abs(math.hypot(i + (sx + .5) / 4 - cx, j + (sy + .5) / 4 - cy) - r)
                       <= width / 2)
            if hits:
                img.put(i, j, c[:3] + (int((c[3] if len(c) > 3 else 255) * hits / 16),))


def line(img, x0, y0, x1, y1, c):
    n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
    for k in range(n + 1):
        t = k / n
        img.put(round(x0 + (x1 - x0) * t), round(y0 + (y1 - y0) * t), c)


# czcionka 5x7 tylko dla liter na kapsułach
GLYPHS = {
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    "W": ["10001", "10001", "10001", "10101", "10101", "11011", "10001"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "+": ["00000", "00100", "00100", "11111", "00100", "00100", "00000"],
}


def glyph(img, ch, x, y, c):
    for j, row in enumerate(GLYPHS[ch]):
        for i, bit in enumerate(row):
            if bit == "1": img.put(x + i, y + j, c)


# ---------------------------------------------------------------- klatki

BW, BH = 48, 24  # cegła
PALETTE = [(230, 72, 72), (245, 150, 50), (240, 210, 60), (90, 200, 90), (60, 190, 230),
           (170, 100, 230)]


def brick(color, bevel=3):
    img = Image(BW, BH)
    c = color + (255,)
    img.rect(0, 0, BW, BH, shade(c, -0.55))                 # obrys
    img.rect(1, 1, BW - 2, BH - 2, c)
    img.rect(1, 1, BW - 2, bevel, shade(c, 0.35))           # górna fazka
    img.rect(1, 1, bevel, BH - 2, shade(c, 0.2))            # lewa
    img.rect(1, BH - 1 - bevel, BW - 2, bevel, shade(c, -0.3))  # dolna
    img.rect(BW - 1 - bevel, 1, bevel, BH - 2, shade(c, -0.2))  # prawa
    img.rect(bevel + 3, bevel + 2, 10, 2, shade(c, 0.6))    # połysk
    return img


def hard_brick(stage):
    img = brick((120, 130, 160))
    for x in (6, BW - 7):
        for y in (6, BH - 7):
            disc(img, x + .5, y + .5, 1.6, (70, 75, 95, 255))
    crack = (40, 40, 55, 255)
    if stage >= 1:
        for a, b in [((14, 3), (19, 10)), ((19, 10), (16, 15)), ((19, 10), (26, 13)),
                     ((26, 13), (30, 21))]:
            line(img, *a, *b, crack)
    if stage >= 2:
        for a, b in [((36, 2), (33, 8)), ((33, 8), (38, 12)), ((38, 12), (35, 20)),
                     ((8, 18), (14, 14)), ((14, 14), (16, 15)), ((26, 13), (33, 8))]:
            line(img, *a, *b, crack)
    return img


def metal_brick(shine):
    img = Image(BW, BH)
    for y in range(BH):
        for x in range(BW):
            t = 0.5 + 0.5 * math.sin((x + y * 0.6) * 0.18)
            img.put(x, y, mix((150, 155, 165), (215, 220, 230), t * 0.6 + y / BH * -0.3 + 0.3))
    img.rect(0, 0, BW, 1, (240, 240, 250, 255)); img.rect(0, 0, 1, BH, (230, 230, 240, 255))
    img.rect(0, BH - 1, BW, 1, (70, 72, 80, 255)); img.rect(BW - 1, 0, 1, BH, (80, 82, 90, 255))
    if shine is not None:  # błysk przesuwający się po skosie
        for y in range(1, BH - 1):
            for d in range(-3, 4):
                x = shine - y // 2 + d
                if 1 <= x < BW - 1:
                    img.put(x, y, (255, 255, 255, 200 - abs(d) * 55))
    return img


def explosive_brick(lit):
    img = brick((210, 60, 40) if not lit else (255, 120, 60))
    disc(img, 24, 13, 6, (30, 25, 25, 255))
    img.rect(25, 3, 2, 5, (90, 70, 50, 255))
    disc(img, 27, 3, 2.2, (255, 230, 90, 255) if lit else (255, 170, 60, 255))
    return img


def hidden_brick():
    img = Image(BW, BH)
    img.rect(0, 0, BW, BH, (220, 240, 255, 60))
    for x in range(BW):
        img.put(x, 0, (230, 250, 255, 170)); img.put(x, BH - 1, (230, 250, 255, 120))
    for y in range(BH):
        img.put(0, y, (230, 250, 255, 170)); img.put(BW - 1, y, (230, 250, 255, 120))
    line(img, 6, 18, 14, 5, (255, 255, 255, 120))
    return img


PW, PH = 104, 20  # paletka


def paddle(glow, laser=False):
    img = Image(PW, PH)
    body = (70, 90, 200, 255)
    rounded(img, 0, 2, PW, PH - 4, 8, shade(body, -0.4))
    rounded(img, 1, 3, PW - 2, PH - 6, 7, body)
    img.rect(10, 4, PW - 20, 3, shade(body, 0.45))
    edge = mix((255, 120, 60), (255, 230, 120), glow)
    rounded(img, 0, 2, 14, PH - 4, 7, edge)                 # końcówki świecą
    rounded(img, PW - 14, 2, 14, PH - 4, 7, edge)
    if laser:
        for x in (16, PW - 22):
            img.rect(x, 0, 6, 8, (200, 60, 60, 255))
            img.rect(x + 2, 0, 2, 2, mix((255, 200, 200), (255, 255, 255), glow))
    return img


def ball():
    img = Image(16, 16)
    disc(img, 8, 8, 7.2, (40, 40, 60, 255))
    for r in range(7, 0, -1):
        t = r / 7
        disc(img, 8 - (1 - t) * 2, 8 - (1 - t) * 2, r * 0.95, mix((255, 255, 255), (170, 190, 230), t))
    return img


CW, CH = 40, 20  # kapsuła
POWERS = [("szersza", "S", (60, 110, 230)), ("multi", "M", (230, 160, 40)),
          ("laser", "L", (220, 60, 60)), ("klej", "K", (70, 190, 90)),
          ("wolniej", "W", (240, 150, 200)), ("siatka", "N", (60, 200, 220)),
          ("zycie", "+", (160, 160, 170))]


def capsule(color, letter, phase):
    img = Image(CW, CH)
    c = color + (255,)
    rounded(img, 0, 0, CW, CH, 9, shade(c, -0.5))
    rounded(img, 1, 1, CW - 2, CH - 2, 8, c)
    band = (phase * 5) % 18 + 1                             # pasek "obraca" kapsułę
    for y in range(max(1, band - 2), min(CH - 1, band + 2)):
        for x in range(3, CW - 3):
            img.put(x, y, (255, 255, 255, 70))
    glyph(img, letter, CW // 2 - 2, 6, (255, 255, 255, 255))
    return img


def laser_shot():
    img = Image(6, 18)
    for y in range(18):
        a = 255 if y > 3 else 120 + y * 30
        img.rect(1, y, 4, 1, (255, 90, 90, a)); img.put(2, y, (255, 230, 230, a)); img.put(3, y, (255, 230, 230, a))
    return img


def icon():
    img = Image(32, 32)
    rounded(img, 0, 0, 32, 32, 6, (25, 25, 45, 255))
    for k, col in enumerate(PALETTE[:3]):
        img.rect(4 + k * 8, 5, 7, 5, col + (255,))
    rounded(img, 6, 24, 20, 5, 2, (80, 100, 210, 255))
    disc(img, 19, 17, 3.5, (240, 240, 255, 255))
    return img


# ---------------------------------------------------------------- arkusz i JSON

frames = []   # (Image, duration_ms)
tags = []     # (name, from, to, direction, repeat)


def add(name, images, durations, direction="forward", repeat=None):
    first = len(frames)
    for img, ms in zip(images, durations):
        frames.append((img, ms))
    tags.append((name, first, len(frames) - 1, direction, repeat))


for k, color in enumerate(PALETTE, start=1):
    add(f"cegla-{k}", [brick(color)], [100])
for stage in range(3):
    add(f"twarda-{stage}", [hard_brick(stage)], [100])
# metal: długa przerwa, potem szybki błysk - czasy klatek z pliku (duration)
add("metal", [metal_brick(None), metal_brick(10), metal_brick(28), metal_brick(46)],
    [1800, 50, 50, 50])
add("wybuchowa", [explosive_brick(False), explosive_brick(True)], [350, 150])
add("ukryta", [hidden_brick()], [100])
add("paletka", [paddle(0.0), paddle(0.5), paddle(1.0)], [160, 160, 160], "pingpong")
add("paletka-laser", [paddle(0.0, True), paddle(0.5, True), paddle(1.0, True)], [90, 90, 90],
    "pingpong")
add("pilka", [ball()], [100])
for name, letter, color in POWERS:
    add(f"moc-{name}", [capsule(color, letter, p) for p in range(4)], [90] * 4)
add("laser", [laser_shot()], [100])
add("ikona", [icon()], [100])

# pakowanie wierszami do szerokości 256 z odstępem 1 px
WIDTH, PAD = 256, 1
x = y = PAD; row_h = 0; placed = []
for img, ms in frames:
    if x + img.w + PAD > WIDTH:
        x = PAD; y += row_h + PAD; row_h = 0
    placed.append((x, y)); x += img.w + PAD; row_h = max(row_h, img.h)
height = y + row_h + PAD
sheet = Image(WIDTH, height)
for (img, _), (px, py) in zip(frames, placed):
    sheet.blit(img, px, py)
sheet.save(f"{OUT}/arcade.png")

doc = {
    "frames": [
        {"filename": f"arcade {i}.aseprite", "frame": {"x": px, "y": py, "w": img.w, "h": img.h},
         "rotated": False, "trimmed": False,
         "spriteSourceSize": {"x": 0, "y": 0, "w": img.w, "h": img.h},
         "sourceSize": {"w": img.w, "h": img.h}, "duration": ms}
        for i, ((img, ms), (px, py)) in enumerate(zip(frames, placed))
    ],
    "meta": {
        "app": "tools/generate_sprites.py", "version": "1.0", "image": "arcade.png",
        "format": "RGBA8888", "size": {"w": WIDTH, "h": height}, "scale": "1",
        "frameTags": [
            dict({"name": n, "from": a, "to": b, "direction": d, "color": "#000000ff"},
                 **({"repeat": str(r)} if r else {}))
            for n, a, b, d, r in tags
        ],
        "layers": [{"name": "Layer 1", "opacity": 255, "blendMode": "normal"}], "slices": [],
    },
}
with open(f"{OUT}/arcade.json", "w") as f:
    json.dump(doc, f, indent=1)

# wybuch: 8 klatek 64x64 w wierszu - pierścień rośnie i blednie, w środku gasnący błysk
boom = Image(64 * 8, 64)
for k in range(8):
    t = k / 7
    cx = k * 64 + 32
    disc(boom, cx, 32, 6 + 18 * t, (255, 220, 120, int(230 * (1 - t) ** 1.5)))
    ring(boom, cx, 32, 8 + 22 * t, 6 * (1 - t) + 1.5, (255, 140, 60, int(255 * (1 - t))))
    for a in range(8):  # iskry
        ang = a * math.pi / 4 + 0.3
        r = 10 + 20 * t
        disc(boom, cx + math.cos(ang) * r, 32 + math.sin(ang) * r, 2.2 * (1 - t) + 0.4,
             (255, 255, 200, int(255 * (1 - t))))
boom.save(f"{OUT}/explosion.png")
print(f"ok: {len(frames)} klatek, arkusz {WIDTH}x{height}, {len(tags)} tagow")
