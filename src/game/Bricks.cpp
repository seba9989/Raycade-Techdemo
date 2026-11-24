/**
 * @file Bricks.cpp
 * @brief Cegły: trafienia, punkty, efekty, wybuchy (Scene::Overlap()) i ruch w kroku stałym.
 */

#include "game/Bricks.hpp"

#include "game/Effects.hpp"
#include "game/Layout.hpp"
#include "game/PowerUps.hpp"
#include "game/Round.hpp"
#include "game/Session.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace {

constexpr float kBlastRadius = 62.0f; /**< Zasięg wybuchu (sąsiednie cegły, także po skosie). */
constexpr int kDropChance = 16;       /**< Szansa na kapsułę w procentach. */

/** @brief Punkty za rodzaj cegły (przed mnożnikiem serii). */
int BasePoints(BrickKind kind) {
    switch (kind) {
    case BrickKind::Hard:
        return 120;
    case BrickKind::Metal:
        return 200;
    case BrickKind::Explosive:
        return 80;
    case BrickKind::Moving:
        return 150;
    case BrickKind::Hidden:
        return 100;
    case BrickKind::Normal:
        break;
    }
    return 50;
}

} // namespace

Brick::Brick() {
    CollisionLayer(kBricks).CollisionMask(kBall);
    Spawn<node::collider::Rect>()
        .Position({-layout::kBrick.x / 2, -layout::kBrick.y / 2})
        .Size(layout::kBrick);
    sprite_ = &Spawn<node::visual::Sprite>().Sheet("sprites/arcade.json").Anchor({0.5f, 0.5f});
    Setup();
}

void Brick::OnMount() {
    palette_ = Context<Palette>();
    round_ = Context<Round>();
    session_ = Context<Session>();
}

void Brick::Setup() {
    switch (kind_) {
    case BrickKind::Normal:
    case BrickKind::Moving:
        hp_ = 1;
        sprite_->Play(std::format("cegla-{}", std::clamp(color_, 0, 5) + 1));
        break;
    case BrickKind::Hard:
        hp_ = 3;
        sprite_->Play("twarda-0");
        break;
    case BrickKind::Metal:
        hp_ = 1;
        sprite_->Play("metal"); // długa przerwa i szybki błysk - czasy klatek z JSON
        break;
    case BrickKind::Explosive:
        hp_ = 1;
        sprite_->Play("wybuchowa");
        break;
    case BrickKind::Hidden:
        hp_ = 2;
        sprite_->Play("ukryta").Visible(false); // collider działa, a grafiki nie widać
        break;
    }
}

Color Brick::Tint() const {
    const Palette fallback;
    const Palette& palette = palette_ ? *palette_ : fallback;
    switch (kind_) {
    case BrickKind::Hard:
        return palette.hard;
    case BrickKind::Metal:
        return palette.metal;
    case BrickKind::Explosive:
        return palette.explosive;
    case BrickKind::Hidden:
        return palette.hidden;
    case BrickKind::Normal:
    case BrickKind::Moving:
        break;
    }
    return palette.colors[static_cast<std::size_t>(std::clamp(color_, 0, 5))];
}

void Brick::Hit(bool blast) {
    if (broken_ || IsDestroyed()) return;
    Entity* field = round_ ? round_->field : nullptr;
    if (blast) {
        Break(true);
        return;
    }
    if (kind_ == BrickKind::Metal) {
        fx::Play("metal");
        sprite_->Frame(1); // błysk od razu po trafieniu
        if (field) fx::Sparks(*field, WorldPosition(), Tint(), 8);
        return;
    }
    --hp_;
    if (kind_ == BrickKind::Hidden && !sprite_->IsVisible()) {
        sprite_->Visible(true); // odkryta - od teraz widać ją jak zwykłą
        fx::Play("twarda");
        if (field) fx::Sparks(*field, WorldPosition(), Tint(), 10);
        return;
    }
    if (hp_ > 0) {
        if (kind_ == BrickKind::Hard) sprite_->Play(std::format("twarda-{}", 3 - hp_));
        fx::Play("twarda");
        if (field) fx::Sparks(*field, WorldPosition(), Tint(), 6);
        return;
    }
    Break(false);
}

void Brick::Break(bool blast) {
    broken_ = true;
    const Vector2 center = WorldPosition();
    if (round_ && session_) {
        if (IsCounted()) --round_->bricksLeft;
        ++round_->combo;
        const int multiplier = 1 + round_->combo / 4;
        const int points = BasePoints(kind_) * multiplier;
        session_->score += points;
        if (round_->field) fx::ScorePopup(*round_->field, center, points, multiplier);
        // każda kolejna cegła serii brzmi wyżej (Voice::Pitch() jednego odtworzenia)
        fx::Play("cegla", 1.0f + 0.05f * static_cast<float>(std::min(round_->combo, 14)));
    }
    if (round_ && round_->field) {
        fx::Debris(*round_->field, center, Tint(), kind_ == BrickKind::Hard ? 9 : 6);
        fx::Sparks(*round_->field, center, Tint(), 8);
        if (!blast && Engine::Get().RandomInt(0, 99) < kDropChance)
            round_->field->Spawn<Capsule>().Kind(RandomPower()).Position(center);
    }
    if (kind_ == BrickKind::Explosive) Explode();
    Destroy();
}

void Brick::Explode() {
    const Vector2 center = WorldPosition();
    fx::Play("wybuch");
    if (round_) {
        fx::Shake(*round_, 9.0f);
        if (round_->field) fx::Explosion(*round_->field, center);
    }
    Scene* scene = OwnerScene();
    if (!scene) return;
    // zapytanie o ciała w kole: same cegły (maska), bez tej, która wybucha
    for (node::physics::PhysicsBody* body : scene->Overlap(center, kBlastRadius, kBricks, this))
        if (auto* brick = dynamic_cast<Brick*>(body)) brick->Hit(true);
}

void Brick::FixedTick(float step, const Input&) {
    if (kind_ != BrickKind::Moving || (round_ && round_->phase != Phase::Playing)) return;
    // położenie z Position() zna się dopiero po Spawn() - OnMount() jest wcześniej
    if (time_ == 0.0f) homeX_ = Position().x;
    time_ += step;
    // StaticBody przesuwany w kodzie: silnik zamienia go w ciało kinematyczne z prędkością
    Position(
        {homeX_ + direction_ * 2.0f * layout::kBrick.x * std::sin(time_ * 1.3f), Position().y}
    );
}
