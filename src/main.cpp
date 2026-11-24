/**
 * @file main.cpp
 * @brief Punkt wejścia Rykoszetu: konfiguracja silnika, kontekst gry, sterowanie, dźwięk,
 * skróty globalne, upuszczanie plików i autotest.
 * @details
 * - okno 960x720 skalowane do dowolnego rozmiaru (Scaling::Viewport - grafika pikselowa,
 *   Aspect::Keep z pasami w kolorze Letterbox()), ikona wycięta z arkusza sprite'ów
 *   (Bitmap::Region() + Save()), Esc nie zamyka programu (ExitKey(KEY_NULL)),
 * - krok stały 1/120 s (fizyka), MaxDeltaTime() chroni przed skokiem po zatrzymaniu okna,
 * - kontekst silnika: Session (stan gry, ustawienia, rekordy) i DefaultTypeface (czcionka
 *   z polskimi znakami dla wszystkich napisów),
 * - akcje i osie wejścia z klawiatury, myszy i pada,
 * - mikser: szyny z rodzicem, dźwięki z wariantami plików, limity głosów, muzyka z przenikaniem,
 * - klawisze globalne: F1 collidery, F2 kineskop (PostShader), F3 FPS, F5 nagrywanie wejścia,
 *   F6 odtworzenie nagrania, F11 pełny ekran, F12 zrzut ekranu,
 * - plik .txt upuszczony na okno to własny poziom (Engine::OnFilesDropped()),
 * - zmienne środowiskowe autotestu (GRA_*) - opis w README.
 */

#include <raycade>

#include "game/Layout.hpp"
#include "game/Session.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>

namespace {

constexpr unsigned int kReplaySeed = 2026; /**< Ziarno losowania przy nagraniu i odtworzeniu. */

/** @brief Wartość zmiennej środowiskowej ("" - brak). */
std::string Env(const char* name) {
    const char* value = std::getenv(name);
    return value ? value : "";
}

/** @brief Ścieżka pliku obok programu. */
std::string NextToProgram(const Engine& engine, const std::string& name) {
    return (std::filesystem::path(engine.ApplicationDirectory()) / name).string();
}

/** @brief Konfiguruje szyny, dźwięki i muzykę - raz, odtwarzane są po nazwie w całej grze. */
void SetupAudio(Audio& audio) {
    audio.Bus("efekty").MaxVoices(32);
    audio.Bus("cegly").Parent("efekty").Volume(0.8f); // ustawienia mnożą się w górę szyn
    audio.Bus("interfejs").Volume(0.8f);              // nie pauzuje się razem z efektami
    audio.Bus(Audio::kMusicBus).Volume(0.55f);        // jedna ścieżka naraz, z przenikaniem
    audio.Music("music/menu.wav").FadeInTime(1.0f).FadeOutTime(1.0f);
    audio.Music("music/gra.wav").FadeInTime(0.8f).FadeOutTime(0.8f);
    audio.Music("music/napiecie.wav").FadeInTime(0.4f).FadeOutTime(0.8f);

    // cegła: losowy plik z trzech, lekko zmienna wysokość, limit głosów i odstęp między nimi
    audio.Sound("cegla")
        .Files({"sounds/cegla1.wav", "sounds/cegla2.wav", "sounds/cegla3.wav"})
        .PitchVariation(0.03f)
        .MaxVoices(6)
        .MinInterval(0.02f)
        .Bus("cegly");
    const auto effect = [&audio](const char* name, float volume, int voices) {
        audio.Sound(name)
            .Files({std::format("sounds/{}.wav", name)})
            .Volume(volume)
            .MaxVoices(voices)
            .Bus("efekty");
    };
    effect("paletka", 0.8f, 3);
    effect("sciana", 0.45f, 3);
    effect("twarda", 0.8f, 4);
    effect("metal", 0.6f, 3);
    effect("wybuch", 0.9f, 3);
    effect("moc", 0.8f, 2);
    effect("laser", 0.5f, 4);
    effect("siatka", 0.6f, 2);
    effect("strata", 0.9f, 1);
    effect("zycie", 0.9f, 1);
    effect("start", 0.7f, 1);
    effect("poziom", 0.9f, 1);
    effect("koniec", 0.9f, 1);
    audio.Sound("klik").Files({"sounds/klik.wav"}).Volume(0.5f).MinInterval(0.03f).Bus("interfejs");
    audio.Sound("wybor").Files({"sounds/wybor.wav"}).Volume(0.6f).Bus("interfejs");
}

/** @brief Akcje i osie: klawiatura, mysz i pad - gra pyta tylko o nazwy. */
void SetupInput(Input& input) {
    input.BindAxis(
        "ruch", {bind::Key(KEY_LEFT), bind::Key(KEY_A), bind::Pad(GAMEPAD_BUTTON_LEFT_FACE_LEFT)},
        {bind::Key(KEY_RIGHT), bind::Key(KEY_D), bind::Pad(GAMEPAD_BUTTON_LEFT_FACE_RIGHT)},
        {bind::PadAxis(GAMEPAD_AXIS_LEFT_X, 1.0f)}
    );
    input.BindAction(
        "start", {bind::Key(KEY_SPACE), bind::Key(KEY_UP), bind::Mouse(MOUSE_BUTTON_LEFT),
                  bind::Pad(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)}
    );
    input.BindAction(
        "strzal", {bind::Key(KEY_X), bind::Key(KEY_LEFT_CONTROL), bind::Mouse(MOUSE_BUTTON_RIGHT),
                   bind::Pad(GAMEPAD_BUTTON_RIGHT_FACE_LEFT)}
    );
    input.BindAction(
        "pauza", {bind::Key(KEY_P), bind::Key(KEY_ESCAPE), bind::Pad(GAMEPAD_BUTTON_MIDDLE_RIGHT)}
    );
    input.BindAction(
        "wybierz", {bind::Key(KEY_ENTER), bind::Key(KEY_KP_ENTER), bind::Key(KEY_SPACE)}
    );
}

/** @brief Ikona okna: klatka "ikona" wycięta z arkusza i zapisana obok programu. */
void SetupIcon(Engine& engine) {
    const auto sheet = engine.Assets().Sheet("sprites/arcade.json");
    const SpriteAnimation* icon = sheet->FindAnimation("ikona");
    if (!icon || !sheet->Bitmap()) return;
    const std::string path = NextToProgram(engine, "ikona.png");
    if (sheet->Bitmap()->Region(sheet->Frame(icon->SheetFrame(0))).Save(path)) engine.Icon(path);
}

} // namespace

/**
 * @brief Konfiguruje silnik i uruchamia pętlę gry.
 * @return Kod wyjścia z Engine::Run().
 */
int main() {
    const std::string startScene = Env("GRA_SCENA").empty() ? "tytul" : Env("GRA_SCENA");
    Engine& engine =
        Engine::Get()
            .Title("Rykoszet - demo techniczne silnika Raycade")
            .Window(layout::kViewWidth, layout::kViewHeight)
            .ViewSize(layout::kViewWidth, layout::kViewHeight)
            .MinSize(layout::kViewWidth / 2, layout::kViewHeight / 2)
            .Resizable()
            .ScaleMode(Scaling::Canvas) // obraz 960x720 skalowany - piksele jak w automacie
            .AspectMode(Aspect::Keep)
            .Letterbox(Color{6, 6, 12, 255})
            .VSync()
            .Background(Color{12, 12, 24, 255})
            .ExitKey(KEY_NULL) // Esc to pauza, nie wyjście
            .FixedStep(layout::kFixedStep)
            .MaxDeltaTime(0.1f)
            .LogFile("engine.log")
            .StartScene(startScene)
            .Init();
    if (!engine.IsReady()) return 1;

    // stan gry dla wszystkich scen: kontekst silnika
    Session& session = *engine.ProvideContext<Session>().Context<Session>();
    session.Load(engine);
    for (const std::string& path : engine.ListDirectory(engine.AssetPath("levels"), ".txt"))
        if (std::filesystem::path(path).filename() != "LEGENDA.txt") session.levels.push_back(path);
    std::ranges::sort(session.levels);
    engine.Log(std::format("poziomy: {}", session.levels.size()));

    // czcionka z polskimi znakami dla node::visual::Text i node::ui::TextField
    engine.ProvideContext<DefaultTypeface>(engine.Assets().Font("font.ttf"));
    SetupIcon(engine);
    SetupInput(engine.Input());
    SetupAudio(engine.Audio());
    session.Apply(engine);

    // ---------------- autotest: zmienne środowiskowe (np. pod Xvfb)
    session.autoplay = Env("GRA_AUTO") == "1";
    if (const std::string level = Env("GRA_POZIOM"); !level.empty())
        session.level = std::clamp(
            std::atoi(level.c_str()) - 1, 0, static_cast<int>(session.levels.size()) - 1
        );
    if (const std::string custom = Env("GRA_WLASNY"); !custom.empty()) session.customLevel = custom;
    const float quitAfter =
        Env("GRA_CZAS").empty() ? 0.0f : static_cast<float>(std::atof(Env("GRA_CZAS").c_str()));
    const std::string shots = Env("GRA_ZRZUTY");
    const float shotEvery =
        Env("GRA_CO").empty() ? 1.0f : static_cast<float>(std::atof(Env("GRA_CO").c_str()));
    const std::string replay = Env("GRA_NAGRANIE");
    if (!shots.empty()) engine.MakeDirectory(shots);

    const std::string recording = NextToProgram(engine, "nagranie.txt");
    return engine
        .OnUpdate([&engine, &session, recording, quitAfter, shots, shotEvery, replay, time = 0.0f,
                   nextShot = 0.0f, shot = 0, replayStarted = false](float dt) mutable {
            const Input& input = engine.Input();
            time += dt;

            if (input.KeyPressed(KEY_F1)) {
                ::Scene& scene = engine.Scene(engine.ActiveScene());
                scene.DebugColliders(!scene.IsDebugColliders());
            }
            if (input.KeyPressed(KEY_F2)) {
                session.crt = !session.crt;
                session.Apply(engine);
                session.Save(engine);
            }
            if (input.KeyPressed(KEY_F3)) session.showFps = !session.showFps;
            if (input.KeyPressed(KEY_F11)) {
                session.fullscreen = !engine.IsFullscreen();
                session.Apply(engine);
                session.Save(engine);
            }
            if (input.KeyPressed(KEY_F12)) {
                const auto stamp =
                    std::chrono::system_clock::now().time_since_epoch() / std::chrono::seconds(1);
                engine.Screenshot(
                    NextToProgram(engine, std::format("zrzuty/rykoszet-{}.png", stamp))
                );
            }
            // nagranie wejścia od początku sceny (ta sama scena i ziarno przy odtworzeniu)
            if (input.KeyPressed(KEY_F5)) {
                if (engine.IsRecording()) {
                    engine.StopRecording(recording);
                } else {
                    engine.RandomSeed(kReplaySeed)
                        .ReloadScene(engine.ActiveScene())
                        .StartRecording();
                }
            }
            if (input.KeyPressed(KEY_F6) && !engine.IsRecording() && engine.FileExists(recording)) {
                engine.RandomSeed(kReplaySeed).ReloadScene(engine.ActiveScene());
                engine.Replay(recording);
            }

            // ---- autotest
            if (!replay.empty() && !replayStarted) {
                replayStarted = true;
                engine.RandomSeed(kReplaySeed);
                engine.Replay(replay);
            }
            if (!shots.empty() && time >= nextShot) {
                nextShot += shotEvery;
                engine.Screenshot(std::format("{}/{:04}.png", shots, shot++));
            }
            if (quitAfter > 0.0f && time >= quitAfter) engine.Quit();
        })
        .OnFilesDropped([&engine, &session](const std::vector<std::string>& files) {
            // pierwszy plik .txt to własny poziom - gra zaczyna się od razu
            for (const std::string& file : files) {
                if (std::filesystem::path(file).extension() != ".txt") continue;
                session.customLevel = file;
                session.NewGame();
                engine.Log("wlasny poziom: " + file);
                if (engine.ActiveScene() == "gra")
                    engine.ReloadScene("gra");
                else
                    engine.SwitchScene("gra").UnloadScene(engine.ActiveScene());
                return;
            }
        })
        .Run();
}
