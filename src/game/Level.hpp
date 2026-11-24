/**
 * @file Level.hpp
 * @brief Plik poziomu: rozbiór siatki znaków i budowa sceny (ściany, cegły, obszary, paletka).
 * @details Format opisuje `assets/levels/LEGENDA.txt`. Poziom wczytuje Engine::LoadText() -
 * z katalogu assets/levels albo z pliku upuszczonego na okno (Engine::OnFilesDropped()).
 */

#pragma once

#include <raycade>

#include <optional>
#include <string>
#include <vector>

struct Round;

/** @brief Opis poziomu z pliku. */
struct LevelData {
    std::string name = "Bez nazwy"; /**< Nazwa (klucz "nazwa="). */
    int backdrop = 0;               /**< Układ tła 0-3 (klucz "tlo="). */
    std::vector<std::string> rows;  /**< Wiersze siatki (najwyżej layout::kRows). */
};

/**
 * @brief Rozbiera tekst poziomu.
 * @param text Zawartość pliku.
 * @return Poziom albo std::nullopt, gdy nie ma w nim żadnej cegły do zbicia.
 */
std::optional<LevelData> ParseLevel(const std::string& text);

/**
 * @brief Wczytuje i rozbiera plik poziomu.
 * @param engine Silnik (LoadText(), log).
 * @param path Ścieżka pliku.
 * @return Poziom albo std::nullopt (błąd zapisany w logu).
 */
std::optional<LevelData> LoadLevel(Engine& engine, const std::string& path);

/**
 * @brief Buduje poziom w polu gry (Round::field): ściany, cegły w grupie z kontekstem Palette,
 * strefę śmierci, pole spowolnienia, siatkę i paletkę.
 * @param round Stan poziomu (uzupełnia bricksLeft, player, slowField, netShape).
 * @param level Opis poziomu.
 */
void BuildLevel(Round& round, const LevelData& level);
