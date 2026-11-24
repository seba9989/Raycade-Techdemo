# Rykoszet - demo techniczne silnika Raycade

Gra zręcznościowa z paletką i piłką (gatunek „breakout”) na silniku
[Raycade](https://github.com/seba9989/Raycade): pięć poziomów, sześć rodzajów cegieł, siedem mocy,
tabela rekordów i własne poziomy z pliku. Demo jest zbudowane tak, żeby użyć możliwie wszystkich
funkcji silnika w naturalnych dla gry miejscach - tabela niżej mówi, gdzie której szukać.

## Kompilacja i uruchomienie

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/gra
```

Z [devenv](https://devenv.sh/): `build`, `run`, `fmt`, `clean`, `edit`.

Żeby pracować na lokalnej kopii silnika:

```sh
cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_RAYCADE=$HOME/Code/Raycade
# albo w devenv:
RAYCADE_DIR=~/Code/Raycade build
```

Zależności silnika (raylib 6.0, nlohmann/json 3.12.0, Box2D 3.1.1) CMake pobiera sam przy
pierwszej konfiguracji, w przypiętych wersjach - wersje zainstalowane w systemie są pomijane. Na
Linuksie potrzebne są tylko nagłówki X11 i OpenGL do kompilacji raylib (devenv je dostarcza).

## Sterowanie

| Akcja | Klawiatura / mysz | Pad |
|---|---|---|
| ruch paletki | strzałki / A D, ruch myszy (kursor zablokowany w oknie w trakcie gry) | lewa gałka, krzyżak |
| start piłki, puszczenie przyklejonej | spacja / strzałka w górę / LPM / podwójne stuknięcie | A |
| laser (moc) | X / lewy Ctrl / PPM | X |
| pauza | P / Esc | Start |
| menu | strzałki, Tab, Enter, mysz | krzyżak, A |

Klawisze globalne: F1 obrysy colliderów, F2 efekt kineskopu, F3 licznik FPS, F5 start/stop
nagrywania wejścia (od początku sceny), F6 odtworzenie nagrania, F11 pełny ekran, F12 zrzut ekranu
(katalog `zrzuty/` obok programu).

## Sceny

- **tytul** - menu z fokusem klawiatury i pole z imieniem gracza. W tle odbijają się piłki,
  a trafione cegły zmieniają kolor; kliknięcie w tło albo B dodaje piłkę.
- **ustawienia** - kineskop, pełny ekran i głośność (także kółkiem myszy i strzałkami), zapisywane
  w `rykoszet.txt` obok programu. Ten sam panel otwiera się w grze z menu pauzy (P/Esc →
  Ustawienia), bez przerywania rozgrywki.
- **gra** - jeden poziom; kolejny poziom to `ReloadScene("gra")` z nowym numerem w `Session`.
- **wyniki** - tabela rekordów w przewijanym panelu i wpis nowego rekordu.

Własny poziom: plik `.txt` przeciągnięty na okno (format opisuje `assets/levels/LEGENDA.txt`).

## Rozgrywka

- **Cegły**: zwykłe (6 kolorów), twarde (3 trafienia, pękają), metalowe (niezniszczalne,
  błyskają), wybuchowe (niszczą sąsiadów, także metal - reakcja łańcuchowa), ruchome i ukryte
  (widać je dopiero po pierwszym trafieniu).
- **Moce** z kapsuł: szersza paletka, multi-piłka, laser, klej, pole spowolnienia, siatka
  bezpieczeństwa i życie. Pozostały czas mocy pokazują paski w panelu.
- **Odbicie od paletki** zależy od miejsca trafienia; piłka przyspiesza z czasem.
- **Seria**: każde 4 cegły bez dotknięcia paletki podnoszą mnożnik punktów, a dźwięk cegły brzmi
  coraz wyżej. Za poziom jest premia (także za zapas żyć).
- Przy ostatnich cegłach muzyka przechodzi w szybszą ścieżkę.

## Funkcje silnika w demie

| Funkcja silnika | Gdzie |
|---|---|
| okno: `Title`, `Window`, `ViewSize`, `MinSize`, `Resizable`, `ScaleMode(Scaling::Canvas)`, `AspectMode(Aspect::Keep)`, `Letterbox`, `VSync`, `Background`, `ExitKey(KEY_NULL)`, `Fullscreen`, `CursorLocked` + `MouseDelta` (paletka), `CursorHidden`, `Icon` (klatka wycięta `Bitmap::Region()` i zapisana `Save()`) | `main.cpp`, `Session.cpp`, `GameScene.cpp` |
| pętla: `FixedStep` (1/120 s), pauza przez `FixedStep(0)`, `MaxDeltaTime`, `Time`, `CurrentFps`, `RandomSeed`/`RandomInt`/`RandomFloat`, `Quit`, `Log`/`LogFile` | `main.cpp`, `GameScene.cpp`, `Hud.cpp`, `PowerUps.cpp` |
| kontekst: silnika (`Session`, `DefaultTypeface`), sceny (`Round`), poddrzewa (`Palette` grupy cegieł, odczyt w `OnMount()`) | `main.cpp`, `GameScene.cpp`, `Level.cpp`, `Bricks.cpp` |
| sceny: `SCENE`, `StartScene`, `SwitchScene`, `ReloadScene`, `UnloadScene` opuszczanej sceny (przy powrocie powstaje od nowa, a silnik usuwa nieużywane zasoby), `ActiveScene` | `scenes/`, `main.cpp`, `Ui.cpp` |
| wejście: `BindAction`, `BindAxis` (klawisze, krzyżak i gałka pada), mysz, `MouseWheel`/`OnScroll`, `GestureDetected`, `KeyPressedRepeat`, `GamepadPressed`, `GamepadVibration` | `main.cpp`, `Player.cpp`, `Ui.cpp`, `GameScene.cpp` |
| UI: cecha `Focusable` (przyciski: `Focus`, `OnFocus`, `IsFocused`, Tab), `node::ui::TextField` (`OnChange`, `OnSubmit`), `OnClick`, `IsHovered` + `MouseDelta` (fokus od ruchu myszy), `OnKeyPressed`, blokowanie kliknięć pustym `OnClick` | `Ui.cpp`, `Settings.cpp`, `TitleScene.cpp`, `ScoresScene.cpp`, `GameScene.cpp` |
| kształty: `Rect`, `Circle`, `Ellipse` (cień piłki), `Polyline` (ślad piłki), kształty jako collidery `Visible()` (ściany, pole spowolnienia, siatka) | `Level.cpp`, `Ball.cpp`, `Effects.cpp` |
| `Sprite`: arkusz z JSON (format Aseprite: klatki, czasy klatek, tagi z kierunkiem), animacje, `Frame`, `Scale`, `SpriteSheet::Grid` + `Animation` + `Event` (wybuch), `OnFrameEvent`, `OnFinish` | `Bricks.cpp`, `Player.cpp`, `PowerUps.cpp`, `Effects.cpp`, `Hud.cpp` |
| `Canvas` + `PrimitiveRenderer`: gradient, wielokąt wypełniany scanline, odcinek przyrostowy, okrąg i elipsa punktu środkowego, `BoundaryFill`, `FloodFill`, `Points`, `Polyline`, `Blit` fragmentu arkusza | `Backdrop.cpp` |
| shadery: `RenderGroup` całego pola z shaderem fali uderzeniowej (uniformy co klatkę), `PostShader` kineskopu | `GameScene.cpp`, `Session.cpp`, `assets/shaders/` |
| rysowanie: `DrawLayer`, `Blend(BLEND_ADDITIVE)` (iskry, ślad, wybuch), `Visible`, `Paint` z przezroczystością, `ClipRect` (przewijana lista rekordów) | `Effects.cpp`, `Ball.cpp`, `ScoresScene.cpp` |
| hierarchia i przekształcenia: `Children`, `Spawn` w trakcie gry, `Destroy`, `Translate`, `Rotation`, `Scale`, `WorldPosition`, `ScreenSpace` (HUD, menu pauzy) | wszędzie |
| kamera: wstrząs przez `SceneCamera::Offset` (HUD w układzie ekranu się nie trzęsie) | `GameScene.cpp` |
| fizyka: `RigidBody` (piłka: `GravityScale(0)`, `Bounce(1)`, `Friction(0)`, `RotationLocked`, korekta `Velocity` w `FixedTick`; odłamki z grawitacją i `ApplyImpulse`), `CharacterBody` + `MoveAndSlide` (paletka), `StaticBody` (ściany, cegły, ruchome cegły przesuwane w `FixedTick`), `OnCollisionEnter` | `Ball.cpp`, `Player.cpp`, `Bricks.cpp`, `Effects.cpp`, `TitleScene.cpp` |
| obszary: `Area` z `OnBodyEnter` (kapsuły, strefa śmierci), `OverlappingBodies` (pole spowolnienia) | `PowerUps.cpp`, `Level.cpp`, `Ball.cpp` |
| kolizje: warstwy i maski, `OneWay` (siatka), `Disabled` (siatka bez mocy), `Scene::Overlap` (zasięg wybuchu), `Scene::Raycast` (pociski lasera), `DebugColliders` (F1), `Gravity` sceny | `Layout.hpp`, `Level.cpp`, `Bricks.cpp`, `PowerUps.cpp`, `main.cpp` |
| dźwięk: szyny z rodzicem (`Bus`, `Parent`, `Paused`), `Sound` z wariantami `Files`, `PitchVariation`, `MaxVoices`, `MinInterval`, `Volume`; `Voice::Pitch` (seria); `Music` z przenikaniem (`FadeInTime`/`FadeOutTime`), `Pause`/`Resume`/`Stop` | `main.cpp`, `Effects.cpp`, `GameScene.cpp` |
| pliki: `ApplicationDirectory`, `LoadText`/`SaveText` (ustawienia, rekordy, poziomy), `ListDirectory` (poziomy), `MakeDirectory`, `FileExists`, `OnFilesDropped` (własny poziom), `Screenshot` (F12), `StartRecording`/`StopRecording`/`Replay` (F5/F6) | `Session.cpp`, `Level.cpp`, `main.cpp` |

### Nie pokazane w demie

Celowo pominięte, bo w tej grze byłyby sztuczne: `SceneCamera::Follow`/`Zoom`/`Rotation`,
`Drawable::Shader` na pojedynczym obiekcie (shader ma cała grupa `RenderGroup` i obraz gry),
`OpenUrl`, `node::sound::Emitter`/`Listener` (dźwięk przestrzenny), `IntegerScaling`, monitory,
schowek, flagi okna (`AlwaysOnTop`, `Opacity`, `Undecorated`...) i dotyk. Pokazuje je program
`demo/` silnika.

### Propozycje dla silnika

- Ciągłe wykrywanie kolizji szybkich brył z ciałami kinematycznymi (np. `RigidBody::Bullet()`
  odpowiadające `b2BodyDef::isBullet`) - teraz gra ogranicza prędkość piłki.
- Bryła o stałej prędkości (materiał bez zmiany energii przy odbiciach) - teraz gra
  normalizuje prędkość piłki co krok stały.
- `Engine::Icon()` przyjmujące bitmapę - teraz ikona jest zapisywana do pliku i wczytywana z niego.
- Cień albo obrys napisu w `node::visual::Text` - teraz cień to drugi napis (`ShadowedText()`
  w `Ui.hpp`), bo dziecko jest zawsze rysowane nad rodzicem.

## Struktura

```
src/main.cpp              konfiguracja silnika, wejście, dźwięk, skróty globalne, autotest
src/scenes/               sceny: tytul, ustawienia, gra, wyniki
src/game/Layout.hpp       wymiary pola i warstwy kolizji
src/game/Session.*        stan gry (kontekst silnika), ustawienia i rekordy w pliku
src/game/Round.hpp        stan poziomu (kontekst sceny)
src/game/Level.*          plik poziomu: rozbiór i budowa pola
src/game/Player.*         gracz - paletka (CharacterBody)
src/game/Ball.*           piłka (RigidBody)
src/game/Bricks.*         cegły (StaticBody)
src/game/PowerUps.*       kapsuły (Area), moce, lasery
src/game/Effects.*        odłamki, iskry, napisy punktów, wybuch, wstrząs, muzyka
src/game/Backdrop.*       tło na płótnie (algorytmy z laboratoriów)
src/game/Hud.*            panel HUD i napis na środku pola
src/game/Ui.*             przycisk z fokusem, napis z cieniem, opóźnienie, przejścia
src/game/Settings.*       panel ustawień (scena "ustawienia" i menu pauzy)
assets/levels/            poziomy i legenda formatu
assets/shaders/           kineskop, fala uderzeniowa
assets/sprites/           arkusz sprite'ów (JSON w formacie Aseprite) i wybuch
tools/                    skrypty generujące grafikę, dźwięki i muzykę
```

## Zasoby

Wszystkie zasoby poza czcionką są generowane skryptami z katalogu `tools/` (tylko biblioteka
standardowa Pythona):

```sh
python3 tools/generate_sprites.py assets/sprites
python3 tools/generate_sounds.py assets/sounds
python3 tools/generate_music.py assets/music
```

Czcionka: DejaVu Sans (`assets/FONT-LICENSE.txt`).

## Autotest

Zmienne środowiskowe do sprawdzania gry bez ręcznego grania:

| Zmienna | Działanie |
|---|---|
| `GRA_SCENA=gra` | scena startowa (`tytul`, `ustawienia`, `gra`, `wyniki`) |
| `GRA_POZIOM=3` | numer poziomu (od 1) |
| `GRA_WLASNY=plik.txt` | poziom z pliku zamiast wbudowanych |
| `GRA_AUTO=1` | paletka sama goni piłkę, wystrzeliwuje ją i strzela laserem |
| `GRA_NAGRANIE=plik.txt` | odtworzenie nagrania wejścia (format zdarzeń raylib, np. z F5) |
| `GRA_ZRZUTY=katalog`, `GRA_CO=0.5` | zrzuty ekranu co podany czas |
| `GRA_CZAS=10` | zamknięcie gry po podanym czasie |

```sh
GRA_SCENA=gra GRA_POZIOM=3 GRA_AUTO=1 GRA_CZAS=20 GRA_ZRZUTY=/tmp/zrzuty ./build/gra
```
