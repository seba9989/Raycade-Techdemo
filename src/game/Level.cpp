/**
 * @file Level.cpp
 * @brief Rozbiór pliku poziomu i budowa pola: StaticBody (ściany, cegły), Area (strefy), OneWay.
 */

#include "game/Level.hpp"

#include "game/Ball.hpp"
#include "game/Bricks.hpp"
#include "game/Layout.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>

namespace {

/** @brief Usuwa białe znaki z końca wiersza (pliki z Windows mają "\r"). */
std::string TrimRight(std::string text) {
    while (!text.empty() && (text.back() == '\r' || text.back() == ' ' || text.back() == '\t'))
        text.pop_back();
    return text;
}

/** @brief Ściany pola: jedno ciało z trzema widocznymi colliderami (lewa, prawa, sufit). */
void SpawnWalls(Entity& field) {
    using layout::kField;
    using layout::kWall;
    auto& walls = field.Spawn<node::physics::StaticBody>().CollisionLayer(kWalls).CollisionMask(
        kBall | kPaddle | kDebris
    );
    const Color stone{58, 62, 92, 255};
    const Rectangle parts[] = {
        {kField.x - kWall, kField.y - kWall, kWall, kField.height + kWall},
        {layout::kRight, kField.y - kWall, kWall, kField.height + kWall},
        {kField.x - kWall, kField.y - kWall, kField.width + 2 * kWall, kWall},
    };
    for (const Rectangle& part : parts) {
        walls.Spawn<node::collider::Rect>()
            .Position({part.x, part.y})
            .Size({part.width, part.height})
            .Paint(stone)
            .Filled()
            .Visible(); // collider jest też grafiką
        // jaśniejsza krawędź od strony pola
        walls.Spawn<node::shapes::Rect>()
            .Position({part.x + 2, part.y + 2})
            .Size({part.width - 4, part.height - 4})
            .Paint(Color{96, 104, 150, 255});
    }
    // nity na ścianach co 40 px
    for (float y = kField.y; y < layout::kBottom; y += 40.0f)
        for (const float x : {kField.x - kWall / 2, layout::kRight + kWall / 2})
            walls.Spawn<node::shapes::Circle>()
                .Position({x, y})
                .Radius(3)
                .Paint(Color{120, 128, 175, 255})
                .Filled();
}

/** @brief Cegła z siatki poziomu. */
void SpawnBrick(Entity& wall, Round& round, char cell, int column, int row) {
    const Vector2 center{
        layout::kGrid.x + (static_cast<float>(column) + 0.5f) * layout::kBrick.x,
        layout::kGrid.y + (static_cast<float>(row) + 0.5f) * layout::kBrick.y
    };
    Brick& brick = wall.Spawn<Brick>().Position(center);
    switch (cell) {
    case 'H':
        brick.Kind(BrickKind::Hard);
        break;
    case 'M':
        brick.Kind(BrickKind::Metal);
        break;
    case 'X':
        brick.Kind(BrickKind::Explosive);
        break;
    case '<':
        brick.Kind(BrickKind::Moving, (column + row) % 6, -1.0f);
        break;
    case '>':
        brick.Kind(BrickKind::Moving, (column + row) % 6, 1.0f);
        break;
    case '?':
        brick.Kind(BrickKind::Hidden);
        break;
    default:
        brick.Kind(BrickKind::Normal, cell - '1');
        break;
    }
    if (brick.IsCounted()) ++round.bricksLeft;
}

} // namespace

std::optional<LevelData> ParseLevel(const std::string& text) {
    LevelData level;
    std::istringstream lines(text);
    std::string line;
    bool hasBricks = false;
    while (std::getline(lines, line)) {
        line = TrimRight(line);
        if (line.empty() || line.front() == '#') continue;
        if (line.starts_with("nazwa=")) {
            level.name = line.substr(6);
            continue;
        }
        if (line.starts_with("tlo=")) {
            level.backdrop = std::clamp(line[4] - '0', 0, 3);
            continue;
        }
        if (static_cast<int>(level.rows.size()) >= layout::kRows) continue;
        line.resize(layout::kColumns, '.');
        for (const char cell : line)
            if ((cell >= '1' && cell <= '6') || std::string_view("HX<>?").contains(cell))
                hasBricks = true;
        level.rows.push_back(line);
    }
    if (!hasBricks) return std::nullopt;
    return level;
}

std::optional<LevelData> LoadLevel(Engine& engine, const std::string& path) {
    const auto text = engine.LoadText(path);
    if (!text) {
        engine.Log(LogLevel::Error, "nie mozna wczytac poziomu: " + path);
        return std::nullopt;
    }
    auto level = ParseLevel(*text);
    if (!level) engine.Log(LogLevel::Warning, "poziom bez cegiel do zbicia: " + path);
    return level;
}

void BuildLevel(Round& round, const LevelData& level) {
    Entity& field = *round.field;
    round.name = level.name;
    SpawnWalls(field);

    // cegły w jednej grupie: grupa udostępnia im kolory (kontekst poddrzewa)
    auto& wall = field.Spawn<node::Group>().ProvideContext<Palette>();
    for (int row = 0; row < static_cast<int>(level.rows.size()); ++row)
        for (int column = 0; column < layout::kColumns; ++column) {
            const char cell =
                level.rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
            if (cell != '.' && cell != ' ') SpawnBrick(wall, round, cell, column, row);
        }

    // strefa śmierci pod polem: obszar wykrywa piłki (maska kBall)
    field.Spawn<node::physics::Area>()
        .Position({layout::kField.x - layout::kWall, layout::kViewHeight + 12.0f})
        .CollisionLayer(kZone)
        .CollisionMask(kBall)
        .Children([](node::physics::Area& zone) {
            zone.Spawn<node::collider::Rect>().Size({layout::kField.width + 2 * layout::kWall, 60});
        })
        .OnBodyEnter([](node::physics::Area&, node::physics::PhysicsBody& body) {
            if (auto* ball = dynamic_cast<Ball*>(&body)) ball->Lose();
        });

    // pole spowolnienia (moc "wolniej"): półprzezroczysty pas, ukryty, dopóki moc nie działa
    round.slowField = &field.Spawn<node::physics::Area>()
                           .Position({layout::kField.x, 430})
                           .CollisionLayer(kZone)
                           .CollisionMask(kBall)
                           .Visible(false);
    round.slowField->Spawn<node::collider::Rect>()
        .Size({layout::kField.width, 120})
        .Paint(Fade(Color{240, 150, 200, 255}, 0.16f))
        .Filled()
        .Visible();

    // siatka (moc "siatka"): platforma jednostronna - odbija piłkę z góry, przepuszcza od dołu
    auto& net = field.Spawn<node::physics::StaticBody>()
                    .Position({layout::kField.x, layout::kPaddleY + 26})
                    .CollisionLayer(kNet)
                    .CollisionMask(kBall);
    round.netShape = &net.Spawn<node::collider::Rect>()
                          .Size({layout::kField.width, 4})
                          .OneWay()
                          .Disabled() // wyłączony collider: świat fizyki go pomija
                          .Paint(Color{90, 220, 255, 255})
                          .Filled();

    // paletka i pierwsza piłka (przyklejona)
    Player& player = field.Spawn<Player>().Position(
        {layout::kField.x + layout::kField.width / 2, layout::kPaddleY}
    );
    SpawnBall(
        round,
        {player.Position().x, layout::kPaddleY - layout::kPaddle.y / 2 - layout::kBallRadius - 1},
        {0, 0}
    );
}
