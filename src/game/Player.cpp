/**
 * @file Player.cpp
 * @brief Paletka: sterowanie osią, myszą albo autopilotem; MoveAndSlide() między ścianami.
 */

#include "game/Player.hpp"

#include "game/Ball.hpp"
#include "game/Layout.hpp"
#include "game/PowerUps.hpp"
#include "game/Round.hpp"
#include "game/Session.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kSpeed = 640.0f;       /**< Prędkość sterowania osią. */
constexpr float kMaxSpeed = 1500.0f;   /**< Największa prędkość (mysz, autopilot). */
constexpr float kWide = 1.5f;          /**< Mnożnik szerokości po poszerzeniu. */
constexpr float kLaserCooldown = 0.3f; /**< Odstęp strzałów. */

} // namespace

Player::Player() {
    width_ = targetWidth_ = layout::kPaddle.x;
    CollisionLayer(kPaddle).CollisionMask(kWalls);
    shape_ = &Spawn<node::collider::Rect>()
                  .Position({-layout::kPaddle.x / 2, -layout::kPaddle.y / 2})
                  .Size(layout::kPaddle);
    // arkusz z pliku JSON (format Aseprite) przez Assets z kontekstu silnika
    sprite_ = &Spawn<node::visual::Sprite>()
                   .Sheet("sprites/arcade.json")
                   .Play("paletka")
                   .Anchor({0.5f, 0.5f});
}

void Player::OnMount() {
    round_ = Context<Round>();
    session_ = Context<Session>();
    if (round_) round_->player = this;
}

void Player::Wide(bool value) {
    targetWidth_ = layout::kPaddle.x * (value ? kWide : 1.0f);
}

void Player::Laser(bool value) {
    sprite_->Play(value ? "paletka-laser" : "paletka");
}

float Player::AutoTarget() const {
    if (!round_) return Position().x;
    // najniżej lecąca piłka, która spada; trochę z boku środka - żeby odbicia miały kąt
    const Ball* lowest = nullptr;
    for (const Ball* ball : round_->balls)
        if (!ball->IsStuck() && ball->Velocity().y > 0.0f &&
            (!lowest || ball->WorldPosition().y > lowest->WorldPosition().y))
            lowest = ball;
    if (!lowest) return Position().x;
    // miejsce trafienia zmienia się powoli w czasie - piłka dostaje różne kąty odbicia
    const float aim =
        0.55f * HalfWidth() * std::sin(static_cast<float>(Engine::Get().Time()) * 0.9f);
    return lowest->WorldPosition().x - aim;
}

void Player::Tick(float dt, const Input& input) {
    if (!round_ || round_->paused) return;
    // płynna zmiana szerokości: sprite skalowany w poziomie, collider o tym samym rozmiarze
    if (width_ != targetWidth_) {
        const float step = 160.0f * dt;
        width_ = width_ < targetWidth_ ? std::min(targetWidth_, width_ + step)
                                       : std::max(targetWidth_, width_ - step);
        sprite_->Scale({width_ / layout::kPaddle.x, 1.0f});
        shape_->Position({-width_ / 2, -layout::kPaddle.y / 2}).Size({width_, layout::kPaddle.y});
    }
    // mysz przejmuje sterowanie, gdy się poruszy; oś - gdy zostanie użyta
    // kursor jest zablokowany w oknie (Engine::CursorLocked()), więc liczy się tylko jego ruch
    // (MouseDelta()); cel paletki nie wychodzi poza pole między ścianami
    const float dx = input.MouseDelta().x;
    if (std::fabs(dx) > 0.01f) {
        if (!mouseControl_) mouseX_ = Position().x;
        mouseControl_ = true;
        mouseX_ += dx;
    }
    if (input.Axis("ruch") != 0.0f) mouseControl_ = false;
    mouseX_ = std::clamp(mouseX_, layout::kField.x + HalfWidth(), layout::kRight - HalfWidth());

    bool launch = input.ActionPressed("start") || input.GestureDetected(GESTURE_DOUBLETAP);
    // autopilot sam wystrzeliwuje przyklejoną piłkę po chwili
    const bool autoplay = session_ && session_->autoplay;
    const bool anyStuck =
        std::ranges::any_of(round_->balls, [](const Ball* ball) { return ball->IsStuck(); });
    autoTimer_ = autoplay && anyStuck ? autoTimer_ + dt : 0.0f;
    if (autoTimer_ > 0.8f) {
        launch = true;
        autoTimer_ = 0.0f;
    }
    if (launch && (round_->phase == Phase::Ready || round_->phase == Phase::Playing)) {
        bool launched = false;
        for (Ball* ball : round_->balls)
            if (ball->IsStuck()) {
                ball->Launch();
                launched = true;
            }
        if (launched) round_->phase = Phase::Playing;
    }
    cooldown_ = std::max(0.0f, cooldown_ - dt);
    if (round_->laser > 0.0f && cooldown_ <= 0.0f && round_->phase == Phase::Playing &&
        (input.ActionDown("strzal") || autoplay)) {
        FireLaser(*round_, WorldPosition(), HalfWidth());
        cooldown_ = kLaserCooldown;
    }
}

void Player::FixedTick(float step, const Input& input) {
    if (!round_ || round_->phase == Phase::Cleared || round_->phase == Phase::GameOver) {
        Velocity({0, 0});
        return;
    }
    const float x = Position().x;
    float vx = 0.0f;
    if (session_ && session_->autoplay)
        vx = std::clamp((AutoTarget() - x) * 14.0f, -kMaxSpeed, kMaxSpeed);
    else if (mouseControl_)
        vx = std::clamp((mouseX_ - x) / step, -kMaxSpeed, kMaxSpeed);
    else
        vx = input.Axis("ruch") * kSpeed;
    // MoveAndSlide() zatrzymuje paletkę na ścianach (maska kWalls)
    Velocity({vx, 0.0f}).MoveAndSlide();
    if (Position().y != layout::kPaddleY) Position({Position().x, layout::kPaddleY});
}
