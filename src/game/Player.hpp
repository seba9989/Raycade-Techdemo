/**
 * @file Player.hpp
 * @brief Gracz - paletka sterowana kodem (node::physics::CharacterBody z MoveAndSlide()).
 * @details Paletka jeździ w poziomie między ścianami: osią "ruch" (klawiatura, krzyżak i gałka
 * pada), myszą albo sama (Session::autoplay). Bryły odbijają się od niej, ale ich nie pcha -
 * CharacterBody jest dla Box2D ciałem kinematycznym przestawianym bez prędkości, dlatego kąt
 * odbicia piłki ustawia Ball w OnCollisionEnter().
 */

#pragma once

#include <raycade>

struct Round;
struct Session;

/** @brief Paletka gracza. */
class Player : public node::physics::CharacterBody {
  public:
    /** @brief Buduje sprite (arkusz z JSON) i collider o tym samym rozmiarze. */
    Player();

    /** @brief Połowa bieżącej szerokości (do kąta odbicia). */
    float HalfWidth() const { return width_ / 2.0f; }

    /** @brief Włącza albo wyłącza poszerzenie (szerokość zmienia się płynnie). */
    void Wide(bool value);

    /** @brief Włącza albo wyłącza działka laserowe (inna animacja sprite'a). */
    void Laser(bool value);

  protected:
    /** @brief Odczytuje Round i Session z kontekstu. */
    void OnMount() override;

    /**
     * @brief Jednorazowe akcje (start, strzał) i szerokość - co klatkę.
     * @details Naciśnięcia sprawdza się w Tick(), bo w kroku stałym mogłyby zginąć.
     */
    void Tick(float dt, const Input& input) override;

    /** @brief Ruch w kroku stałym: oś, mysz albo autopilot i MoveAndSlide(). */
    void FixedTick(float step, const Input& input) override;

  private:
    /** @brief Położenie x, do którego zmierza autopilot (najniższa spadająca piłka). */
    float AutoTarget() const;

    node::visual::Sprite* sprite_ = nullptr; /**< Grafika. */
    node::collider::Rect* shape_ = nullptr;  /**< Collider. */
    Round* round_ = nullptr;                 /**< Stan poziomu. */
    Session* session_ = nullptr;             /**< Stan gry (autopilot). */
    float width_ = 0.0f;                     /**< Bieżąca szerokość. */
    float targetWidth_ = 0.0f;               /**< Docelowa szerokość. */
    float cooldown_ = 0.0f;                  /**< Czas do następnego strzału. */
    float autoTimer_ = 0.0f;                 /**< Autopilot: czas z przyklejoną piłką. */
    float mouseX_ = 0.0f;                    /**< Cel paletki ze sterowania myszą. */
    bool mouseControl_ = false;              /**< Steruje mysz (do pierwszego użycia osi). */
};
