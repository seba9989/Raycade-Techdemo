/**
 * @file Effects.cpp
 * @brief Implementacja efektów: odłamki (RigidBody), iskry (Blend), napisy, wybuch (Sprite).
 */

#include "game/Effects.hpp"

#include "game/Layout.hpp"
#include "game/Round.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace {

/** @brief Bieżąca ścieżka muzyki ("" - cisza). */
std::string currentMusic;

/** @brief Losowa liczba z przedziału (generator silnika - działa RandomSeed()). */
float Random(float min, float max) {
    return Engine::Get().RandomFloat(min, max);
}

/**
 * @brief Arkusz wybuchu: 8 klatek 64x64 z siatki i animacja z jednym zdarzeniem klatki.
 * @details Arkusz z Assets jest tylko do odczytu, więc animację i zdarzenie dopisujemy do
 * własnego arkusza. Pamiętamy go słabym wskaźnikiem - po zwolnieniu sceny i Assets::Purge()
 * zniknie razem z bitmapą.
 */
std::shared_ptr<const SpriteSheet> ExplosionSheet() {
    static std::weak_ptr<const SpriteSheet> cache;
    if (auto sheet = cache.lock()) return sheet;
    auto sheet = std::make_shared<SpriteSheet>(
        SpriteSheet::Grid(Engine::Get().Assets().Bitmap("sprites/explosion.png"), {64, 64})
    );
    sheet->Animation("wybuch", 0, 8, 20, Playback::Once).Event("wybuch", 2, "fala");
    cache = sheet;
    return sheet;
}

} // namespace

namespace fx {

void Debris(Entity& parent, Vector2 center, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        const float size = Random(4, 8);
        const float mass = 0.05f;
        auto& piece = parent.Spawn<node::physics::RigidBody>()
                          .Position({center.x + Random(-18, 18), center.y + Random(-6, 6)})
                          .Mass(mass)
                          .Bounce(0.45f)
                          .Friction(0.3f)
                          .CollisionLayer(kDebris)
                          .CollisionMask(kWalls | kPaddle);
        piece.Spawn<node::collider::Rect>()
            .Position({-size / 2, -size / 2})
            .Size({size, size})
            .Paint(color)
            .Filled()
            .Visible();
        // impuls poza środkiem masy: odłamek leci i się obraca
        const Vector2 velocity{Random(-160, 160), Random(-260, -60)};
        piece
            .ApplyImpulse(
                {velocity.x * mass, velocity.y * mass},
                {piece.Position().x + Random(-3, 3), piece.Position().y + Random(-3, 3)}
            )
            .OnUpdate([life =
                           Random(1.2f, 1.8f)](node::physics::RigidBody& self, float dt) mutable {
                life -= dt;
                if (life <= 0.0f || self.Position().y > layout::kViewHeight + 40) self.Destroy();
            });
    }
}

void Sparks(Entity& parent, Vector2 center, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        const float angle = Random(0, 2 * PI);
        const float speed = Random(80, 260);
        Vector2 velocity{std::cos(angle) * speed, std::sin(angle) * speed};
        parent.Spawn<node::shapes::Circle>()
            .Position(center)
            .Radius(Random(1.5f, 3.0f))
            .Paint(color)
            .Filled()
            .Blend(BLEND_ADDITIVE) // nakładające się iskry rozjaśniają się
            .DrawLayer(5)
            .OnUpdate([velocity, color,
                       life = 0.45f](node::shapes::Circle& self, float dt) mutable {
                life -= dt;
                if (life <= 0.0f) {
                    self.Destroy();
                    return;
                }
                self.Translate({velocity.x * dt, velocity.y * dt});
                velocity.y += 300.0f * dt;
                self.Paint(Fade(color, std::min(1.0f, life / 0.3f)));
            });
    }
}

void ScorePopup(Entity& parent, Vector2 at, int points, int combo) {
    const Color base = combo > 1 ? GOLD : RAYWHITE;
    auto& text = parent.Spawn<node::visual::Text>().FontSize(18).Anchor({0.5f, 0.5f}).Position(at);
    if (combo > 1)
        text.Content("+{} ×{}", points, combo);
    else
        text.Content("+{}", points);
    text.Paint(base).DrawLayer(6).OnUpdate(
        [base, life = 0.0f](node::visual::Text& self, float dt) mutable {
            life += dt;
            if (life > 0.9f) {
                self.Destroy();
                return;
            }
            // obrót, skala i przesunięcie składają się w macierzy obiektu (Transformable)
            self.Translate({0, -40.0f * dt}).Rotation(0.12f * std::sin(life * 14.0f));
            self.Scale({1.0f + life * 0.5f, 1.0f + life * 0.5f});
            self.Paint(Fade(base, std::clamp(1.4f - life * 1.6f, 0.0f, 1.0f)));
        }
    );
}

void Explosion(Entity& parent, Vector2 center) {
    parent.Spawn<node::visual::Sprite>()
        .Sheet(ExplosionSheet())
        .Anchor({0.5f, 0.5f})
        .Position(center)
        .Scale({2.2f, 2.2f})
        .Blend(BLEND_ADDITIVE)
        .DrawLayer(6)
        .OnFrameEvent([center](node::visual::Sprite& self, const std::string& event) {
            if (event != "fala") return;
            if (Round* round = self.Context<Round>()) Shockwave(*round, center);
        })
        .OnFinish([](node::visual::Sprite& self, const std::string&) { self.Destroy(); })
        .Play("wybuch");
}

void Shake(Round& round, float strength) {
    round.shake = std::max(round.shake, strength);
}

void Shockwave(Round& round, Vector2 center) {
    round.wave = center;
    round.waveTime = 0.0f;
}

void Play(const std::string& name, float pitch) {
    audio::Voice voice = Engine::Get().Audio().Sound(name).Play();
    if (pitch != 1.0f) voice.Pitch(pitch);
}

void Music(const std::string& name) {
    audio::Music& music = Engine::Get().Audio().Music(name);
    if (currentMusic == name && music.VoiceCount() > 0) {
        if (music.IsPaused()) music.Resume();
        return;
    }
    currentMusic = name;
    music.Play();
}

void PauseMusic(bool value) {
    if (currentMusic.empty()) return;
    audio::Music& music = Engine::Get().Audio().Music(currentMusic);
    if (value)
        music.Pause();
    else
        music.Resume();
}

void StopMusic() {
    if (currentMusic.empty()) return;
    Engine::Get().Audio().Music(currentMusic).Stop();
    currentMusic.clear();
}

} // namespace fx
