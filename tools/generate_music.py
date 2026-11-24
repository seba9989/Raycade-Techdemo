"""Syntezuje oryginalne pętle muzyczne gry Rykoszet (WAV, 22050 Hz, 16 bit, mono).

Każdy utwór ma długość równą całej liczbie taktów, więc zapętla się bez szwu
(audio::Music gra w pętli). Utwory:
- menu.wav   - spokojny, arpeggio na trójkącie,
- gra.wav    - główny motyw rozgrywki z perkusją,
- napiecie.wav - szybszy, w molu (ostatnie cegły poziomu).

Użycie: python3 tools/generate_music.py assets/music
"""
import math, random, struct, sys, wave

RATE = 22050
OUT = sys.argv[1] if len(sys.argv) > 1 else "assets/music"
random.seed(5)


def save(name, samples):
    peak = max(1e-9, max(abs(s) for s in samples))
    scale = 0.8 / peak
    with wave.open(f"{OUT}/{name}", "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes(b"".join(
            struct.pack("<h", int(max(-1, min(1, s * scale)) * 32767)) for s in samples))


def osc(freq, t, shape):
    p = (freq * t) % 1.0
    if shape == "sine": return math.sin(2 * math.pi * p)
    if shape == "square": return 1.0 if p < 0.5 else -1.0
    if shape == "pulse": return 1.0 if p < 0.25 else -1.0
    if shape == "tri": return 4 * abs(p - 0.5) - 1
    if shape == "saw": return 2 * p - 1
    raise ValueError(shape)


def note_freq(name):
    """'A4' -> 440 Hz, 'C#5' itd.; None - pauza."""
    if name is None: return None
    names = {"C": -9, "D": -7, "E": -5, "F": -4, "G": -2, "A": 0, "B": 2}
    semis = names[name[0]]; rest = name[1:]
    if rest.startswith("#"): semis += 1; rest = rest[1:]
    elif rest.startswith("b"): semis -= 1; rest = rest[1:]
    octave = int(rest)
    return 440.0 * 2 ** ((semis + (octave - 4) * 12) / 12)


class Song:
    def __init__(self, bpm, bars):
        self.beat = 60 / bpm
        self.n = int(round(RATE * self.beat * 4 * bars))
        self.s = [0.0] * self.n

    def note(self, start_beats, length_beats, freq, shape, gain, release=1.2, attack=0.004):
        if freq is None: return
        start = int(RATE * start_beats * self.beat)
        length = int(RATE * length_beats * self.beat)
        for i in range(length):
            k = (start + i) % self.n          # zawinięcie: ogon nuty przechodzi na początek pętli
            t = i / RATE
            a = min(1.0, t / attack) * max(0.0, 1 - i / length) ** release
            self.s[k] += gain * osc(freq, t, shape) * a

    def kick(self, beat_pos, gain=0.7):
        start = int(RATE * beat_pos * self.beat); n = int(RATE * 0.18)
        for i in range(n):
            t = i / RATE
            f = 50 + 110 * math.exp(-t * 30)
            self.s[(start + i) % self.n] += gain * math.sin(2 * math.pi * f * t) * (1 - i / n) ** 2

    def hat(self, beat_pos, gain=0.12, length=0.04):
        start = int(RATE * beat_pos * self.beat); n = int(RATE * length); prev = 0.0
        for i in range(n):
            x = random.uniform(-1, 1); y = x - prev; prev = x  # górnoprzepustowy
            self.s[(start + i) % self.n] += gain * y * (1 - i / n) ** 3

    def snare(self, beat_pos, gain=0.3):
        start = int(RATE * beat_pos * self.beat); n = int(RATE * 0.14); y = 0.0
        for i in range(n):
            y += 0.6 * (random.uniform(-1, 1) - y)
            t = i / RATE
            v = y * 0.8 + 0.4 * math.sin(2 * math.pi * 190 * t)
            self.s[(start + i) % self.n] += gain * v * (1 - i / n) ** 2


def chord(names):
    return [note_freq(x) for x in names.split()]


# ---------------- menu: 100 BPM, 8 taktów, arpeggio + bas + łagodna melodia
song = Song(100, 8)
prog = ["C4 E4 G4", "A3 C4 E4", "F3 A3 C4", "G3 B3 D4"] * 2
for bar, ch in enumerate(prog):
    root, third, fifth = chord(ch)
    for k, f in enumerate([root, third, fifth, root * 2, fifth * 2, root * 2, fifth, third]):
        song.note(bar * 4 + k * 0.5, 0.5, f * 2, "tri", 0.18)
    song.note(bar * 4, 4, root / 2, "sine", 0.4, 0.7)
melody = ["E5", None, "G5", "E5", "D5", None, "C5", None,
          "C5", None, "E5", "C5", "B4", None, "A4", None,
          "A4", None, "C5", "A4", "G4", None, "F4", "A4",
          "G4", None, "B4", "D5", "G5", None, None, None]
for k, name in enumerate(melody * 2):
    song.note(k, 1.0, note_freq(name), "pulse", 0.08, 1.5)
save("menu.wav", song.s)

# ---------------- gra: 132 BPM, 8 taktów, bas ósemkowy, perkusja, motyw
song = Song(132, 8)
prog = ["A3 C4 E4", "F3 A3 C4", "C4 E4 G4", "G3 B3 D4"] * 2
for bar, ch in enumerate(prog):
    root, third, fifth = chord(ch)
    for k in range(8):
        song.note(bar * 4 + k * 0.5, 0.45, root / 2 if k % 2 == 0 else root, "square", 0.12, 2)
    for k in range(4):
        song.kick(bar * 4 + k, 0.6)
        song.hat(bar * 4 + k + 0.5)
    song.snare(bar * 4 + 1); song.snare(bar * 4 + 3)
    for k, f in enumerate([third, fifth, third * 2, fifth]):
        song.note(bar * 4 + k, 0.9, f * 2, "tri", 0.07, 1.8)
lead = ["A5", "E5", "A5", "B5", "C6", "B5", "A5", "E5",
        "F5", "C5", "F5", "G5", "A5", "G5", "F5", "C5",
        "E5", "G5", "C6", "G5", "E5", "G5", "C6", "D6",
        "D6", "B5", "G5", "B5", "D6", "C6", "B5", "G5"]
for k, name in enumerate(lead * 2):
    song.note(k, 0.8 if k % 4 != 3 else 0.5, note_freq(name), "pulse", 0.09, 1.4)
save("gra.wav", song.s)

# ---------------- napięcie: 156 BPM, 4 takty, mol, szesnastki
song = Song(156, 4)
prog = ["D4 F4 A4", "Bb3 D4 F4", "C4 E4 G4", "A3 C#4 E4"]
for bar, ch in enumerate(prog):
    root, third, fifth = chord(ch)
    for k in range(16):
        f = [root, fifth, root * 2, third * 2][k % 4]
        song.note(bar * 4 + k * 0.25, 0.22, f * 2, "square", 0.07, 2)
    for k in range(8):
        song.note(bar * 4 + k * 0.5, 0.45, root / 2, "saw", 0.12, 2)
        song.hat(bar * 4 + k * 0.5 + 0.25, 0.1)
    for k in range(4):
        song.kick(bar * 4 + k, 0.7)
    song.snare(bar * 4 + 1, 0.35); song.snare(bar * 4 + 3, 0.35); song.snare(bar * 4 + 3.5, 0.2)
save("napiecie.wav", song.s)
print("ok")
