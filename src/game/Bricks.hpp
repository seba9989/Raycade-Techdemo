/**
 * @file Bricks.hpp
 * @brief Cegły: nieruchome ciała (node::physics::StaticBody) ze spritem z arkusza.
 * @details Rodzaje: zwykła, twarda (pęka), metalowa (niezniszczalna, błyska animacją z czasami
 * klatek z pliku), wybuchowa (niszczy sąsiadów przez Scene::Overlap()), ruchoma (StaticBody
 * przesuwany w kroku stałym - silnik robi z niego ciało kinematyczne) i ukryta (Visible(false)
 * do pierwszego trafienia). Kolory odłamków i iskier cegła bierze z kontekstu poddrzewa
 * (Palette), który udostępnia grupa wszystkich cegieł poziomu.
 */

#pragma once

#include <raycade>

#include <array>

struct Round;
struct Session;

/** @brief Kolory cegieł - kontekst poddrzewa udostępniany przez grupę cegieł. */
struct Palette {
    std::array<Color, 6> colors{
        Color{230, 72, 72, 255},  Color{245, 150, 50, 255},
        Color{240, 210, 60, 255}, Color{90, 200, 90, 255},
        Color{60, 190, 230, 255}, Color{170, 100, 230, 255}
    };                                  /**< Zwykłe cegły 1-6. */
    Color hard{140, 150, 180, 255};     /**< Twarda. */
    Color metal{210, 215, 225, 255};    /**< Metal (iskry). */
    Color explosive{255, 120, 50, 255}; /**< Wybuchowa. */
    Color hidden{200, 240, 255, 255};   /**< Ukryta. */
};

/** @brief Rodzaj cegły. */
enum class BrickKind {
    Normal,    /**< 1 trafienie. */
    Hard,      /**< 3 trafienia, pęka. */
    Metal,     /**< Niezniszczalna (oprócz wybuchu). */
    Explosive, /**< Niszczy cegły w promieniu. */
    Moving,    /**< Jeździ w poziomie. */
    Hidden,    /**< Niewidoczna do pierwszego trafienia, potem zwykła. */
};

/** @brief Cegła. */
class Brick : public node::physics::StaticBody {
  public:
    /** @brief Buduje collider i sprite (punkt zaczepienia - środek cegły). */
    Brick();

    /**
     * @brief Ustawia rodzaj cegły.
     * @param kind Rodzaj.
     * @param color Kolor palety 0-5 (zwykła i ruchoma).
     * @param direction Kierunek startowy ruchomej (-1 albo 1).
     * @return Ta sama cegła.
     */
    template <typename Self>
    Self& Kind(this Self& self, BrickKind kind, int color = 0, float direction = 1.0f) {
        self.kind_ = kind;
        self.color_ = color;
        self.direction_ = direction;
        self.Setup();
        return self;
    }

    /** @brief Podaje rodzaj cegły. */
    BrickKind Kind() const { return kind_; }

    /** @brief Czy cegła liczy się do ukończenia poziomu (metal nie). */
    bool IsCounted() const { return kind_ != BrickKind::Metal; }

    /**
     * @brief Trafienie piłką, laserem albo wybuchem.
     * @param blast Wybuch niszczy każdą cegłę od razu (także metal) i nie wyrzuca kapsuł.
     */
    void Hit(bool blast = false);

  protected:
    /** @brief Odczytuje kontekst: Palette (poddrzewo), Round (scena), Session (silnik). */
    void OnMount() override;

    /** @brief Ruch cegły ruchomej. */
    void FixedTick(float step, const Input& input) override;

  private:
    /** @brief Animacja i wytrzymałość dla rodzaju. */
    void Setup();

    /** @brief Zniszczenie: punkty, efekty, kapsuła, wybuch. */
    void Break(bool blast);

    /** @brief Wybuch: niszczy cegły w promieniu (Scene::Overlap()). */
    void Explode();

    /** @brief Kolor efektów cegły. */
    Color Tint() const;

    BrickKind kind_ = BrickKind::Normal;     /**< Rodzaj. */
    int color_ = 0;                          /**< Kolor palety. */
    int hp_ = 1;                             /**< Pozostałe trafienia. */
    float direction_ = 1.0f;                 /**< Kierunek startowy ruchomej. */
    float homeX_ = 0.0f;                     /**< Środek ruchu ruchomej. */
    float time_ = 0.0f;                      /**< Czas ruchu ruchomej. */
    bool broken_ = false;                    /**< Zniszczona w tej klatce. */
    node::visual::Sprite* sprite_ = nullptr; /**< Grafika. */
    const Palette* palette_ = nullptr;       /**< Kolory (kontekst poddrzewa). */
    Round* round_ = nullptr;                 /**< Stan poziomu. */
    Session* session_ = nullptr;             /**< Wynik. */
};
