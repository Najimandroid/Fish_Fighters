#pragma once

#include <SFML/Graphics/Image.hpp>

/*
 * ImageTools
 * ----------
 * Image processing for the editor's image tools.
 */
namespace ImageTools
{
    // Makes an image repeat seamlessly side by side (stage backgrounds).
    // Its last `overlap` columns are laid over its first ones, and the result is `overlap` px narrower:
    //  - light and colors (a blur) fade from one edge to the other across the whole overlap
    //  - details are cut along the path where both edges look most alike (dynamic programming),
    //    so nothing shows twice
    // Rows don't move: the ground line stays where it was.
    sf::Image make_seamless(const sf::Image& source, unsigned overlap);
}
