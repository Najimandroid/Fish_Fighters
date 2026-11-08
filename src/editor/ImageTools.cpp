#include "ImageTools.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ImageTools
{
    sf::Image make_seamless(const sf::Image& source, unsigned blendWidth)
    {
        const sf::Vector2u size = source.getSize();
        if (size.x < 4 || size.y == 0) return source;

        const unsigned width = size.x;
        const unsigned height = size.y;
        const unsigned blend = std::clamp(blendWidth, 1u, width / 2);
        const unsigned overlappedWidth = width - blend;
        const std::uint8_t* pixels = source.getPixelsPtr();

        // 1. Overlap: the right band fades into the left band (smoothstep), the right band is dropped.
        //    Column 0 is now the column after the last kept one: the wrap is continuous.
        std::vector<std::uint8_t> overlapped(static_cast<size_t>(overlappedWidth) * height * 4);
        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < overlappedWidth; ++x)
            {
                const std::uint8_t* original = &pixels[(static_cast<size_t>(y) * width + x) * 4];
                std::uint8_t* out = &overlapped[(static_cast<size_t>(y) * overlappedWidth + x) * 4];

                if (x >= blend)
                {
                    std::copy(original, original + 4, out);
                    continue;
                }

                const std::uint8_t* rightBand = &pixels[(static_cast<size_t>(y) * width + (overlappedWidth + x)) * 4];
                const float t = (x + 0.5f) / blend;
                const float weight = t * t * (3.f - 2.f * t);   // 0 = right band, 1 = original
                for (int c = 0; c < 4; ++c)
                    out[c] = static_cast<std::uint8_t>(std::lround(rightBand[c] + (original[c] - rightBand[c]) * weight));
            }
        }

        // 2. Back to the original width, interpolating across the wrap so the edges stay continuous
        std::vector<std::uint8_t> result(static_cast<size_t>(width) * height * 4);
        const float ratio = static_cast<float>(overlappedWidth) / width;
        for (unsigned x = 0; x < width; ++x)
        {
            float u = (x + 0.5f) * ratio - 0.5f;
            if (u < 0.f) u += overlappedWidth;
            const unsigned left = static_cast<unsigned>(u) % overlappedWidth;
            const unsigned right = (left + 1) % overlappedWidth;
            const float f = u - std::floor(u);

            for (unsigned y = 0; y < height; ++y)
            {
                const std::uint8_t* a = &overlapped[(static_cast<size_t>(y) * overlappedWidth + left) * 4];
                const std::uint8_t* b = &overlapped[(static_cast<size_t>(y) * overlappedWidth + right) * 4];
                std::uint8_t* out = &result[(static_cast<size_t>(y) * width + x) * 4];
                for (int c = 0; c < 4; ++c)
                    out[c] = static_cast<std::uint8_t>(std::lround(a[c] + (b[c] - a[c]) * f));
            }
        }

        return sf::Image(size, result.data());
    }
}
