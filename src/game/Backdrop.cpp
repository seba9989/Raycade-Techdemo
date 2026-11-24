/**
 * @file Backdrop.cpp
 * @brief Tło: niebo z gradientem, mgławica (scanline), góry (boundary fill), planeta (boundary
 * fill) z kraterami (flood fill) i pierścieniem (elipsa punktu środkowego), gwiazdy, gwiazdozbiór
 * (odcinek przyrostowy) i pas kafli skopiowanych z arkusza (Blit).
 */

#include "game/Backdrop.hpp"

#include "game/Layout.hpp"

#include <array>
#include <cmath>
#include <random>
#include <vector>

namespace {

/** @brief Kolory jednego układu tła. */
struct Scheme {
    Color top;    /**< Niebo u góry. */
    Color bottom; /**< Niebo u dołu. */
    Color nebula; /**< Mgławica. */
    Color hills;  /**< Góry. */
    Color planet; /**< Planeta. */
    Color ring;   /**< Pierścień planety. */
};

constexpr std::array<Scheme, 4> kSchemes{{
    {{14, 12, 34, 255},
     {40, 24, 66, 255},
     {60, 34, 90, 255},
     {24, 18, 44, 255},
     {200, 110, 90, 255},
     {240, 200, 140, 255}},
    {{8, 18, 30, 255},
     {20, 48, 70, 255},
     {24, 70, 90, 255},
     {12, 30, 44, 255},
     {90, 160, 210, 255},
     {180, 230, 255, 255}},
    {{26, 8, 18, 255},
     {70, 22, 30, 255},
     {110, 40, 40, 255},
     {40, 12, 20, 255},
     {230, 170, 60, 255},
     {255, 220, 150, 255}},
    {{10, 22, 14, 255},
     {28, 58, 36, 255},
     {40, 90, 50, 255},
     {14, 34, 20, 255},
     {150, 210, 120, 255},
     {220, 255, 190, 255}},
}};

/** @brief Kolor obrysu planety - unikalny, bo zatrzymuje boundary fill. */
constexpr Color kPlanetEdge{255, 255, 254, 255};

} // namespace

node::visual::Canvas& SpawnBackdrop(Entity& parent, int variant) {
    const Scheme& scheme = kSchemes[static_cast<std::size_t>(variant) % kSchemes.size()];
    const float width = layout::kField.width;
    const float height = layout::kField.height;
    // prywatny generator: tło zależy tylko od numeru układu, a nie od losowań rozgrywki
    std::minstd_rand random(static_cast<unsigned>(variant) * 7919u + 17u);
    const auto uniform = [&random](float min, float max) {
        return std::uniform_real_distribution<float>(min, max)(random);
    };

    // fragment arkusza do kafli: pierwsza klatka animacji "metal"
    const auto sheet = Engine::Get().Assets().Sheet("sprites/arcade.json");
    Rectangle tile{};
    if (const SpriteAnimation* metal = sheet->FindAnimation("metal"))
        tile = sheet->Frame(metal->SheetFrame(0));

    return parent.Spawn<node::visual::Canvas>()
        .Position({layout::kField.x, layout::kField.y})
        .Size({width, height})
        .Background(scheme.top)
        .DrawLayer(-10)
        .Edit([&](PrimitiveRenderer& r) {
            // niebo: poziome paski z gradientem
            for (int y = 0; y < static_cast<int>(height); y += 4) {
                const float t = static_cast<float>(y) / height;
                const Color c{
                    static_cast<unsigned char>(scheme.top.r + (scheme.bottom.r - scheme.top.r) * t),
                    static_cast<unsigned char>(scheme.top.g + (scheme.bottom.g - scheme.top.g) * t),
                    static_cast<unsigned char>(scheme.top.b + (scheme.bottom.b - scheme.top.b) * t),
                    255
                };
                r.Paint(c).Filled().Rect({0, static_cast<float>(y)}, {width, 4});
            }

            // mgławica: wklęsły wielokąt wypełniony algorytmem scanline (kontrola parzystości)
            std::vector<Point2D> cloud;
            const Vector2 middle{uniform(180, 460), uniform(220, 360)};
            for (int i = 0; i < 14; ++i) {
                const float angle = static_cast<float>(i) / 14.0f * 2.0f * PI;
                const float radius = (i % 2 ? 90.0f : 150.0f) * uniform(0.7f, 1.2f);
                cloud.push_back(
                    {middle.x + std::cos(angle) * radius * 1.4f,
                     middle.y + std::sin(angle) * radius * 0.6f}
                );
            }
            r.Raster(Rasterizer::Custom).Paint(scheme.nebula).Filled().Polygon(cloud);

            // góry: łamana odcinków (algorytm przyrostowy) zamyka obszar z brzegami płótna;
            // boundary fill maluje go aż do granicy w kolorze gór (niebo pod spodem ma paski
            // gradientu, więc flood fill zatrzymałby się na pierwszym z nich)
            std::vector<Point2D> ridge;
            for (float x = 0; x <= width; x += 40.0f)
                ridge.push_back({x, height - 120.0f - uniform(0, 90)});
            r.Filled(false).Paint(scheme.hills);
            for (std::size_t i = 1; i < ridge.size(); ++i)
                r.Line(ridge[i - 1], ridge[i]);
            r.BoundaryFill({width / 2, height - 4}, scheme.hills);

            // planeta: okrąg punktu środkowego w kolorze granicy i boundary fill wnętrza -
            // wnętrze ma potem jeden kolor
            const Vector2 planet{uniform(120, 520), uniform(380, 470)};
            const float radius = uniform(48, 70);
            r.Raster(Rasterizer::Midpoint).Paint(kPlanetEdge).Circle(planet, radius);
            r.Paint(scheme.planet).BoundaryFill(planet, kPlanetEdge);
            // kratery: okręgi w ciemniejszym kolorze wewnątrz planety, a ich wnętrze (jeden kolor
            // planety) zamalowuje flood fill - wypełnienie zatrzymuje się na obrysie krateru
            const Color dark{
                static_cast<unsigned char>(scheme.planet.r * 3 / 5),
                static_cast<unsigned char>(scheme.planet.g * 3 / 5),
                static_cast<unsigned char>(scheme.planet.b * 3 / 5), 255
            };
            for (const Vector3 crater :
                 {Vector3{-0.35f, -0.25f, 0.22f}, Vector3{0.3f, 0.2f, 0.16f},
                  Vector3{-0.05f, 0.45f, 0.12f}}) {
                const Vector2 center{planet.x + crater.x * radius, planet.y + crater.y * radius};
                r.Paint(dark).Circle(center, crater.z * radius);
                r.FloodFill(center);
            }
            r.Paint(scheme.planet).Circle(planet, radius); // obrys w kolorze planety
            // pierścień: elipsa punktu środkowego
            r.Paint(scheme.ring).Ellipse(planet, {radius * 1.8f, radius * 0.35f});
            r.Ellipse(planet, {radius * 1.65f, radius * 0.28f});

            // gwiazdy: tablica punktów jednym wywołaniem
            std::vector<Point2D> stars;
            for (int i = 0; i < 160; ++i)
                stars.push_back({uniform(0, width), uniform(0, height - 200)});
            r.Paint(Color{220, 220, 255, 255}).Points(stars);

            // gwiazdozbiór: jasne gwiazdy połączone odcinkami (algorytm przyrostowy, Custom)
            std::vector<Point2D> constellation;
            Vector2 star{uniform(80, 200), uniform(90, 160)};
            for (int i = 0; i < 6; ++i) {
                constellation.push_back(star);
                star = {star.x + uniform(40, 80), star.y + uniform(-40, 40)};
            }
            r.Raster(Rasterizer::Custom).Paint(Color{160, 170, 230, 255}).Polyline(constellation);
            for (const Point2D& bright : constellation)
                r.Paint(WHITE).Filled().Circle(bright, 2).Filled(false);

            // pas kafli pod sufitem: fragment bitmapy arkusza skopiowany na płótno (Blit)
            if (tile.width > 0)
                for (float x = 0; x < width; x += tile.width)
                    r.Blit(*sheet->Bitmap(), tile, {x, 0}, Color{90, 92, 120, 255});
            r.Reset();
        });
}
