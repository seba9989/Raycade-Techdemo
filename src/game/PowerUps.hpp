/**
 * @file PowerUps.hpp
 * @brief Moce: kapsuły (node::physics::Area spadające w kroku stałym), ich działanie i lasery.
 * @details Kapsuła wykrywa paletkę przez OnBodyEnter() - obszar nie zatrzymuje ciał, tylko
 * zgłasza wejście. Pociski lasera nie są ciałami: w każdym kroku rzucają promień
 * (Scene::Raycast()) od poprzedniego do nowego położenia, więc nie przelatują przez cegły.
 */

#pragma once

#include <raycade>

struct Round;
struct Session;

/** @brief Rodzaj mocy. */
enum class Power {
    Wide,  /**< Szersza paletka. */
    Multi, /**< Dwie dodatkowe piłki. */
    Laser, /**< Działka laserowe (akcja "strzal"). */
    Glue,  /**< Piłka przykleja się do paletki. */
    Slow,  /**< Pole spowolnienia na środku pola. */
    Net,   /**< Siatka bezpieczeństwa pod paletką. */
    Life,  /**< Dodatkowe życie. */
};

/** @brief Nazwa mocy w arkuszu ("moc-<nazwa>"). */
const char* PowerName(Power power);

/** @brief Podpis mocy w HUD-zie. */
const char* PowerLabel(Power power);

/** @brief Losuje moc (życie rzadziej). */
Power RandomPower();

/** @brief Kapsuła mocy. */
class Capsule : public node::physics::Area {
  public:
    /** @brief Buduje collider, sprite i reakcję na paletkę. */
    Capsule();

    /** @brief Ustawia rodzaj mocy (i animację sprite'a). */
    template <typename Self> Self& Kind(this Self& self, Power power) {
        self.power_ = power;
        self.sprite_->Play(std::string("moc-") + PowerName(power));
        return self;
    }

  protected:
    /** @brief Spadanie w kroku stałym; poniżej pola kapsuła znika. */
    void FixedTick(float step, const Input& input) override;

  private:
    Power power_ = Power::Wide;              /**< Rodzaj mocy. */
    node::visual::Sprite* sprite_ = nullptr; /**< Grafika (animacja obrotu). */
};

/**
 * @brief Włącza moc.
 * @param round Stan poziomu.
 * @param session Stan gry (życia).
 * @param power Moc.
 */
void ApplyPower(Round& round, Session& session, Power power);

/**
 * @brief Strzela parą pocisków z działek paletki.
 * @param round Stan poziomu (pole gry).
 * @param center Środek paletki.
 * @param halfWidth Połowa szerokości paletki.
 */
void FireLaser(Round& round, Vector2 center, float halfWidth);
