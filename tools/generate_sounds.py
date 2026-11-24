"""Syntezuje efekty dźwiękowe gry Rykoszet (WAV, 22050 Hz, 16 bit, mono).

Użycie: python3 tools/generate_sounds.py assets/sounds
"""
import math, random, struct, sys, wave

RATE = 22050
OUT = sys.argv[1] if len(sys.argv) > 1 else "assets/sounds"
random.seed(11)


def save(name, samples, peak_to=0.85):
    peak = max(1e-9, max(abs(s) for s in samples))
    scale = peak_to / peak
    with wave.open(f"{OUT}/{name}", "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes(b"".join(
            struct.pack("<h", int(max(-1, min(1, s * scale)) * 32767)) for s in samples))


def env(i, n, attack=0.005, release=1.0):
    t = i / RATE
    a = min(1.0, t / attack) if attack > 0 else 1.0
    return a * max(0.0, 1 - i / n) ** release


def tone(freq, t, shape="sine"):
    p = (freq * t) % 1.0
    if shape == "sine": return math.sin(2 * math.pi * p)
    if shape == "square": return 1.0 if p < 0.5 else -1.0
    if shape == "pulse": return 1.0 if p < 0.25 else -1.0
    if shape == "tri": return 4 * abs(p - 0.5) - 1
    if shape == "saw": return 2 * p - 1
    raise ValueError(shape)


def sweep(f0, f1, length, shape="square", release=1.5, attack=0.002):
    """Ton o częstotliwości zmieniającej się wykładniczo od f0 do f1 (faza całkowana)."""
    n = int(RATE * length); s = []; phase = 0.0
    for i in range(n):
        f = f0 * (f1 / f0) ** (i / n)
        phase += f / RATE
        s.append(tone(1.0, phase, shape) * env(i, n, attack, release))
    return s


def notes(seq, shape="square", gap=0.0, release=1.2):
    """Ciąg nut (częstotliwość, czas)."""
    s = []
    for f, length in seq:
        n = int(RATE * length)
        s += [tone(f, i / RATE, shape) * env(i, n, 0.003, release) for i in range(n)]
        s += [0.0] * int(RATE * gap)
    return s


def noise(length, cutoff=0.3, release=2.0):
    n = int(RATE * length); y = 0.0; s = []
    for i in range(n):
        y += cutoff * (random.uniform(-1, 1) - y)
        s.append(y * env(i, n, 0.001, release))
    return s


def add(a, b, gain=1.0):
    out = list(a) + [0.0] * max(0, len(b) - len(a))
    for i, v in enumerate(b):
        out[i] += v * gain
    return out


# odbicie od paletki: miękkie "bonk"
save("paletka.wav", add(sweep(520, 380, 0.09, "tri", 2.0), sweep(1040, 760, 0.05, "square", 3), 0.15))

# ściana: cichy, krótki stuk
save("sciana.wav", add(sweep(300, 220, 0.05, "tri", 3), noise(0.03, 0.5, 4), 0.3), 0.5)

# cegła: trzy warianty barwy (losuje je audio::Sound z listy Files())
save("cegla1.wav", sweep(880, 900, 0.08, "square", 2.5))
save("cegla2.wav", sweep(990, 1010, 0.07, "pulse", 2.5))
save("cegla3.wav", add(sweep(784, 800, 0.09, "tri", 2.0), sweep(1568, 1600, 0.05, "square", 3), 0.3))

# twarda cegła: trzask + niski ton
save("twarda.wav", add(noise(0.12, 0.7, 3), sweep(260, 200, 0.12, "square", 2), 0.4))

# metal: niewspółmierne składowe (dzwonienie)
n = int(RATE * 0.5)
save("metal.wav", [sum(math.sin(2 * math.pi * f * i / RATE) * g for f, g in
                       [(523, 1), (1410, 0.6), (2210, 0.4), (3170, 0.25)])
                   * env(i, n, 0.001, 4) for i in range(n)])

# wybuch: szum z opadającym basem
n = int(RATE * 0.9); s = []; y = 0.0
for i in range(n):
    t = i / RATE
    y += (0.5 * math.exp(-t * 3) + 0.02) * (random.uniform(-1, 1) - y)
    s.append((y * 1.4 + 0.8 * math.sin(2 * math.pi * (90 * math.exp(-t * 4) + 35) * t))
             * env(i, n, 0.002, 1.6))
save("wybuch.wav", s)

# moc: arpeggio w górę
save("moc.wav", notes([(523, .06), (659, .06), (784, .06), (1047, .12)], "square"))

# laser: opadające "pew"
save("laser.wav", sweep(1800, 300, 0.14, "saw", 1.4), 0.6)

# strata życia: opadająca melodia
save("strata.wav", notes([(494, .14), (440, .14), (392, .14), (330, .3)], "tri", 0.02, 0.8))

# życie: radosne "1UP"
save("zycie.wav", notes([(659, .08), (784, .08), (1319, .08), (1047, .08), (1175, .08), (1568, .2)],
                        "square"))

# start rundy
save("start.wav", notes([(392, .1), (523, .1), (659, .1), (784, .25)], "pulse", 0.02))

# koniec poziomu: fanfara
save("poziom.wav", add(notes([(523, .12), (523, .12), (523, .12), (659, .36), (587, .12), (659, .12),
                              (784, .5)], "square", 0.01),
                       notes([(262, .36), (330, .36), (392, .36), (523, .5)], "tri"), 0.6))

# przegrana
save("koniec.wav", notes([(392, .25), (370, .25), (349, .25), (330, .7)], "saw", 0.03, 0.6))

# siatka: brzęczenie pola
save("siatka.wav", add(sweep(200, 600, 0.25, "saw", 1), sweep(203, 606, 0.25, "saw", 1)), 0.6)

# interfejs: klik i wybór
n = int(RATE * 0.03)
save("klik.wav", [tone(2000, i / RATE) * env(i, n, 0.0005, 2) for i in range(n)], 0.5)
save("wybor.wav", notes([(880, .04), (1320, .07)], "square"), 0.5)
print("ok")
