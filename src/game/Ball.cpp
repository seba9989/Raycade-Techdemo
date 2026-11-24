/**
 * @file Ball.cpp
 * @brief Piłka: RigidBody z korektą prędkości w kroku stałym i ślad z node::shapes::Polyline.
 */

#include "game/Ball.hpp"

#include "game/Bricks.hpp"
#include "game/Effects.hpp"
#include "game/Layout.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr std::size_t kTrail = 10;    /**< Długość śladu (liczba położeń). */
constexpr float kMinVertical = 0.32f; /**< Najmniejsza pionowa część kierunku (bez lotu płasko). */
constexpr float kMaxBounce = 1.05f;   /**< Największy kąt od pionu przy odbiciu od paletki. */
constexpr float kSlowFactor = 0.55f;  /**< Prędkość w polu spowolnienia. */

} // namespace

Ball::Ball() {
    GravityScale(0)     // bez grawitacji sceny (odłamki ją mają)
        .Bounce(1.0f)   // odbicie bez straty energii
        .Friction(0.0f) // bez podkręcania przy ślizgu po ścianie
        .RotationLocked()
        .Mass(0.2f)
        .CollisionLayer(kBall)
        .CollisionMask(kWalls | kBricks | kPaddle | kNet);
    Spawn<node::collider::Circle>().Radius(layout::kBallRadius);
    // cień pod piłką - przesunięta, półprzezroczysta elipsa pod spritem
    Spawn<node::shapes::Ellipse>()
        .Position({5, 7})
        .Radius({8, 6})
        .Paint(Fade(BLACK, 0.35f))
        .Filled()
        .DrawLayer(-1);
    Spawn<node::visual::Sprite>().Sheet("sprites/arcade.json").Play("pilka").Anchor({0.5f, 0.5f});
    OnCollisionEnter([](Ball& self, const node::physics::Collision& hit) { self.Bounced(hit); });
}

void Ball::OnMount() {
    round_ = Context<Round>();
    if (!round_) return;
    round_->balls.push_back(this);
    if (round_->field)
        trail_ = &round_->field->Spawn<node::shapes::Polyline>()
                      .Paint(Fade(SKYBLUE, 0.45f))
                      .Blend(BLEND_ADDITIVE)
                      .DrawLayer(-1);
}

void Ball::Stick(float offset) {
    stuck_ = true;
    stickOffset_ = offset;
    Velocity({0, 0});
}

void Ball::Launch() {
    if (!stuck_ || !round_) return;
    stuck_ = false;
    // piłka na środku paletki poleciałaby pionowo - dostaje mały, losowy kąt
    float offset = stickOffset_;
    if (std::fabs(offset) < 6.0f) offset = Engine::Get().RandomInt(0, 1) ? 14.0f : -14.0f;
    const Vector2 direction = PaddleDirection(offset);
    Velocity({direction.x * round_->speed, direction.y * round_->speed});
}

void Ball::Lose() {
    if (IsDestroyed()) return;
    if (round_) std::erase(round_->balls, this);
    if (trail_) trail_->Destroy();
    trail_ = nullptr;
    Destroy();
}

Vector2 Ball::PaddleDirection(float offset) const {
    const float half = round_ && round_->player ? round_->player->HalfWidth() : 52.0f;
    const float side = std::clamp(offset / (half + layout::kBallRadius), -1.0f, 1.0f);
    const float angle = side * kMaxBounce;
    return {std::sin(angle), -std::cos(angle)};
}

void Ball::Tick(float dt, const Input&) {
    wallCooldown_ = std::max(0.0f, wallCooldown_ - dt);
    if (!trail_) return;
    history_.push_front(WorldPosition());
    while (history_.size() > kTrail)
        history_.pop_back();
    trail_->Vertices(std::vector<Point2D>(history_.begin(), history_.end()));
}

void Ball::FixedTick(float, const Input&) {
    if (!round_) return;
    if (stuck_) {
        // przyklejona: jedzie z paletką tuż nad nią (bez styku - nie ma zderzeń)
        if (round_->player)
            Position(
                {round_->player->Position().x + stickOffset_,
                 layout::kPaddleY - layout::kPaddle.y / 2 - layout::kBallRadius - 1.0f}
            );
        Velocity({0, 0});
        return;
    }
    Vector2 velocity = Velocity();
    float length = std::hypot(velocity.x, velocity.y);
    if (length < 1.0f) {
        velocity = {0.0f, -1.0f};
        length = 1.0f;
    }
    Vector2 direction{velocity.x / length, velocity.y / length};
    // płaski lot (prawie poziomo) - piłka krążyłaby bez końca między ścianami
    if (std::fabs(direction.y) < kMinVertical) {
        direction.y = direction.y < 0.0f ? -kMinVertical : kMinVertical;
        direction.x = std::copysign(std::sqrt(1.0f - kMinVertical * kMinVertical), direction.x);
    }
    float speed = round_->speed;
    if (round_->slow > 0.0f && round_->slowField) {
        // pole spowolnienia: obszar sam podaje ciała, które teraz są w nim
        const auto inside = round_->slowField->OverlappingBodies();
        if (std::ranges::find(inside, this) != inside.end()) speed *= kSlowFactor;
    }
    Velocity({direction.x * speed, direction.y * speed});
}

void Ball::Bounced(const node::physics::Collision& hit) {
    if (!round_ || stuck_) return;
    if (auto* player = dynamic_cast<Player*>(hit.other)) {
        // normalna od paletki do piłki: od góry wskazuje w górę; z boku zostaje odbicie fizyki
        if (hit.normal.y > -0.4f) return;
        const float offset = WorldPosition().x - player->WorldPosition().x;
        round_->combo = 0;
        fx::Play("paletka");
        if (round_->glue > 0.0f) {
            Stick(offset);
            return;
        }
        const Vector2 direction = PaddleDirection(offset);
        Velocity({direction.x * round_->speed, direction.y * round_->speed});
        return;
    }
    if (auto* brick = dynamic_cast<Brick*>(hit.other)) {
        brick->Hit();
        return;
    }
    if (round_->netShape && hit.otherShape == round_->netShape) {
        fx::Play("siatka");
        if (round_->field) fx::Sparks(*round_->field, WorldPosition(), SKYBLUE, 6);
        return;
    }
    if (wallCooldown_ <= 0.0f) {
        fx::Play("sciana");
        wallCooldown_ = 0.05f;
    }
}

Ball& SpawnBall(Round& round, Vector2 position, Vector2 velocity) {
    Ball& ball = round.field->Spawn<Ball>().Position(position).Velocity(velocity);
    if (velocity.x == 0.0f && velocity.y == 0.0f && round.player)
        ball.Stick(position.x - round.player->Position().x);
    return ball;
}
