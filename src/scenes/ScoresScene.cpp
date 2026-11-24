/**
 * @file ScoresScene.cpp
 * @brief Scena "wyniki": tabela rekordów w przewijanym panelu i wpis nowego rekordu.
 * @details
 * - panel z ClipRect(): wiersze poza prostokątem nie są rysowane ani klikalne; kółko myszy nad
 *   panelem (OnScroll()) i strzałki przewijają listę (Translate() grupy wierszy),
 * - wynik z zakończonej gry (Session::pending) wpisuje się imieniem w polu tekstowym
 *   (node::ui::TextField, OnSubmit()) i trafia do pliku zapisu (Engine::SaveText()),
 * - przyciski z fokusem: zagraj ponownie, menu.
 */

#include <raycade>

#include "game/Effects.hpp"
#include "game/Layout.hpp"
#include "game/Session.hpp"
#include "game/Ui.hpp"

#include <algorithm>
#include <format>
#include <memory>
#include <vector>

namespace {

constexpr Rectangle kPanel{230, 236, 500, 264}; /**< Widoczny obszar listy. */
constexpr float kRow = 44.0f;                   /**< Wysokość wiersza. */

} // namespace

/** @brief Konfiguruje tabelę rekordów. */
SCENE("wyniki") {
    Session& session = *engine.Context<Session>();
    session.autoplay = false;
    engine.CursorLocked(false); // kursor z gry wraca do menu
    engine.CursorHidden(false);
    // wyjście z pauzy gry przez menu: krok stały i efekty wracają tutaj
    engine.FixedStep(layout::kFixedStep);
    engine.Audio().Bus("efekty").Paused(false);
    fx::Music("music/menu.wav");

    scene.Spawn<node::shapes::Rect>()
        .Size({static_cast<float>(layout::kViewWidth), static_cast<float>(layout::kViewHeight)})
        .Paint(Color{14, 14, 30, 255})
        .Filled()
        .DrawLayer(-10);
    ShadowedText(scene)
        .Content("REKORDY")
        .FontSize(56)
        .Paint(GOLD)
        .Anchor({0.5f, 0.5f})
        .Position({480, 70});

    // nagłówek zależny od tego, skąd przyszedł gracz
    auto highlight = std::make_shared<int>(-1); // wiersz nowego rekordu
    auto& status =
        ShadowedText(scene).FontSize(22).Paint(RAYWHITE).Anchor({0.5f, 0.5f}).Position({480, 126});
    if (session.pending)
        status.Content(
            session.victory
                ? std::format("Wszystkie poziomy ukończone! Wynik: {}", *session.pending)
                : std::format("Koniec gry. Wynik: {}", *session.pending)
        );
    else
        status.Content("Najlepsze wyniki");

    // pole na imię - tylko dla wyniku, który mieści się w tabeli
    const bool entering = session.pending && session.Qualifies(*session.pending);
    if (session.pending && !entering) session.pending.reset();
    auto& name = scene.Spawn<node::ui::TextField>()
                     .MaxLength(16)
                     .Size({300, 40})
                     .FontSize(20)
                     .Content(session.player)
                     .Placeholder("Twoje imię")
                     .Position({330, 160})
                     .Visible(entering);
    if (entering) name.Focus();
    auto& hint = scene.Spawn<node::visual::Text>()
                     .Content("Nowy rekord! Wpisz imię i naciśnij Enter")
                     .FontSize(15)
                     .Paint(Color{255, 210, 120, 255})
                     .Anchor({0.5f, 0})
                     .Position({480, 206})
                     .Visible(entering);

    // ---------------- lista: grupa wierszy przesuwana wewnątrz przyciętego panelu
    auto& panel = scene.Spawn<node::Group>()
                      .Position({kPanel.x, kPanel.y})
                      .ClipRect({0, 0, kPanel.width, kPanel.height});
    panel.Spawn<node::shapes::Rect>()
        .Size({kPanel.width, kPanel.height})
        .Paint(Color{24, 26, 48, 255})
        .Filled();
    auto& rows = panel.Spawn<node::Group>();
    for (std::size_t i = 0; i < Session::kMaxScores; ++i) {
        const float y = static_cast<float>(i) * kRow;
        auto& row = rows.Spawn<node::Group>().Position({0, y});
        row.Spawn<node::shapes::Rect>()
            .Position({6, 4})
            .Size({kPanel.width - 12, kRow - 8})
            .Filled()
            .OnUpdate([&session, highlight, i](node::shapes::Rect& self, float) {
                const bool mine = static_cast<int>(i) == *highlight;
                self.Paint(
                    mine ? Color{90, 80, 30, 255}
                         : (i % 2 ? Color{30, 32, 58, 255} : Color{36, 38, 68, 255})
                );
                self.Visible(i < session.scores.size());
            });
        // kolumny osobnymi napisami - czcionka jest proporcjonalna, spacje nie wyrównają tabeli
        auto& rank = row.Spawn<node::visual::Text>().FontSize(20).Position({20, 11});
        auto& who = row.Spawn<node::visual::Text>().FontSize(20).Position({64, 11});
        auto& points =
            row.Spawn<node::visual::Text>().FontSize(20).Anchor({1, 0}).Position({390, 11});
        auto& reached = row.Spawn<node::visual::Text>()
                            .FontSize(16)
                            .Paint(Color{150, 160, 210, 255})
                            .Position({410, 14});
        row.OnUpdate([&session, &rank, &who, &points, &reached, i](node::Group&, float) {
            const bool filled = i < session.scores.size();
            const Color color = i == 0 ? GOLD : RAYWHITE;
            rank.Content(filled ? std::format("{}.", i + 1) : "").Paint(color);
            who.Content(
                   filled ? session.scores[i].name
                          : (i == 0 ? "Brak wyników - zagraj pierwszy!" : "")
            )
                .Paint(color);
            points.Content(filled ? std::format("{}", session.scores[i].score) : "").Paint(color);
            reached.Content(filled ? std::format("poziom {}", session.scores[i].level) : "");
        });
    }
    // przewijanie: kółko myszy nad panelem (zdarzenie przechodzi z tła panelu do grupy)
    const float maxScroll = std::max(0.0f, Session::kMaxScores * kRow - kPanel.height);
    const auto scroll = [&rows, maxScroll](float delta) {
        const float y = std::clamp(rows.Position().y + delta, -maxScroll, 0.0f);
        rows.Position({0, y});
    };
    panel.OnScroll([scroll](node::Group&, float wheel) { scroll(wheel * kRow); });
    scene.Spawn<node::visual::Text>()
        .Content("kółko myszy albo PgUp/PgDn - przewijanie")
        .FontSize(14)
        .Paint(Color{150, 160, 210, 255})
        .Anchor({0.5f, 0})
        .Position({480, kPanel.y + kPanel.height + 8});

    // ---------------- wpis rekordu
    name.OnSubmit([&engine, &session, &name, &hint,
                   highlight](node::ui::TextField& self, const std::string& text) {
        if (!session.pending) return;
        std::string clean = text.empty() ? std::string("Gracz") : text;
        std::erase(clean, ';');
        session.player = clean;
        const int score = *session.pending;
        session.AddScore({clean, score, session.level + 1});
        session.pending.reset();
        session.Save(engine);
        for (std::size_t i = 0; i < session.scores.size(); ++i)
            if (session.scores[i].score == score && session.scores[i].name == clean) {
                *highlight = static_cast<int>(i);
                break;
            }
        self.Blur();
        name.Visible(false);
        hint.Visible(false);
        fx::Play("zycie");
    });

    // ---------------- przyciski
    auto& menu = scene.Spawn<node::Group>();
    std::vector<Button*> buttons;
    buttons.push_back(&menu.Spawn<Button>()
                           .Label("Zagraj ponownie")
                           .Position({215, 560})
                           .Action([&scene, &session, &engine] {
                               session.NewGame();
                               session.customLevel.clear();
                               session.Save(engine);
                               FadeTo(scene, "gra");
                           }));
    buttons.push_back(
        &menu.Spawn<Button>().Label("Menu główne").Position({485, 560}).Action([&scene] {
            FadeTo(scene, "tytul");
        })
    );
    MenuNavigation(menu, buttons);
    if (!entering) buttons.front()->Focus();

    scene.OnUpdate([&engine, scroll](float) {
        const Input& input = engine.Input();
        if (input.KeyPressedRepeat(KEY_PAGE_DOWN) || input.KeyPressed(KEY_PAGE_DOWN)) scroll(-kRow);
        if (input.KeyPressedRepeat(KEY_PAGE_UP) || input.KeyPressed(KEY_PAGE_UP)) scroll(kRow);
    });
    FadeIn(scene);
}
