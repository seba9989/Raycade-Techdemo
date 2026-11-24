/**
 * @file GameScene.cpp
 * @brief Scena "gra": jeden poziom Rykoszetu - od wystrzału piłki do ukończenia albo przegranej.
 * @details
 * - kontekst sceny: Round (stan poziomu) - znika przy ReloadScene("gra"), które zaczyna
 *   następny poziom albo powtórkę; Session (kontekst silnika) przenosi wynik i życia,
 * - całe pole gry jest dziećmi jednej grupy node::visual::RenderGroup z shaderem fali
 *   uderzeniowej (uniformy ustawiane co klatkę: rozmiar, środek, czas, siła),
 * - wstrząs kamery: SceneCamera::Offset(); HUD jest obiektem ekranu, więc się nie trzęsie,
 * - pauza: Engine::FixedStep(0) zatrzymuje fizykę i wszystkie FixedTick(), szyna "efekty"
 *   dostaje Paused(), a muzyka Music::Pause(); menu pauzy ma przyciski z fokusem i panel
 *   ustawień (SpawnSettings()) - kineskop, pełny ekran i głośność bez wychodzenia z gry,
 * - w trakcie gry kursor jest zablokowany w oknie (Engine::CursorLocked()) - paletka idzie za
 *   ruchem myszy (Input::MouseDelta()) i nie da się nią wyjechać poza okno; pauza i koniec
 *   poziomu zwalniają kursor,
 * - moce działają przez czas z Round; po jego upływie scena je wyłącza,
 * - ostatnie cegły poziomu przełączają muzykę na szybszą (przenikanie szyny muzyki).
 */

#include <raycade>

#include "game/Backdrop.hpp"
#include "game/Ball.hpp"
#include "game/Effects.hpp"
#include "game/Hud.hpp"
#include "game/Layout.hpp"
#include "game/Level.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "game/Session.hpp"
#include "game/Settings.hpp"
#include "game/Ui.hpp"

#include <cmath>
#include <format>
#include <functional>
#include <memory>
#include <vector>

namespace {

constexpr float kStartSpeed = 360.0f; /**< Prędkość piłki na początku poziomu i po utracie. */
constexpr float kMaxSpeed = 600.0f;   /**< Największa prędkość piłki. */
constexpr float kSpeedGain = 2.5f;    /**< Przyrost prędkości na sekundę gry. */
constexpr int kTenseBricks = 5;       /**< Od tylu cegieł gra szybsza muzyka. */

/** @brief Stan sceny niepotrzebny obiektom pola (menu pauzy, poprzedni etap). */
struct SceneState {
    node::Group* pauseMenu = nullptr;         /**< Menu pauzy albo nullptr. */
    Phase lastPhase = Phase::Ready;           /**< Etap z poprzedniej klatki. */
    bool cursorLocked = false;                /**< Czy kursor jest zablokowany w oknie. */
    int startScore = 0;                       /**< Wynik na początku poziomu (powtórka). */
    int startLives = 0;                       /**< Życia na początku poziomu (powtórka). */
    std::function<void(bool)> setPaused;      /**< Włącza/wyłącza pauzę (też z przycisków menu). */
    node::Group* pauseContent = nullptr;      /**< Przyciski pauzy albo panel ustawień. */
    node::visual::Text* pauseTitle = nullptr; /**< Nagłówek menu pauzy. */
    bool inSettings = false;                  /**< Czy menu pauzy pokazuje ustawienia. */
    std::function<void()> showPause;          /**< Pokazuje przyciski pauzy. */
    std::function<void()> showSettings;       /**< Pokazuje panel ustawień. */
};

/** @brief Wyłącza wszystkie moce (utrata piłki). */
void ClearPowers(Round& round) {
    round.wide = round.laser = round.glue = round.slow = round.net = 0.0f;
    if (round.player) round.player->Wide(false), round.player->Laser(false);
    if (round.slowField) round.slowField->Visible(false);
    if (round.netShape) round.netShape->Disabled().Visible(false);
}

/** @brief Odlicza czas mocy; po upływie wywołuje funkcję wyłączającą. */
template <typename Off> void Countdown(float& time, float dt, Off off) {
    if (time <= 0.0f) return;
    time -= dt;
    if (time <= 0.0f) {
        time = 0.0f;
        off();
    }
}

/** @brief Nowa piłka przyklejona do paletki. */
void ServeBall(Round& round) {
    if (!round.player) return;
    SpawnBall(
        round,
        {round.player->Position().x,
         layout::kPaddleY - layout::kPaddle.y / 2 - layout::kBallRadius - 1.0f},
        {0, 0}
    );
    round.phase = Phase::Ready;
    ShowMessage(round, "spacja, LPM albo A - start");
}

} // namespace

/** @brief Konfiguruje scenę gry: poziom z Session, HUD, logika etapów i pauzy. */
SCENE("gra") {
    Session& session = *engine.Context<Session>();
    Round& round = *scene.ProvideContext<Round>().Context<Round>();
    auto state = std::make_shared<SceneState>();
    state->startScore = session.score;
    state->startLives = session.lives;

    const auto level = LoadLevel(engine, session.LevelPath());
    if (!level) {
        // błędny plik (np. upuszczony na okno) - powrót do menu z wpisem w logu
        session.customLevel.clear();
        Delay(scene, 0.0f, [&engine] { engine.SwitchScene("tytul").UnloadScene("gra"); });
        return;
    }

    // grawitacja działa tylko na odłamki - piłki mają GravityScale(0), kapsuły spadają w kodzie
    scene.Gravity({0, 900});
    round.speed = kStartSpeed;

    // pole gry: wszystkie obiekty poziomu rysują się do tekstury z shaderem fali uderzeniowej
    const auto wave = engine.Assets().Shader("shaders/shockwave.fs");
    round.field =
        &scene.Spawn<node::visual::RenderGroup>()
             .Size(
                 {static_cast<float>(layout::kViewWidth), static_cast<float>(layout::kViewHeight)}
             )
             .Background(Color{10, 10, 20, 255})
             .Shader(wave);
    SpawnBackdrop(*round.field, level->backdrop);
    BuildLevel(round, *level);
    SpawnHud(scene, round, session);
    SpawnMessage(scene, round);
    ShowMessage(
        round,
        session.customLevel.empty()
            ? std::format(
                  "POZIOM {}\n{}\n\nspacja, LPM albo A - start", session.level + 1, round.name
              )
            : std::format("{}\n\nspacja, LPM albo A - start", round.name)
    );
    fx::Music("music/gra.wav");
    fx::Play("start");
    FadeIn(scene);

    // ---------------- pauza
    // przyciski menu wołają setPaused przez surowy wskaźnik - stan żyje tyle co scena
    state->setPaused = [&engine, &scene, &round, self = state.get()](bool value) {
        if (round.paused == value) return;
        round.paused = value;
        engine.FixedStep(value ? 0.0f : layout::kFixedStep); // 0 - bez kroków stałych
        engine.Audio().Bus("efekty").Paused(value);
        fx::PauseMusic(value);
        if (!value) {
            if (self->pauseMenu) self->pauseMenu->Destroy();
            self->pauseMenu = self->pauseContent = nullptr;
            self->pauseTitle = nullptr;
            self->inSettings = false;
            return;
        }
        auto& menu = scene.Spawn<node::Group>().ScreenSpace().DrawLayer(50);
        self->pauseMenu = &menu;
        menu.Spawn<node::shapes::Rect>()
            .Size({static_cast<float>(layout::kViewWidth), static_cast<float>(layout::kViewHeight)})
            .Paint(Fade(BLACK, 0.6f))
            .Filled()
            .OnClick([](node::shapes::Rect&) {}); // pole gry pod spodem nie dostaje kliknięć
        self->pauseTitle = &ShadowedText(menu)
                                .FontSize(48)
                                .Paint(WHITE)
                                .Anchor({0.5f, 0.5f})
                                .Position({layout::kFieldCenter.x, 240});
        self->showPause();
    };

    // zawartość menu pauzy: przyciski albo panel ustawień - gra stoi przez cały czas
    const auto replaceContent = [self = state.get()] {
        if (self->pauseContent) self->pauseContent->Destroy();
        self->pauseContent = nullptr;
    };
    state->showPause = [&engine, &scene, self = state.get(), replaceContent] {
        if (!self->pauseMenu) return;
        replaceContent();
        self->inSettings = false;
        self->pauseTitle->Content("PAUZA");
        auto& content = self->pauseMenu->Spawn<node::Group>();
        self->pauseContent = &content;
        std::vector<Button*> buttons;
        const auto button = [&content,
                             &buttons](float y, const char* label, std::function<void()> fn) {
            buttons.push_back(&content.Spawn<Button>()
                                   .Label(label)
                                   .Action(std::move(fn))
                                   .Position({layout::kFieldCenter.x - 130, y}));
        };
        button(300, "Wznów", [self] { self->setPaused(false); });
        button(356, "Ustawienia", [self] { self->showSettings(); });
        button(412, "Od nowa", [&engine, self] {
            self->setPaused(false);
            Session& game = *engine.Context<Session>();
            game.score = self->startScore;
            game.lives = self->startLives;
            engine.ReloadScene("gra");
        });
        // pauza trwa w trakcie ściemniania - krok stały i szynę przywraca scena menu
        button(468, "Menu główne", [&scene] { FadeTo(scene, "tytul"); });
        MenuNavigation(content, buttons);
        buttons.front()->Focus();
    };
    state->showSettings = [self = state.get(), replaceContent] {
        if (!self->pauseMenu) return;
        replaceContent();
        self->inSettings = true;
        self->pauseTitle->Content("USTAWIENIA");
        // ten sam panel co w scenie "ustawienia"; "Powrót" wraca do przycisków pauzy
        self->pauseContent =
            &SpawnSettings(*self->pauseMenu, {layout::kFieldCenter.x - 130, 300}, [self] {
                self->showPause();
            });
    };

    // ---------------- logika etapów
    scene.OnUpdate([&engine, &scene, &round, &session, state, wave](float dt) {
        const Input& input = engine.Input();
        const bool canPause = round.phase == Phase::Playing || round.phase == Phase::Ready;
        if (input.ActionPressed("pauza") && canPause) {
            if (state->inSettings)
                state->showPause(); // Esc w ustawieniach wraca do menu pauzy
            else
                state->setPaused(!round.paused);
        }
        // kursor zablokowany w oknie w trakcie gry; w pauzie (menu) i po poziomie - wolny
        const bool lock = !round.paused && !session.autoplay && round.phase != Phase::Cleared &&
                          round.phase != Phase::GameOver;
        if (lock != state->cursorLocked) {
            engine.CursorLocked(lock);
            state->cursorLocked = lock;
        }
        if (round.paused) return;

        // ---- czas mocy
        Countdown(round.wide, dt, [&round] {
            if (round.player) round.player->Wide(false);
        });
        Countdown(round.laser, dt, [&round] {
            if (round.player) round.player->Laser(false);
        });
        Countdown(round.glue, dt, [&round] {
            for (Ball* ball : round.balls)
                ball->Launch(); // klej puszcza przyklejone piłki
        });
        Countdown(round.slow, dt, [&round] {
            if (round.slowField) round.slowField->Visible(false);
        });
        Countdown(round.net, dt, [&round] {
            if (round.netShape) round.netShape->Disabled().Visible(false);
        });

        // ---- wstrząs kamery (obiekty świata), HUD w układzie ekranu zostaje w miejscu
        if (round.shake > 0.0f) {
            round.shake *= std::exp(-dt * 7.0f);
            if (round.shake < 0.3f) round.shake = 0.0f;
            scene.Camera().Offset(
                {engine.RandomFloat(-1, 1) * round.shake, engine.RandomFloat(-1, 1) * round.shake}
            );
        } else {
            scene.Camera().Offset({0, 0});
        }

        // ---- fala uderzeniowa: uniformy shadera pola gry
        if (round.waveTime >= 0.0f) {
            round.waveTime += dt;
            if (round.waveTime > 0.8f) round.waveTime = -1.0f;
        }
        if (wave)
            wave->Uniform(
                    "size",
                    Vector2{
                        static_cast<float>(layout::kViewWidth),
                        static_cast<float>(layout::kViewHeight)
                    }
            )
                .Uniform("center", round.wave)
                .Uniform("time", round.waveTime)
                .Uniform("strength", 16.0f);

        // ---- etapy
        if (state->lastPhase == Phase::Ready && round.phase == Phase::Playing)
            ShowMessage(round, "");
        state->lastPhase = round.phase;
        if (round.phase == Phase::Playing) {
            round.speed = std::min(kMaxSpeed, round.speed + kSpeedGain * dt);
            if (round.bricksLeft > 0 && round.bricksLeft <= kTenseBricks)
                fx::Music("music/napiecie.wav");
        }

        if (round.phase == Phase::Playing && round.bricksLeft <= 0) {
            // poziom ukończony: premia za poziom i zapas żyć
            round.phase = Phase::Cleared;
            for (Ball* ball : std::vector<Ball*>(round.balls))
                ball->Lose();
            const int bonus = 1000 * (session.level + 1) + 250 * session.lives;
            session.score += bonus;
            fx::Play("poziom");
            fx::Shake(round, 4.0f);
            ShowMessage(round, std::format("POZIOM UKOŃCZONY!\npremia +{}", bonus));
            Delay(scene, 3.0f, [&engine, &scene, &session] {
                if (session.IsLastLevel()) {
                    session.victory = true;
                    session.pending = session.score;
                    FadeTo(scene, "wyniki");
                } else {
                    ++session.level;
                    engine.ReloadScene("gra");
                }
            });
            return;
        }

        if ((round.phase == Phase::Playing || round.phase == Phase::Ready) && round.balls.empty()) {
            ClearPowers(round);
            round.combo = 0;
            round.speed = kStartSpeed;
            fx::Play("strata");
            fx::Shake(round, 7.0f);
            engine.GamepadVibration(0.7f, 0.7f, 0.35f);
            if (session.lives <= 0) {
                round.phase = Phase::GameOver;
                fx::StopMusic();
                fx::Play("koniec");
                ShowMessage(round, "KONIEC GRY");
                Delay(scene, 3.0f, [&scene, &session] {
                    session.pending = session.score;
                    FadeTo(scene, "wyniki");
                });
                return;
            }
            --session.lives;
            round.phase = Phase::Lost;
            ShowMessage(
                round, session.lives == 0 ? "Ostatnia piłka!"
                                          : std::format("Zostało żyć: {}", session.lives)
            );
            Delay(scene, 1.4f, [&round] { ServeBall(round); });
        }
    });
}
