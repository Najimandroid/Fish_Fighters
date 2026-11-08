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
    // Its last `blendWidth` columns are crossfaded over its first ones, so the right edge flows into the
    // left one, then the image is stretched back to its width (by blendWidth / width, a few percent).
    // The height and everything away from the edges stay as they were.
    sf::Image make_seamless(const sf::Image& source, unsigned blendWidth);
}
