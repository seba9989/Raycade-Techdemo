/**
 * @file Ui.cpp
 * @brief Przycisk z fokusem, cień napisu, opóźnienie, przejścia i nawigacja menu.
 */

#include "game/Ui.hpp"

#include "game/Layout.hpp"

#include <algorithm>

namespace {

constexpr Color kIdle{36, 40, 70, 255};    /**< Tło przycisku bez fokusu. */
constexpr Color kFocus{80, 100, 200, 255}; /**< Tło przycisku z fokusem. */

/** @brief Kolor pośredni. */
Color Mix(Color a, Color b, float t) {
    const auto lerp = [t](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) + (static_cast<float>(y) - x) * t);
    };
    return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
}

} // namespace

Button::Button() {
    Size({260, 44}).Paint(kIdle).Filled();
    label_ = &Spawn<node::visual::Text>()
                  .FontSize(22)
                  .Paint(RAYWHITE)
                  .Anchor({0.5f, 0.5f})
                  .Position({130, 22});
    OnClick([](Button& self) { self.Activate(); });
    OnFocus([](Button&) { Engine::Get().Audio().Sound("klik").Play(); });
}

void Button::Activate() {
    Engine::Get().Audio().Sound("wybor").Play();
    if (action_) action_();
}

void Button::Tick(float dt, const Input& input) {
    // fokus od myszy tylko przy jej ruchu: kursor zwolniony z blokady (pauza) pojawia się na
    // środku okna i bez tego zabierałby fokus przyciskowi wybranemu strzałkami
    const Vector2 moved = input.MouseDelta();
    if (IsHovered() && !IsFocused() && (moved.x != 0.0f || moved.y != 0.0f)) Focus();
    const float target = IsFocused() ? 1.0f : 0.0f;
    glow_ += (target - glow_) * std::min(1.0f, dt * 12.0f);
    Paint(Mix(kIdle, kFocus, glow_));
    label_->Paint(Mix(Color{200, 200, 215, 255}, WHITE, glow_));
    label_->Scale({1.0f + 0.08f * glow_, 1.0f + 0.08f * glow_});
    label_->Position({Size().x / 2, Size().y / 2});
    if (IsFocused() &&
        (input.ActionPressed("wybierz") || input.GamepadPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)))
        Activate();
}

void Delay(Scene& scene, float seconds, std::function<void()> action) {
    scene.Spawn<node::Group>().OnUpdate(
        [time = seconds, fn = std::move(action)](node::Group& self, float dt) mutable {
            time -= dt;
            if (time > 0.0f || self.IsDestroyed()) return;
            self.Destroy();
            fn();
        }
    );
}

void FadeIn(Scene& scene, float seconds) {
    scene.Spawn<node::shapes::Rect>()
        .Size({static_cast<float>(layout::kViewWidth), static_cast<float>(layout::kViewHeight)})
        .Paint(BLACK)
        .Filled()
        .ScreenSpace()
        .DrawLayer(100)
        .OnUpdate([seconds, time = 0.0f](node::shapes::Rect& self, float dt) mutable {
            time += dt;
            self.Paint(Fade(BLACK, std::max(0.0f, 1.0f - time / seconds)));
            if (time >= seconds) self.Destroy();
        });
}

void FadeTo(Scene& scene, std::string next, float seconds) {
    scene.Spawn<node::shapes::Rect>()
        .Size({static_cast<float>(layout::kViewWidth), static_cast<float>(layout::kViewHeight)})
        .Paint(Fade(BLACK, 0.0f))
        .Filled()
        .ScreenSpace()
        .DrawLayer(100)
        .OnClick([](node::shapes::Rect&) {}) // blokuje kliknięcia w trakcie przejścia
        .OnUpdate([seconds, name = std::move(next), time = 0.0f,
                   done = false](node::shapes::Rect& self, float dt) mutable {
            time += dt;
            self.Paint(Fade(BLACK, std::min(1.0f, time / seconds)));
            if (time >= seconds && !done) {
                done = true;
                // raz wczytana scena zostaje w pamięci, a SwitchScene() nie uruchamia jej
                // konfiguracji ponownie - opuszczana scena jest zwalniana, więc przy powrocie
                // powstaje od nowa (bez tego zaciemnienia i ze świeżym stanem)
                Engine& engine = Engine::Get();
                const std::string current = engine.ActiveScene();
                engine.SwitchScene(name).UnloadScene(current);
            }
        });
}

void MenuNavigation(Entity& parent, std::vector<Button*> buttons) {
    parent.Spawn<node::Group>().OnUpdate([list = std::move(buttons)](node::Group&, float) {
        const Input& input = Engine::Get().Input();
        int step = 0;
        if (input.KeyPressedRepeat(KEY_DOWN) || input.KeyPressed(KEY_DOWN) ||
            input.GamepadPressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN))
            step = 1;
        if (input.KeyPressedRepeat(KEY_UP) || input.KeyPressed(KEY_UP) ||
            input.GamepadPressed(GAMEPAD_BUTTON_LEFT_FACE_UP))
            step = -1;
        if (step == 0 || list.empty()) return;
        const auto focused = std::ranges::find_if(list, [](Button* b) { return b->IsFocused(); });
        const int count = static_cast<int>(list.size());
        const int current = focused == list.end() ? (step > 0 ? -1 : count)
                                                  : static_cast<int>(focused - list.begin());
        list[static_cast<std::size_t>((current + step + count) % count)]->Focus();
    });
}
