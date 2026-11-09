#include "ImageTools.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace
{
    constexpr int BLUR_RADIUS = 16;   // Light and color: what's bigger than ~16 px
    constexpr int CUT_SOFTNESS = 4;   // Details switch sides over 2 x 4 px around the cut

    float smoothstep(float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    // Box blur of the RGB channels (two separable passes, edges clamped): the light and colors of the image
    std::vector<float> blur_rgb(const std::uint8_t* pixels, int width, int height, int radius)
    {
        std::vector<float> horizontal(static_cast<size_t>(width) * height * 3);
        std::vector<float> result(horizontal.size());
        const float window = static_cast<float>(radius * 2 + 1);

        for (int y = 0; y < height; ++y)
        {
            for (int c = 0; c < 3; ++c)
            {
                float sum = 0.f;
                for (int k = -radius; k <= radius; ++k)
                    sum += pixels[(static_cast<size_t>(y) * width + std::clamp(k, 0, width - 1)) * 4 + c];
                for (int x = 0; x < width; ++x)
                {
                    horizontal[(static_cast<size_t>(y) * width + x) * 3 + c] = sum / window;
                    const int removed = std::clamp(x - radius, 0, width - 1);
                    const int added = std::clamp(x + radius + 1, 0, width - 1);
                    sum += pixels[(static_cast<size_t>(y) * width + added) * 4 + c] - pixels[(static_cast<size_t>(y) * width + removed) * 4 + c];
                }
            }
        }

        for (int x = 0; x < width; ++x)
        {
            for (int c = 0; c < 3; ++c)
            {
                float sum = 0.f;
                for (int k = -radius; k <= radius; ++k)
                    sum += horizontal[(static_cast<size_t>(std::clamp(k, 0, height - 1)) * width + x) * 3 + c];
                for (int y = 0; y < height; ++y)
                {
                    result[(static_cast<size_t>(y) * width + x) * 3 + c] = sum / window;
                    const int removed = std::clamp(y - radius, 0, height - 1);
                    const int added = std::clamp(y + radius + 1, 0, height - 1);
                    sum += horizontal[(static_cast<size_t>(added) * width + x) * 3 + c] - horizontal[(static_cast<size_t>(removed) * width + x) * 3 + c];
                }
            }
        }
        return result;
    }
}

namespace ImageTools
{
    sf::Image make_seamless(const sf::Image& source, unsigned overlap)
    {
        const sf::Vector2u size = source.getSize();
        if (size.x < 64 || size.y == 0) return source;

        const int width = static_cast<int>(size.x);
        const int height = static_cast<int>(size.y);
        const int band = std::clamp(static_cast<int>(overlap), 16, width / 2);
        const int rightStart = width - band;          // The right edge band starts here
        const std::uint8_t* pixels = source.getPixelsPtr();
        const std::vector<float> low = blur_rgb(pixels, width, height, BLUR_RADIUS);

        auto pixel = [&](int x, int y, int c) { return static_cast<float>(pixels[(static_cast<size_t>(y) * width + x) * 4 + c]); };
        auto lowAt = [&](int x, int y, int c) { return low[(static_cast<size_t>(y) * width + x) * 3 + c]; };

        // ----- 1. The cut: the path through the overlap where the details of both edges look most alike -----
        // Dynamic programming, top to bottom, moving at most one column per row
        const int margin = CUT_SOFTNESS + 2;
        const float infinite = std::numeric_limits<float>::max() / 4.f;
        std::vector<float> total(static_cast<size_t>(band) * height, infinite);
        for (int y = 0; y < height; ++y)
        {
            for (int x = margin; x < band - margin; ++x)
            {
                float cost = 0.f;
                for (int c = 0; c < 3; ++c)
                {
                    const float detailRight = pixel(rightStart + x, y, c) - lowAt(rightStart + x, y, c);
                    const float detailLeft = pixel(x, y, c) - lowAt(x, y, c);
                    cost += (detailRight - detailLeft) * (detailRight - detailLeft);
                }

                float best = 0.f;
                if (y > 0)
                {
                    best = infinite;
                    for (int dx = -1; dx <= 1; ++dx)
                        best = std::min(best, total[static_cast<size_t>(y - 1) * band + std::clamp(x + dx, 0, band - 1)]);
                }
                total[static_cast<size_t>(y) * band + x] = cost + best;
            }
        }

        std::vector<int> cut(height);
        auto cheapest = [&](int y, int from, int to)
            {
                int bestX = std::clamp(from, margin, band - margin - 1);
                for (int x = std::max(from, margin); x <= std::min(to, band - margin - 1); ++x)
                    if (total[static_cast<size_t>(y) * band + x] < total[static_cast<size_t>(y) * band + bestX]) bestX = x;
                return bestX;
            };
        cut[height - 1] = cheapest(height - 1, 0, band - 1);
        for (int y = height - 2; y >= 0; --y) cut[y] = cheapest(y, cut[y + 1] - 1, cut[y + 1] + 1);

        // ----- 2. The new left edge: light and colors fade across the whole overlap, details switch at the cut -----
        // Column 0 continues the last kept column of the right side, so the copies join without a seam
        const int resultWidth = width - band;
        std::vector<std::uint8_t> result(static_cast<size_t>(resultWidth) * height * 4);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < resultWidth; ++x)
            {
                std::uint8_t* out = &result[(static_cast<size_t>(y) * resultWidth + x) * 4];
                if (x >= band)
                {
                    const std::uint8_t* original = &pixels[(static_cast<size_t>(y) * width + x) * 4];
                    std::copy(original, original + 4, out);
                    continue;
                }

                const float lightWeight = smoothstep((x + 0.5f) / band);                                       // 0 = right edge, 1 = left edge
                const float detailWeight = smoothstep((x - cut[y] + CUT_SOFTNESS) / (2.f * CUT_SOFTNESS));
                for (int c = 0; c < 3; ++c)
                {
                    const float lowRight = lowAt(rightStart + x, y, c);
                    const float lowLeft = lowAt(x, y, c);
                    const float detailRight = pixel(rightStart + x, y, c) - lowRight;
                    const float detailLeft = pixel(x, y, c) - lowLeft;
                    const float value = lowRight + (lowLeft - lowRight) * lightWeight + detailRight + (detailLeft - detailRight) * detailWeight;
                    out[c] = static_cast<std::uint8_t>(std::clamp(std::lround(value), 0l, 255l));
                }
                out[3] = static_cast<std::uint8_t>(std::lround(pixel(rightStart + x, y, 3) + (pixel(x, y, 3) - pixel(rightStart + x, y, 3)) * lightWeight));
            }
        }

        return sf::Image(sf::Vector2u(static_cast<unsigned>(resultWidth), size.y), result.data());
    }
}
