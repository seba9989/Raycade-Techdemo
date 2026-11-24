/**
 * @file Backdrop.hpp
 * @brief Tło pola gry narysowane raz na płótnie (node::visual::Canvas) algorytmami z laboratoriów.
 */

#pragma once

#include <raycade>

/**
 * @brief Tworzy płótno tła pod polem gry.
 * @details Po płótnie (bitmapie w RAM) rysuje PrimitiveRenderer z własnymi algorytmami
 * (Rasterizer::Custom - przyrostowy odcinek, parametryczny okrąg i elipsa; Rasterizer::Midpoint -
 * okrąg i elipsa punktu środkowego), wypełnia wielokąt algorytmem scanline (Filled()), obszary
 * przez spójność (FloodFill(), BoundaryFill()) i kopiuje fragment arkusza sprite'ów (Blit()).
 * @param parent Rodzic (pole gry).
 * @param variant Układ 0-3 (klucz "tlo=" pliku poziomu).
 * @return Płótno.
 */
node::visual::Canvas& SpawnBackdrop(Entity& parent, int variant);
