/**
 * @file Session.hpp
 * @brief Stan gry trwający między scenami - kontekst silnika (Engine::ProvideContext()).
 * @details Session żyje przez cały czas działania programu: wynik i życia przechodzą z poziomu na
 * poziom (ReloadScene("gra")), a ustawienia i tabela rekordów są zapisywane do pliku tekstowego
 * obok programu (Engine::ApplicationDirectory(), SaveText()/LoadText()). Każdy obiekt sceny
 * odczytuje ją przez `Context<Session>()`, a kod scen - przez `engine.Context<Session>()`.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

class Engine;

/** @brief Wpis tabeli rekordów. */
struct ScoreEntry {
    std::string name; /**< Imię gracza. */
    int score = 0;    /**< Wynik. */
    int level = 0;    /**< Osiągnięty poziom (od 1). */
};

/** @brief Stan gry i ustawienia - kontekst silnika. */
struct Session {
    static constexpr int kLives = 3;                     /**< Życia na start. */
    static constexpr std::size_t kMaxScores = 10;        /**< Długość tabeli rekordów. */
    static constexpr const char* kFile = "rykoszet.txt"; /**< Plik zapisu. */

    // ---------------- ustawienia (zapisywane)
    float volume = 0.8f;            /**< Głośność główna (szyna główna miksera). */
    bool crt = false;               /**< Efekt kineskopu (Engine::PostShader()). */
    bool fullscreen = false;        /**< Pełny ekran. */
    std::string player = "Gracz";   /**< Imię do tabeli rekordów. */
    std::vector<ScoreEntry> scores; /**< Tabela rekordów (malejąco). */

    // ---------------- rozgrywka
    std::vector<std::string> levels; /**< Pełne ścieżki plików poziomów (assets/levels). */
    std::string customLevel;         /**< Poziom z upuszczonego pliku ("" - brak). */
    int level = 0;                   /**< Indeks bieżącego poziomu w levels. */
    int score = 0;                   /**< Wynik. */
    int lives = kLives;              /**< Pozostałe życia (bez bieżącej piłki). */
    std::optional<int> pending;      /**< Wynik czekający na wpis do tabeli (po grze). */
    bool victory = false;            /**< Czy ostatnia gra skończyła się przejściem poziomów. */
    bool autoplay = false;           /**< Paletka sama goni piłkę (GRA_AUTO, menu tytułowe). */
    bool showFps = false;            /**< Licznik FPS w HUD-zie (F3). */

    /** @brief Zaczyna nową grę od pierwszego poziomu (albo od poziomu z pliku). */
    void NewGame();

    /** @brief Ścieżka pliku bieżącego poziomu. */
    std::string LevelPath() const;

    /** @brief Czy bieżący poziom jest ostatni (poziom z pliku jest zawsze jedyny). */
    bool IsLastLevel() const;

    /** @brief Najlepszy wynik z tabeli (0, gdy jest pusta). */
    int Best() const;

    /** @brief Czy wynik mieści się w tabeli rekordów. */
    bool Qualifies(int value) const;

    /** @brief Dopisuje wynik do tabeli (sortuje i przycina do kMaxScores). */
    void AddScore(ScoreEntry entry);

    /** @brief Wczytuje ustawienia i rekordy z pliku (brak pliku - wartości domyślne). */
    void Load(const Engine& engine);

    /** @brief Zapisuje ustawienia i rekordy do pliku. */
    void Save(const Engine& engine) const;

    /**
     * @brief Stosuje ustawienia w silniku: głośność szyny głównej, shader kineskopu
     * (Engine::PostShader()) i pełny ekran.
     */
    void Apply(Engine& engine) const;
};
