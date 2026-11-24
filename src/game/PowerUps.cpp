/**
 * @file PowerUps.cpp
 * @brief Kapsuły (Area), działanie mocy i pociski lasera (Scene::Raycast()).
 */

#include "game/PowerUps.hpp"

#include "game/Ball.hpp"
#include "game/Bricks.hpp"
#include "game/Effects.hpp"
#include "game/Layout.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "game/Session.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr float kFallSpeed = 150.0f;  /**< Prędkość spadania kapsuły. */
constexpr float kLaserSpeed = 900.0f; /**< Prędkość pocisku lasera. */
constexpr std::size_t kMaxBalls = 8;  /**< Najwięcej piłek naraz (multi). */

/**
 * @brief Pocisk lasera: sprite lecący w górę, który w każdym kroku stałym sprawdza promieniem
 * odcinek do nowego położenia - trafia cegłę albo sufit.
 */
class LaserShot : public node::visual::Sprite {
  public:
    LaserShot() {
        Sheet("sprites/arcade.json").Play("laser").Anchor({0.5f, 0.0f});
        Blend(BLEND_ADDITIVE);
    }

  protected:
    void FixedTick(float step, const Input&) override {
        Scene* scene = OwnerScene();
        if (!scene || IsDestroyed()) return;
        const Vector2 from = WorldPosition();
        const Vector2 to{from.x, from.y - kLaserSpeed * step};
        if (const auto hit = scene->Raycast(from, to, kBricks | kWalls)) {
            if (auto* brick = dynamic_cast<Brick*>(hit->body)) brick->Hit();
            if (Round* round = Context<Round>(); round && round->field)
                fx::Sparks(*round->field, hit->point, Color{255, 120, 120, 255}, 5);
            Destroy();
            return;
        }
        Position(to);
    }
};

} // namespace

const char* PowerName(Power power) {
    switch (power) {
    case Power::Wide:
        return "szersza";
    case Power::Multi:
        return "multi";
    case Power::Laser:
        return "laser";
    case Power::Glue:
        return "klej";
    case Power::Slow:
        return "wolniej";
    case Power::Net:
        return "siatka";
    case Power::Life:
        return "zycie";
    }
    return "szersza";
}

const char* PowerLabel(Power power) {
    switch (power) {
    case Power::Wide:
        return "Szersza paletka";
    case Power::Multi:
        return "Multi-piłka";
    case Power::Laser:
        return "Laser";
    case Power::Glue:
        return "Klej";
    case Power::Slow:
        return "Spowolnienie";
    case Power::Net:
        return "Siatka";
    case Power::Life:
        return "Życie";
    }
    return "";
}

Power RandomPower() {
    // życie ma 1 los na 13, pozostałe moce po 2
    const int roll = Engine::Get().RandomInt(0, 12);
    return roll == 12 ? Power::Life : static_cast<Power>(roll / 2);
}

Capsule::Capsule() {
    CollisionLayer(kCapsule).CollisionMask(kPaddle);
    Spawn<node::collider::Rect>().Position({-20, -10}).Size({40, 20});
    sprite_ = &Spawn<node::visual::Sprite>().Sheet("sprites/arcade.json").Anchor({0.5f, 0.5f});
    DrawLayer(2);
    OnBodyEnter([](Capsule& self, node::physics::PhysicsBody& body) {
        if (!dynamic_cast<Player*>(&body) || self.IsDestroyed()) return;
        Round* round = self.Context<Round>();
        Session* session = self.Context<Session>();
        if (round && session) {
            ApplyPower(*round, *session, self.power_);
            if (round->field) {
                fx::Sparks(*round->field, self.WorldPosition(), WHITE, 12);
                round->field->Spawn<node::visual::Text>()
                    .Content(PowerLabel(self.power_))
                    .FontSize(20)
                    .Anchor({0.5f, 0.5f})
                    .Position({self.WorldPosition().x, layout::kPaddleY - 40})
                    .DrawLayer(6)
                    .OnUpdate([life = 1.0f](node::visual::Text& text, float dt) mutable {
                        life -= dt;
                        text.Translate({0, -30 * dt}).Paint(Fade(WHITE, std::max(0.0f, life)));
                        if (life <= 0.0f) text.Destroy();
                    });
            }
        }
        self.Destroy();
    });
}

void Capsule::FixedTick(float step, const Input&) {
    // Area przesuwany w kodzie - obszar sprawdza wejścia ciał w nowym położeniu
    Translate({0, kFallSpeed * step});
    if (Position().y > layout::kViewHeight + 20) Destroy();
}

void ApplyPower(Round& round, Session& session, Power power) {
    fx::Play(power == Power::Life ? "zycie" : "moc");
    switch (power) {
    case Power::Wide:
        round.wide = 15.0f;
        if (round.player) round.player->Wide(true);
        break;
    case Power::Multi: {
        // kopia listy: nowe piłki dopisują się do round.balls w OnMount()
        const std::vector<Ball*> sources = round.balls;
        for (Ball* ball : sources) {
            if (round.balls.size() >= kMaxBalls) break;
            if (ball->IsDestroyed()) continue;
            Vector2 velocity = ball->Velocity();
            if (ball->IsStuck() || std::hypot(velocity.x, velocity.y) < 1.0f)
                velocity = {0, -round.speed};
            for (const float angle : {-0.45f, 0.45f}) {
                const float c = std::cos(angle), s = std::sin(angle);
                SpawnBall(
                    round, ball->WorldPosition(),
                    {velocity.x * c - velocity.y * s, velocity.x * s + velocity.y * c}
                );
            }
        }
        break;
    }
    case Power::Laser:
        round.laser = 12.0f;
        if (round.player) round.player->Laser(true);
        break;
    case Power::Glue:
        round.glue = 15.0f;
        break;
    case Power::Slow:
        round.slow = 10.0f;
        if (round.slowField) round.slowField->Visible(true);
        break;
    case Power::Net:
        round.net = 12.0f;
        if (round.netShape) round.netShape->Disabled(false).Visible(true);
        break;
    case Power::Life:
        ++session.lives;
        break;
    }
}

void FireLaser(Round& round, Vector2 center, float halfWidth) {
    if (!round.field) return;
    fx::Play("laser");
    for (const float side : {-1.0f, 1.0f})
        round.field->Spawn<LaserShot>().Position(
            {center.x + side * (halfWidth - 19.0f), center.y - layout::kPaddle.y / 2 - 6}
        );
}
