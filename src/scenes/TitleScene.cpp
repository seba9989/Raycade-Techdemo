/**
 * @file TitleScene.cpp
 * @brief Scena "tytul": menu główne z fokusem klawiatury i tło z odbijającymi się piłkami.
 * @details
 * - przyciski z cechą Focusable: fokus myszą, Tabem i strzałkami (KeyPressedRepeat()),
 *   aktywacja kliknięciem, Enterem albo przyciskiem A pada,
 * - pole tekstowe (node::ui::TextField) z imieniem gracza - przejmuje klawiaturę, gdy ma fokus,
 * - imię trafia do Session i do pliku zapisu przy starcie gry; ustawienia są w scenie
 *   "ustawienia",
 * - tło: piłki to bryły sztywne (RigidBody bez grawitacji), a cegły - StaticBody, które przy
 *   zderzeniu zmieniają kolor (OnCollisionEnter()); kliknięcie w tło albo B dodaje piłkę,
 * - litery logo falują (osobne napisy z cieniem przesuwane w OnUpdate).
 */

#include <raycade>

#include "game/Effects.hpp"
#include "game/Layout.hpp"
#include "game/Session.hpp"
#include "game/Ui.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr float kBallSpeed = 240.0f; /**< Prędkość piłek w tle. */
constexpr int kMaxBalls = 8;         /**< Najwięcej piłek w tle. */

/** @brief Piłka tła: bryła sztywna ze stałą prędkością. */
void SpawnDemoBall(Scene& scene, Vector2 position) {
    const float angle =
        Engine::Get().RandomFloat(0.3f, 1.2f) * (Engine::Get().RandomInt(0, 1) ? 1 : -1);
    auto& ball = scene.Spawn<node::physics::RigidBody>()
                     .Position(position)
                     .GravityScale(0)
                     .Bounce(1)
                     .Friction(0)
                     .RotationLocked()
                     .Velocity({std::sin(angle) * kBallSpeed, -std::cos(angle) * kBallSpeed})
                     .CollisionLayer(kBall)
                     .CollisionMask(kWalls | kBricks | kBall);
    ball.Spawn<node::collider::Circle>().Radius(layout::kBallRadius);
    ball.Spawn<node::visual::Sprite>()
        .Sheet("sprites/arcade.json")
        .Play("pilka")
        .Anchor({0.5f, 0.5f});
    // Box2D przy odbiciach zmienia energię - w kroku stałym przywracamy prędkość
    ball.OnFixedUpdate([](node::physics::RigidBody& self, float) {
        Vector2 v = self.Velocity();
        const float length = std::max(1.0f, std::hypot(v.x, v.y));
        if (std::fabs(v.y / length) < 0.25f) v.y = v.y < 0 ? -0.25f * length : 0.25f * length;
        const float fix = kBallSpeed / std::max(1.0f, std::hypot(v.x, v.y));
        self.Velocity({v.x * fix, v.y * fix});
    });
}

/** @brief Ramka ekranu i dekoracyjne cegły tła. */
void SpawnDemoField(Scene& scene) {
    const float w = static_cast<float>(layout::kViewWidth);
    const float h = static_cast<float>(layout::kViewHeight);
    auto& frame =
        scene.Spawn<node::physics::StaticBody>().CollisionLayer(kWalls).CollisionMask(kBall);
    for (const Rectangle r :
         {Rectangle{-20, 0, 20, h}, Rectangle{w, 0, 20, h}, Rectangle{0, -20, w, 20},
          Rectangle{0, h, w, 20}})
        frame.Spawn<node::collider::Rect>().Position({r.x, r.y}).Size({r.width, r.height});

    // dwa rzędy cegieł przy górze i dole ekranu; trafiona zmienia kolor na następny
    for (int row = 0; row < 2; ++row)
        for (int column = 0; column < 19; ++column) {
            const Vector2 center{
                24.0f + 48.0f * column + (row % 2) * 24.0f, row == 0 ? 20.0f : h - 20.0f
            };
            auto& brick = scene.Spawn<node::physics::StaticBody>()
                              .Position(center)
                              .CollisionLayer(kBricks)
                              .CollisionMask(kBall);
            brick.Spawn<node::collider::Rect>().Position({-24, -12}).Size(layout::kBrick);
            auto& sprite = brick.Spawn<node::visual::Sprite>()
                               .Sheet("sprites/arcade.json")
                               .Play(std::format("cegla-{}", (column + row * 3) % 6 + 1))
                               .Anchor({0.5f, 0.5f})
                               .Paint(Fade(WHITE, 0.55f));
            brick.OnCollisionEnter([&sprite, color = (column + row * 3) % 6](
                                       node::physics::StaticBody&, const node::physics::Collision&
                                   ) mutable {
                color = (color + 1) % 6;
                sprite.Play(std::format("cegla-{}", color + 1));
            });
        }
}

} // namespace

/** @brief Konfiguruje menu główne. */
SCENE("tytul") {
    Session& session = *engine.Context<Session>();
    session.autoplay = false;
    engine.CursorLocked(false); // kursor z gry wraca do menu
    engine.CursorHidden(false);
    // wyjście z pauzy gry przez menu: krok stały i efekty wracają tutaj
    engine.FixedStep(layout::kFixedStep);
    engine.Audio().Bus("efekty").Paused(false);
    scene.Gravity({0, 0});
    fx::Music("music/menu.wav");

    // tło: kliknięcie w puste miejsce albo B dodaje piłkę
    auto balls = std::make_shared<int>(0); // licznik piłek wspólny dla zdarzeń
    scene.Spawn<node::shapes::Rect>()
        .Size({static_cast<float>(layout::kViewWidth), static_cast<float>(layout::kViewHeight)})
        .Paint(Color{14, 14, 30, 255})
        .Filled()
        .DrawLayer(-10)
        .OnClick([&scene, &engine, balls](node::shapes::Rect&) {
            if (*balls >= kMaxBalls) return;
            ++*balls;
            SpawnDemoBall(scene, engine.Input().MousePosition());
        })
        .OnKeyPressed(KEY_B, [&scene, balls](node::shapes::Rect&) {
            if (*balls >= kMaxBalls) return;
            ++*balls;
            SpawnDemoBall(scene, {480, 600});
        });
    SpawnDemoField(scene);
    for (int i = 0; i < 3; ++i) {
        ++*balls;
        SpawnDemoBall(scene, {200.0f + 280.0f * i, 560});
    }

    // logo: każda litera osobno, z cieniem; fala przesuwa je w pionie
    const std::string logo = "RYKOSZET";
    for (std::size_t i = 0; i < logo.size(); ++i) {
        const float x = 480.0f + (static_cast<float>(i) - 3.5f) * 62.0f;
        ShadowedText(scene)
            .Content(std::string(1, logo[i]))
            .FontSize(76)
            .Paint(GOLD)
            .Anchor({0.5f, 0.5f})
            .Position({x, 110})
            .OnUpdate([x, i, time = 0.0f](node::visual::Text& self, float dt) mutable {
                time += dt;
                const float phase = time * 3.0f - static_cast<float>(i) * 0.5f;
                self.Position({x, 110 + 10 * std::sin(phase)}).Rotation(0.06f * std::cos(phase));
            });
    }
    ShadowedText(scene)
        .Content("demo techniczne silnika Raycade")
        .FontSize(22)
        .Paint(Color{190, 195, 230, 255})
        .Anchor({0.5f, 0.5f})
        .Position({480, 180});

    // imię gracza
    scene.Spawn<node::visual::Text>()
        .Content("Imię gracza")
        .FontSize(16)
        .Paint(Color{150, 160, 210, 255})
        .Position({350, 214});
    auto& name = scene.Spawn<node::ui::TextField>()
                     .Placeholder("Twoje imię")
                     .MaxLength(16)
                     .Size({260, 38})
                     .FontSize(20)
                     .Content(session.player)
                     .Position({350, 234});
    name.OnChange([&session](node::ui::TextField& self, const std::string& text) {
        std::string clean = text;
        std::erase(clean, ';'); // średnik rozdziela pola w pliku zapisu
        if (clean != text) self.Content(clean);
        if (!clean.empty()) session.player = clean;
    });

    // ---------------- menu
    auto& menu = scene.Spawn<node::Group>();
    std::vector<Button*> buttons;
    const auto button =
        [&menu, &buttons](float y, std::string label, std::function<void()> fn) -> Button& {
        Button& b =
            menu.Spawn<Button>().Label(std::move(label)).Action(std::move(fn)).Position({350, y});
        buttons.push_back(&b);
        return b;
    };

    Button& play = button(292, "Graj", [&scene, &engine, &session] {
        session.NewGame();
        session.customLevel.clear();
        session.Save(engine);
        FadeTo(scene, "gra");
    });
    button(344, "Ustawienia", [&scene] { FadeTo(scene, "ustawienia"); });
    button(396, "Rekordy", [&scene] { FadeTo(scene, "wyniki"); });
    button(448, "Wyjście", [&engine, &session] {
        session.Save(engine);
        engine.Quit();
    });
    MenuNavigation(menu, buttons);
    name.OnSubmit([&play](node::ui::TextField&, const std::string&) { play.Focus(); });
    play.Focus();

    scene.Spawn<node::visual::Text>()
        .Content(
            "Przeciągnij na okno plik .txt z własnym poziomem (format: assets/levels/LEGENDA.txt)\n"
            "kliknięcie w tło albo B - nowa piłka   F1 collidery   F2 kineskop   F3 FPS   F11 "
            "pełny "
            "ekran   F12 zrzut"
        )
        .FontSize(14)
        .LineHeight(1.4f)
        .Align(TextAlign::Center)
        .Paint(Color{150, 160, 210, 255})
        .Anchor({0.5f, 0})
        .Position({480, 622});
    FadeIn(scene);
}
