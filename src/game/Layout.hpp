/**
 * @file Layout.hpp
 * @brief Układ ekranu i pola gry oraz warstwy kolizji.
 * @details Wszystkie wymiary są w jednostkach gry (Engine::ViewSize()), niezależnie od rozmiaru
 * okna. Warstwy to maski bitowe dla CollisionObject::CollisionLayer() i CollisionMask():
 * obiekt A zderza się z B, gdy `A.CollisionMask() & B.CollisionLayer()` jest niezerowe (i na
 * odwrót - Box2D sprawdza obie strony), więc pary podaje się symetrycznie.
 */

#pragma once

#include <raylib.h>

#include <cstdint>

/** @brief Wymiary ekranu, pola gry i siatki cegieł. */
namespace layout {

constexpr int kViewWidth = 960;             /**< Szerokość układu gry. */
constexpr int kViewHeight = 720;            /**< Wysokość układu gry. */
constexpr float kFixedStep = 1.0f / 120.0f; /**< Krok stały fizyki (Engine::FixedStep()). */

constexpr float kWall = 20.0f;                      /**< Grubość ścian. */
constexpr Rectangle kField{40, 40, 640, 680};       /**< Wnętrze pola (dół otwarty). */
constexpr Vector2 kBrick{48, 24};                   /**< Rozmiar cegły. */
constexpr Vector2 kGrid{48, 80};                    /**< Lewy górny róg siatki cegieł. */
constexpr int kColumns = 13;                        /**< Kolumny siatki poziomu. */
constexpr int kRows = 14;                           /**< Najwięcej wierszy siatki. */
constexpr float kPaddleY = 664.0f;                  /**< Środek paletki w pionie. */
constexpr Vector2 kPaddle{104, 20};                 /**< Rozmiar paletki (zwykłej). */
constexpr float kBallRadius = 8.0f;                 /**< Promień piłki. */
constexpr float kPanelX = 712.0f;                   /**< Lewa krawędź panelu HUD. */
constexpr Vector2 kFieldCenter{360, 380};           /**< Środek pola (napisy). */
constexpr float kBottom = kField.y + kField.height; /**< Dolna krawędź pola. */
constexpr float kRight = kField.x + kField.width;   /**< Prawa krawędź pola. */

} // namespace layout

/** @brief Warstwy kolizji (maski bitowe). */
enum Layer : std::uint32_t {
    kWalls = 1u << 0,   /**< Ściany pola. */
    kBricks = 1u << 1,  /**< Cegły. */
    kPaddle = 1u << 2,  /**< Paletka gracza. */
    kBall = 1u << 3,    /**< Piłki. */
    kCapsule = 1u << 4, /**< Kapsuły mocy (obszary). */
    kDebris = 1u << 5,  /**< Odłamki cegieł. */
    kNet = 1u << 6,     /**< Siatka bezpieczeństwa. */
    kZone = 1u << 7,    /**< Obszary: strefa śmierci, pole spowolnienia. */
};
