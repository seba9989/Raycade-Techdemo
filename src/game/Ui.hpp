/**
 * @file Ui.hpp
 * @brief Elementy interfejsu: przycisk z fokusem klawiatury, cień napisu, opóźnienie,
 * zaciemnienie przejść i nawigacja strzałkami.
 */

#pragma once

#include <raycade>

#include <functional>
#include <string>
#include <vector>

/**
 * @brief Przycisk menu: prostokąt z cechą Focusable.
 * @details Fokus dostaje kliknięciem, ruchem myszy nad nim, klawiszem Tab albo strzałkami
 * (MenuNavigation()). Aktywuje go kliknięcie, Enter/spacja (akcja "wybierz") i przycisk A pada.
 * Kółko myszy nad przyciskiem wywołuje OnScroll() (np. zmiana głośności).
 */
class Button : public node::shapes::Rect, public Focusable {
  public:
    /** @brief Buduje napis i reakcje na mysz i fokus. */
    Button();

    /** @brief Ustawia napis przycisku. */
    template <typename Self> Self& Label(this Self& self, std::string text) {
        self.label_->Content(std::move(text));
        return self;
    }

    /** @brief Ustawia działanie przycisku. */
    template <typename Self> Self& Action(this Self& self, std::function<void()> action) {
        self.action_ = std::move(action);
        return self;
    }

    /** @brief Wywołuje działanie (z dźwiękiem). */
    void Activate();

  protected:
    /** @brief Wygląd zależny od fokusu i aktywacja z klawiatury i pada. */
    void Tick(float dt, const Input& input) override;

  private:
    node::visual::Text* label_ = nullptr; /**< Napis. */
    std::function<void()> action_;        /**< Działanie. */
    float glow_ = 0.0f;                   /**< Podświetlenie 0-1 (płynne). */
};

/**
 * @brief Napis z cieniem: najpierw powstaje cień (rodzeństwo rysowane wcześniej, więc pod
 * spodem), potem napis. Cień co klatkę kopiuje treść, układ, przekształcenie, warstwę
 * i widoczność napisu, przesunięty o 2 px.
 * @details Dziecko jest zawsze rysowane nad rodzicem, dlatego cień nie może być dzieckiem
 * napisu. Napis i cień muszą żyć tak samo długo (np. do końca sceny).
 * @tparam Parent Scene albo Entity (oba mają Spawn<T>()).
 * @param parent Rodzic obu napisów.
 * @return Napis (do dalszej konfiguracji łańcuchem).
 */
template <typename Parent> node::visual::Text& ShadowedText(Parent& parent) {
    auto& shadow = parent.template Spawn<node::visual::Text>();
    auto& text = parent.template Spawn<node::visual::Text>();
    shadow.Paint(Fade(BLACK, 0.65f)).OnUpdate([&text](node::visual::Text& self, float) {
        if (self.Content() != text.Content()) self.Content(text.Content());
        self.FontSize(text.FontSize())
            .Anchor(text.Anchor())
            .Position({text.Position().x + 2, text.Position().y + 2})
            .Rotation(text.Rotation())
            .Scale(text.Scale())
            .Align(text.Align())
            .LineHeight(text.LineHeight())
            .WrapWidth(text.WrapWidth())
            .ScreenSpace(text.IsScreenSpace())
            .DrawLayer(text.DrawLayer())
            .Visible(text.IsVisible());
    });
    return text;
}

/**
 * @brief Wywołuje funkcję po czasie (pusty obiekt sceny z OnUpdate, który sam się usuwa).
 * @param scene Scena.
 * @param seconds Opóźnienie.
 * @param action Funkcja.
 */
void Delay(Scene& scene, float seconds, std::function<void()> action);

/** @brief Rozjaśnia scenę z czerni (prostokąt ekranu nad wszystkim). */
void FadeIn(Scene& scene, float seconds = 0.4f);

/**
 * @brief Ściemnia scenę, przełącza na inną (Engine::SwitchScene()) i zwalnia opuszczaną
 * (Engine::UnloadScene()), żeby przy powrocie powstała od nowa.
 */
void FadeTo(Scene& scene, std::string next, float seconds = 0.35f);

/**
 * @brief Strzałki w górę/w dół (z powtarzaniem przy przytrzymaniu - KeyPressedRepeat())
 * i krzyżak pada przenoszą fokus między przyciskami.
 * @param parent Rodzic obiektu nawigacji (znika razem z nim, np. menu pauzy).
 * @param buttons Przyciski w kolejności.
 */
void MenuNavigation(Entity& parent, std::vector<Button*> buttons);
