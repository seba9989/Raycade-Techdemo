/**
 * @file Hud.cpp
 * @brief Panel HUD (ScreenSpace): napisy aktualizowane w OnUpdate, ikony żyć ze sprite'ów,
 * paski czasu mocy, licznik FPS i znacznik nagrywania wejścia.
 */

#include "game/Hud.hpp"

#include "game/Layout.hpp"
#include "game/PowerUps.hpp"
#include "game/Round.hpp"
#include "game/Session.hpp"
#include "game/Ui.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace {

constexpr float kPanelWidth = 232.0f;            /**< Szerokość panelu. */
constexpr Color kLabel{140, 150, 200, 255};      /**< Kolor podpisów. */
constexpr float kBarWidth = kPanelWidth - 40.0f; /**< Pełny pasek czasu mocy. */

/** @brief Podpis sekcji panelu. */
void Label(Entity& panel, Vector2 position, const char* text) {
    panel.Spawn<node::visual::Text>().Content(text).FontSize(15).Paint(kLabel).Position(position);
}

/** @brief Pasek pozostałego czasu jednej mocy. */
void PowerBar(Entity& panel, float y, Power power, const float& time, float full, Color color) {
    auto& row = panel.Spawn<node::Group>().Position({20, y});
    row.Spawn<node::visual::Text>().Content(PowerLabel(power)).FontSize(14).Paint(RAYWHITE);
    row.Spawn<node::shapes::Rect>()
        .Position({0, 20})
        .Size({kBarWidth, 6})
        .Paint(Fade(color, 0.25f))
        .Filled();
    auto& bar = row.Spawn<node::shapes::Rect>()
                    .Position({0, 20})
                    .Size({kBarWidth, 6})
                    .Paint(color)
                    .Filled();
    row.OnUpdate([&time, full, &bar](node::Group& self, float) {
        self.Visible(time > 0.0f);
        bar.Size({kBarWidth * std::clamp(time / full, 0.0f, 1.0f), 6});
    });
}

} // namespace

void SpawnHud(Scene& scene, Round& round, Session& session) {
    Engine& engine = Engine::Get();
    auto& panel = scene.Spawn<node::Group>()
                      .Position({layout::kPanelX, layout::kField.y - layout::kWall})
                      .ScreenSpace();
    panel.Spawn<node::shapes::Rect>()
        .Size({kPanelWidth, layout::kField.height + layout::kWall})
        .Paint(Color{20, 22, 40, 255})
        .Filled();
    panel.Spawn<node::shapes::Rect>()
        .Position({2, 2})
        .Size({kPanelWidth - 4, layout::kField.height + layout::kWall - 4})
        .Paint(Color{70, 78, 130, 255});

    ShadowedText(panel)
        .Content("RYKOSZET")
        .FontSize(30)
        .Paint(GOLD)
        .Anchor({0.5f, 0})
        .Position({kPanelWidth / 2, 14});

    // wynik liczony "w górę" do bieżącej wartości
    Label(panel, {20, 64}, "WYNIK");
    panel.Spawn<node::visual::Text>().FontSize(30).Paint(WHITE).Position({20, 82}).OnUpdate(
        [&session,
         shown = static_cast<float>(session.score)](node::visual::Text& self, float dt) mutable {
            const float target = static_cast<float>(session.score);
            shown = shown + (target - shown) * std::min(1.0f, dt * 10.0f);
            if (std::fabs(target - shown) < 1.0f) shown = target;
            self.Content("{:07}", static_cast<int>(shown));
        }
    );
    Label(panel, {20, 124}, "REKORD");
    panel.Spawn<node::visual::Text>()
        .FontSize(20)
        .Paint(Color{255, 210, 120, 255})
        .Position({20, 142})
        .OnUpdate([&session](node::visual::Text& self, float) {
            self.Content("{:07}", std::max(session.Best(), session.score));
        });

    Label(panel, {20, 180}, "POZIOM");
    panel.Spawn<node::visual::Text>()
        .FontSize(20)
        .Paint(WHITE)
        .WrapWidth(kPanelWidth - 40)
        .Position({20, 198})
        .Content(
            session.customLevel.empty()
                ? std::format("{}/{}  {}", session.level + 1, session.levels.size(), round.name)
                : std::format("własny  {}", round.name)
        );

    // życia: ikony piłek ze wspólnego arkusza
    Label(panel, {20, 256}, "ŻYCIA");
    for (int i = 0; i < 8; ++i)
        panel.Spawn<node::visual::Sprite>()
            .Sheet("sprites/arcade.json")
            .Play("pilka")
            .Position({20.0f + static_cast<float>(i) * 22.0f, 276})
            .OnUpdate([&session, i](node::visual::Sprite& self, float) {
                self.Visible(i < session.lives);
            });

    Label(panel, {20, 304}, "SERIA");
    panel.Spawn<node::visual::Text>().FontSize(22).Position({20, 322}).OnUpdate(
        [&round](node::visual::Text& self, float) {
            const int multiplier = 1 + round.combo / 4;
            self.Content("{} cegieł  ×{}", round.combo, multiplier)
                .Paint(multiplier > 1 ? GOLD : Color{200, 200, 215, 255});
        }
    );

    Label(panel, {20, 362}, "MOCE");
    PowerBar(panel, 382, Power::Wide, round.wide, 15, Color{90, 140, 255, 255});
    PowerBar(panel, 414, Power::Laser, round.laser, 12, Color{255, 90, 90, 255});
    PowerBar(panel, 446, Power::Glue, round.glue, 15, Color{90, 210, 110, 255});
    PowerBar(panel, 478, Power::Slow, round.slow, 10, Color{240, 150, 200, 255});
    PowerBar(panel, 510, Power::Net, round.net, 12, Color{90, 220, 255, 255});

    panel.Spawn<node::visual::Text>()
        .Content(
            "←/→, mysz, pad - ruch\nspacja/LPM - start\nX/Ctrl/PPM - laser\nP/Esc - pauza\n"
            "F1 collidery  F2 kineskop\nF3 FPS  F5/F6 nagranie\nF11 pełny ekran  F12 zrzut"
        )
        .FontSize(13)
        .LineHeight(1.35f)
        .Paint(kLabel)
        .Position({20, 552});

    // FPS i nagrywanie: stan silnika czytany co klatkę
    scene.Spawn<node::visual::Text>()
        .FontSize(16)
        .Paint(LIME)
        .Position({layout::kField.x + 6, layout::kField.y + 4})
        .ScreenSpace()
        .DrawLayer(20)
        .OnUpdate([&engine, &session](node::visual::Text& self, float) {
            self.Visible(session.showFps).Content("FPS: {}", engine.CurrentFps());
        });
    scene.Spawn<node::visual::Text>()
        .FontSize(16)
        .Position({layout::kRight - 6, layout::kField.y + 4})
        .Anchor({1, 0})
        .ScreenSpace()
        .DrawLayer(20)
        .OnUpdate([&engine](node::visual::Text& self, float) {
            const bool blink = std::fmod(engine.Time(), 1.0) < 0.6;
            if (engine.IsRecording())
                self.Content("● NAGRYWANIE").Paint(RED).Visible(blink);
            else if (engine.IsReplaying())
                self.Content("▶ ODTWARZANIE").Paint(SKYBLUE).Visible(true);
            else
                self.Visible(false);
        });
}

void SpawnMessage(Scene& scene, Round& round) {
    auto& message = ShadowedText(scene);
    round.message = &message.FontSize(34)
                         .Paint(WHITE)
                         .Align(TextAlign::Center)
                         .LineHeight(1.3f)
                         .Anchor({0.5f, 0.5f})
                         .Position(layout::kFieldCenter)
                         .ScreenSpace()
                         .DrawLayer(30)
                         .Visible(false)
                         .OnUpdate([time = 0.0f](node::visual::Text& self, float dt) mutable {
                             time += dt;
                             const float pulse = 1.0f + 0.04f * std::sin(time * 5.0f);
                             self.Scale({pulse, pulse});
                         });
}

void ShowMessage(Round& round, const std::string& text) {
    if (!round.message) return;
    round.message->Content(text).Visible(!text.empty());
}
