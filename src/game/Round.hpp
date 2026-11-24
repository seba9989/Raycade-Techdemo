/**
 * @file Round.hpp
 * @brief Stan bieżącego poziomu - kontekst sceny (Scene::ProvideContext()).
 * @details Round powstaje przy każdym wczytaniu sceny "gra" i znika razem z nią
 * (ReloadScene() zaczyna poziom od nowa), w odróżnieniu od Session, która jest kontekstem
 * silnika. Obiekty pola (piłki, cegły, kapsuły) znajdują go przez `Context<Round>()`
 * w OnMount().
 */

#pragma once

#include <raycade>

#include <string>
#include <vector>

class Ball;
class Player;

/** @brief Etap rozgrywki na poziomie. */
enum class Phase {
    Ready,    /**< Piłka czeka na paletce na wystrzał. */
    Playing,  /**< Gra trwa. */
    Cleared,  /**< Poziom ukończony - za chwilę następny. */
    Lost,     /**< Stracona ostatnia piłka - za chwilę nowa albo koniec gry. */
    GameOver, /**< Koniec gry - za chwilę tabela rekordów. */
};

/** @brief Stan poziomu: cegły, seria, moce, efekty ekranu. */
struct Round {
    Phase phase = Phase::Ready; /**< Etap rozgrywki. */
    std::string name;           /**< Nazwa poziomu z pliku. */
    int bricksLeft = 0;         /**< Cegły do zbicia (bez metalu). */
    std::vector<Ball*> balls;   /**< Piłki w grze (dopisują się w OnMount(), znikają w Lose()). */
    int combo = 0;              /**< Cegły zbite od ostatniego odbicia od paletki. */
    float speed = 360.0f;       /**< Bieżąca prędkość piłek (rośnie z czasem). */
    bool paused = false;        /**< Pauza (krok stały wyłączony). */

    // ---------------- moce: pozostały czas w sekundach (0 - nieaktywna)
    float wide = 0.0f;  /**< Szersza paletka. */
    float laser = 0.0f; /**< Działka laserowe. */
    float glue = 0.0f;  /**< Klej - piłka przykleja się do paletki. */
    float slow = 0.0f;  /**< Pole spowolnienia. */
    float net = 0.0f;   /**< Siatka bezpieczeństwa. */

    // ---------------- efekty ekranu
    float shake = 0.0f;     /**< Siła wstrząsu kamery (maleje z czasem). */
    Vector2 wave{};         /**< Środek fali uderzeniowej (układ pola). */
    float waveTime = -1.0f; /**< Czas od wybuchu (< 0 - brak fali). */

    // ---------------- obiekty sceny (ważne przez całe życie sceny)
    node::visual::RenderGroup* field = nullptr; /**< Pole gry - tu trafiają obiekty i efekty. */
    Player* player = nullptr;                   /**< Paletka. */
    node::physics::Area* slowField = nullptr;   /**< Pole spowolnienia (wyłączone: ukryte). */
    node::collider::Rect* netShape = nullptr;   /**< Collider siatki (Disabled(), gdy brak). */
    node::visual::Text* message = nullptr;      /**< Duży napis na środku pola. */
};
