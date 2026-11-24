/**
 * @file Settings.hpp
 * @brief Panel ustawień (kineskop, pełny ekran, głośność) - wspólny dla sceny "ustawienia"
 * i menu pauzy w grze.
 */

#pragma once

#include <raycade>

#include <functional>

/**
 * @brief Tworzy panel ustawień: trzy przyciski i „Powrót”, z nawigacją strzałkami.
 * @details Każda zmiana od razu działa (Session::Apply(): szyna główna miksera,
 * Engine::PostShader(), Engine::Fullscreen()) i trafia do pliku zapisu. Głośność zmieniają
 * strzałki w lewo/prawo (KeyPressedRepeat()), kółko myszy nad przyciskiem (OnScroll()) albo
 * kolejne naciśnięcia. Napisy przycisków idą za stanem, więc zgadzają się także po F2 i F11.
 * Panel znika razem z rodzicem albo po Destroy() zwróconej grupy.
 * @param parent Rodzic panelu.
 * @param position Lewy górny róg pierwszego przycisku (w układzie rodzica).
 * @param back Działanie przycisku „Powrót”.
 * @return Grupa panelu (pierwszy przycisk ma już fokus).
 */
node::Group& SpawnSettings(Entity& parent, Vector2 position, std::function<void()> back);
