/**
 * @file Hud.hpp
 * @brief HUD gry: panel z wynikiem, życiami, serią i mocami oraz duży napis na środku pola.
 * @details Wszystkie elementy są obiektami ekranu (Entity::ScreenSpace()) - kamera ich nie
 * przesuwa, więc nie trzęsą się razem z polem gry przy wstrząsie.
 */

#pragma once

#include <raycade>

struct Round;
struct Session;

/**
 * @brief Tworzy panel HUD po prawej stronie pola.
 * @param scene Scena gry.
 * @param round Stan poziomu.
 * @param session Stan gry.
 */
void SpawnHud(Scene& scene, Round& round, Session& session);

/**
 * @brief Tworzy duży napis na środku pola (Round::message) - pulsuje, gdy jest widoczny.
 * @param scene Scena gry.
 * @param round Stan poziomu.
 */
void SpawnMessage(Scene& scene, Round& round);

/**
 * @brief Pokazuje napis na środku pola (pusty tekst - ukrywa).
 * @param round Stan poziomu.
 * @param text Treść (może mieć kilka wierszy).
 */
void ShowMessage(Round& round, const std::string& text);
