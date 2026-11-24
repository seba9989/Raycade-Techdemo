/**
 * @file Session.cpp
 * @brief Zapis i odczyt ustawień oraz tabeli rekordów (Engine::LoadText()/SaveText()).
 * @details Format pliku to wiersze `klucz=wartość`; wpis rekordu ma postać
 * `wynik=imie;punkty;poziom`. Nieznane klucze są pomijane, więc starszy plik wczytuje się bez
 * błędów.
 */

#include "game/Session.hpp"

#include <raycade>

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <sstream>

namespace {

/** @brief Ścieżka pliku zapisu obok programu. */
std::string SavePath(const Engine& engine) {
    return (std::filesystem::path(engine.ApplicationDirectory()) / Session::kFile).string();
}

/** @brief Liczba całkowita z tekstu (fallback przy błędzie). */
int ToInt(std::string_view text, int fallback) {
    int value = fallback;
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

} // namespace

void Session::NewGame() {
    level = 0;
    score = 0;
    lives = kLives;
    pending.reset();
    victory = false;
}

std::string Session::LevelPath() const {
    if (!customLevel.empty()) return customLevel;
    if (levels.empty()) return {};
    return levels[static_cast<std::size_t>(
        std::clamp(level, 0, static_cast<int>(levels.size()) - 1)
    )];
}

bool Session::IsLastLevel() const {
    return !customLevel.empty() || level + 1 >= static_cast<int>(levels.size());
}

int Session::Best() const {
    return scores.empty() ? 0 : scores.front().score;
}

bool Session::Qualifies(int value) const {
    return value > 0 && (scores.size() < kMaxScores || value > scores.back().score);
}

void Session::AddScore(ScoreEntry entry) {
    scores.push_back(std::move(entry));
    std::stable_sort(scores.begin(), scores.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
        return a.score > b.score;
    });
    if (scores.size() > kMaxScores) scores.resize(kMaxScores);
}

void Session::Load(const Engine& engine) {
    const auto text = engine.LoadText(SavePath(engine));
    if (!text) return; // pierwsze uruchomienie - wartości domyślne
    std::istringstream lines(*text);
    std::string line;
    scores.clear();
    while (std::getline(lines, line)) {
        const auto split = line.find('=');
        if (split == std::string::npos) continue;
        const std::string key = line.substr(0, split);
        const std::string value = line.substr(split + 1);
        if (key == "glosnosc")
            volume = std::clamp(ToInt(value, 80), 0, 100) / 100.0f;
        else if (key == "kineskop")
            crt = value == "1";
        else if (key == "pelny_ekran")
            fullscreen = value == "1";
        else if (key == "gracz" && !value.empty())
            player = value;
        else if (key == "wynik") {
            // imie;punkty;poziom - imię może zawierać spacje, ale nie średnik
            const auto a = value.find(';');
            const auto b = value.find(';', a == std::string::npos ? a : a + 1);
            if (a == std::string::npos || b == std::string::npos) continue;
            AddScore(
                {value.substr(0, a), ToInt(value.substr(a + 1, b - a - 1), 0),
                 ToInt(value.substr(b + 1), 1)}
            );
        }
    }
}

void Session::Save(const Engine& engine) const {
    std::string text = std::format(
        "glosnosc={}\nkineskop={}\npelny_ekran={}\ngracz={}\n",
        static_cast<int>(volume * 100.0f + 0.5f), crt ? 1 : 0, fullscreen ? 1 : 0, player
    );
    for (const ScoreEntry& entry : scores)
        text += std::format("wynik={};{};{}\n", entry.name, entry.score, entry.level);
    engine.SaveText(SavePath(engine), text);
}

void Session::Apply(Engine& engine) const {
    engine.Audio().Volume(volume);
    // shader z Assets jest wspólny - kolejne pobranie zwraca ten sam obiekt
    engine.PostShader(crt ? engine.Assets().Shader("shaders/crt.fs") : nullptr);
    if (engine.IsFullscreen() != fullscreen) engine.Fullscreen(fullscreen);
}
