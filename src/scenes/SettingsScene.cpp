/**
 * @file SettingsScene.cpp
 * @brief Scena "ustawienia": kineskop, pełny ekran i głośność.
 * @details
 * - panel ustawień (SpawnSettings()) - ten sam, który otwiera się z menu pauzy w grze,
 * - "Powrót", Esc albo przycisk Start pada wracają do menu głównego.
 */

#include <raycade>

#include "game/Effects.hpp"
#include "game/Layout.hpp"
#include "game/Settings.hpp"
#include "game/Ui.hpp"

/** @brief Konfiguruje ekran ustawień. */
SCENE("ustawienia") {
    engine.CursorLocked(false);
    engine.CursorHidden(false);
    fx::Music("music/menu.wav");

    scene.Spawn<node::shapes::Rect>()
        .Size({static_cast<float>(layout::kViewWidth), static_cast<float>(layout::kViewHeight)})
        .Paint(Color{14, 14, 30, 255})
        .Filled()
        .DrawLayer(-10);
    ShadowedText(scene)
        .Content("USTAWIENIA")
        .FontSize(56)
        .Paint(GOLD)
        .Anchor({0.5f, 0.5f})
        .Position({480, 110});

    auto& root = scene.Spawn<node::Group>();
    SpawnSettings(root, {350, 230}, [&scene] { FadeTo(scene, "tytul"); });

    scene.Spawn<node::visual::Text>()
        .Content(
            "strzałki w lewo/prawo albo kółko myszy - głośność   Esc - powrót\n"
            "ustawienia są też w menu pauzy (P/Esc w grze)"
        )
        .FontSize(15)
        .LineHeight(1.4f)
        .Align(TextAlign::Center)
        .Paint(Color{150, 160, 210, 255})
        .Anchor({0.5f, 0})
        .Position({480, 490});

    scene.OnUpdate([&engine, &scene, leaving = false](float) mutable {
        if (engine.Input().ActionPressed("pauza") && !leaving) {
            leaving = true; // jedno przejście, nawet przy kilku naciśnięciach
            FadeTo(scene, "tytul");
        }
    });
    FadeIn(scene);
}
