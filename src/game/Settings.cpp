/**
 * @file Settings.cpp
 * @brief Panel ustawień: przyciski z fokusem, zapis do Session i do pliku.
 */

#include "game/Settings.hpp"

#include "game/Session.hpp"
#include "game/Ui.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <vector>

namespace {

/** @brief Napis przełącznika. */
std::string Toggle(const char* name, bool value) {
    return std::format("{}: {}", name, value ? "wł." : "wył.");
}

/** @brief Napis głośności. */
std::string VolumeLabel(float volume) {
    return std::format("Głośność: {}%  ◀ ▶", static_cast<int>(volume * 100 + 0.5f));
}

} // namespace

node::Group& SpawnSettings(Entity& parent, Vector2 position, std::function<void()> back) {
    Engine& engine = Engine::Get();
    Session& session = *engine.Context<Session>();
    auto& panel = parent.Spawn<node::Group>().Position(position);

    // zmiana ustawienia: od razu w silniku i w pliku
    const auto changed = [&engine, &session] {
        session.Apply(engine);
        session.Save(engine);
    };
    const auto setVolume = [&session, changed](float value) {
        session.volume = std::clamp(std::round(value * 10.0f) / 10.0f, 0.0f, 1.0f);
        changed();
    };

    auto& crt = panel.Spawn<Button>().Action([&session, changed] {
        session.crt = !session.crt;
        changed();
    });
    auto& full = panel.Spawn<Button>().Position({0, 52}).Action([&session, changed] {
        session.fullscreen = !session.fullscreen;
        changed();
    });
    auto& volume = panel.Spawn<Button>()
                       .Position({0, 104})
                       .Action([&session, setVolume] {
                           setVolume(session.volume >= 1.0f ? 0.0f : session.volume + 0.1f);
                       })
                       .OnScroll([&session, setVolume](Button&, float wheel) {
                           setVolume(session.volume + (wheel > 0 ? 0.1f : -0.1f));
                       });
    auto& done = panel.Spawn<Button>().Label("Powrót").Position({0, 180}).Action(std::move(back));
    MenuNavigation(panel, {&crt, &full, &volume, &done});

    // napisy za stanem (także po F2/F11) i strzałki w lewo/prawo na głośności
    const auto refresh = [&session, &crt, &full, &volume] {
        crt.Label(Toggle("Kineskop", session.crt));
        full.Label(Toggle("Pełny ekran", session.fullscreen));
        volume.Label(VolumeLabel(session.volume));
    };
    refresh();
    panel.OnUpdate([&engine, &session, &volume, setVolume, refresh](node::Group&, float) {
        refresh();
        if (!volume.IsFocused()) return;
        const Input& input = engine.Input();
        if (input.KeyPressedRepeat(KEY_LEFT) || input.KeyPressed(KEY_LEFT))
            setVolume(session.volume - 0.1f);
        if (input.KeyPressedRepeat(KEY_RIGHT) || input.KeyPressed(KEY_RIGHT))
            setVolume(session.volume + 0.1f);
    });
    crt.Focus();
    return panel;
}
