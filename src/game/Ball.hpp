/**
 * @file Ball.hpp
 * @brief Piłka - bryła sztywna Box2D (node::physics::RigidBody) bez grawitacji i tarcia.
 * @details Odbicia od ścian i cegieł liczy fizyka (Bounce(1)), a gra pilnuje tylko, żeby piłka
 * miała stałą prędkość i nie leciała zbyt płasko - Box2D przy odbiciach nieznacznie zmienia
 * energię. Kąt odbicia od paletki zależy od miejsca trafienia i jest ustawiany w
 * OnCollisionEnter().
 */

#pragma once

#include <raycade>

#include <deque>

struct Round;

/** @brief Piłka. */
class Ball : public node::physics::RigidBody {
  public:
    /** @brief Buduje collider, sprite, cień i reakcję na zderzenia. */
    Ball();

    /**
     * @brief Przykleja piłkę do paletki (start rundy albo moc „klej”).
     * @param offset Przesunięcie od środka paletki w poziomie.
     */
    void Stick(float offset);

    /** @brief Czy piłka jest przyklejona do paletki. */
    bool IsStuck() const { return stuck_; }

    /** @brief Wystrzeliwuje przyklejoną piłkę - kąt zależy od miejsca na paletce. */
    void Launch();

    /** @brief Usuwa piłkę z gry (strefa śmierci, koniec poziomu). */
    void Lose();

  protected:
    /** @brief Zapisuje się w Round i tworzy ślad w polu gry. */
    void OnMount() override;

    /** @brief Ślad piłki (łamana z ostatnich położeń). */
    void Tick(float dt, const Input& input) override;

    /** @brief Stała prędkość, minimalny kąt pionowy, pole spowolnienia, przyklejenie. */
    void FixedTick(float step, const Input& input) override;

  private:
    /** @brief Reakcja na zderzenie: paletka, cegła, siatka albo ściana. */
    void Bounced(const node::physics::Collision& hit);

    /** @brief Kierunek odbicia od paletki dla przesunięcia od jej środka. */
    Vector2 PaddleDirection(float offset) const;

    Round* round_ = nullptr;                  /**< Stan poziomu (kontekst sceny). */
    node::shapes::Polyline* trail_ = nullptr; /**< Ślad (osobny obiekt pola gry). */
    std::deque<Point2D> history_;             /**< Ostatnie położenia do śladu. */
    bool stuck_ = false;                      /**< Przyklejona do paletki. */
    float stickOffset_ = 0.0f;                /**< Przesunięcie od środka paletki. */
    float wallCooldown_ = 0.0f;               /**< Odstęp dźwięków ściany. */
};

/**
 * @brief Tworzy piłkę w polu gry.
 * @param round Stan poziomu (pole gry, lista piłek).
 * @param position Położenie środka.
 * @param velocity Prędkość początkowa (zero - piłka przyklejona do paletki).
 * @return Nowa piłka.
 */
Ball& SpawnBall(Round& round, Vector2 position, Vector2 velocity);
