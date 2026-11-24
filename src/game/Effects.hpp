/**
 * @file Effects.hpp
 * @brief Efekty wizualne: odłamki, iskry, napisy punktów, wybuch, wstrząs i fala uderzeniowa.
 * @details Efekty są obiektami sceny, które same się usuwają (Entity::Destroy()) - gra tylko je
 * tworzy. Rodzicem jest zwykle pole gry (Round::field), żeby obejmowała je fala shadera.
 */

#pragma once

#include <raycade>

struct Round;

/** @brief Funkcje tworzące efekty. */
namespace fx {

/**
 * @brief Odłamki cegły: małe bryły sztywne z grawitacją, które spadają i odbijają się od ścian
 * i paletki (ApplyImpulse() poza środkiem masy - obracają się).
 * @param parent Rodzic (pole gry).
 * @param center Środek cegły.
 * @param color Kolor odłamków.
 * @param count Liczba odłamków.
 */
void Debris(Entity& parent, Vector2 center, Color color, int count = 6);

/**
 * @brief Iskry: koła rysowane addytywnie (Blend(BLEND_ADDITIVE)), rozlatujące się i gasnące.
 * @param parent Rodzic.
 * @param center Środek.
 * @param color Kolor.
 * @param count Liczba iskier.
 */
void Sparks(Entity& parent, Vector2 center, Color color, int count = 10);

/**
 * @brief Napis z punktami unoszący się nad miejscem trafienia (rośnie, kołysze się i blednie).
 * @param parent Rodzic.
 * @param at Miejsce napisu.
 * @param points Punkty.
 * @param combo Mnożnik serii (pokazywany od 2).
 */
void ScorePopup(Entity& parent, Vector2 at, int points, int combo);

/**
 * @brief Wybuch: sprite z arkusza-siatki (SpriteSheet::Grid) odtwarzany raz; zdarzenie klatki
 * "fala" uruchamia falę uderzeniową, a OnFinish() usuwa sprite.
 * @param parent Rodzic.
 * @param center Środek wybuchu.
 */
void Explosion(Entity& parent, Vector2 center);

/** @brief Wstrząs kamery o podanej sile (piksele). */
void Shake(Round& round, float strength);

/** @brief Fala uderzeniowa shadera pola gry ze środkiem w podanym punkcie. */
void Shockwave(Round& round, Vector2 center);

/**
 * @brief Gra dźwięk efektu z wysokością zależną od serii (Voice::Pitch()).
 * @param name Nazwa dźwięku w mikserze.
 * @param pitch Wysokość (1 - bez zmiany).
 */
void Play(const std::string& name, float pitch = 1.0f);

/**
 * @brief Włącza ścieżkę muzyki z przenikaniem (szyna muzyki z Exclusive() wycisza poprzednią);
 * ta sama, już grająca ścieżka gra dalej.
 * @param name Plik muzyki względem assets/.
 */
void Music(const std::string& name);

/** @brief Wstrzymuje albo wznawia bieżącą ścieżkę (Music::Pause()/Resume()). */
void PauseMusic(bool value);

/** @brief Zatrzymuje muzykę (z wyciszeniem). */
void StopMusic();

} // namespace fx
