#include "EntityPortraits.h"
#include "UISkin.h"
#include "../data/EntityData.h"

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

namespace
{
    const std::uint8_t SOLID_ALPHA = 32;   // Pixels at least this opaque are part of the drawing

    /*
     * A solid black silhouette of a drawing, made once per image: the drawing and every hole it encloses
     * are black, only what's reachable from the image border stays transparent.
     * (Tinting the sprite black isn't enough: line art with a transparent inside would only show black lines.)
     */
    const sf::Texture& silhouette_of(const std::string& path)
    {
        static std::unordered_map<std::string, std::unique_ptr<sf::Texture>> silhouettes;

        auto it = silhouettes.find(path);
        if (it != silhouettes.end()) return *it->second;

        auto texture = std::make_unique<sf::Texture>();
        const sf::Image source = UISkin::texture_file(path).copyToImage();
        const sf::Vector2u size = source.getSize();

        if (size.x > 0 && size.y > 0)
        {
            // Flood fill the transparent outside from the borders
            std::vector<std::uint8_t> isOutside(static_cast<size_t>(size.x) * size.y, 0);
            std::vector<sf::Vector2u> toVisit;
            auto visit = [&](unsigned x, unsigned y)
                {
                    const size_t index = static_cast<size_t>(y) * size.x + x;
                    if (isOutside[index] || source.getPixel({ x, y }).a >= SOLID_ALPHA) return;
                    isOutside[index] = 1;
                    toVisit.push_back({ x, y });
                };
            for (unsigned x = 0; x < size.x; ++x) { visit(x, 0); visit(x, size.y - 1); }
            for (unsigned y = 0; y < size.y; ++y) { visit(0, y); visit(size.x - 1, y); }
            while (!toVisit.empty())
            {
                const sf::Vector2u pixel = toVisit.back();
                toVisit.pop_back();
                if (pixel.x > 0) visit(pixel.x - 1, pixel.y);
                if (pixel.x + 1 < size.x) visit(pixel.x + 1, pixel.y);
                if (pixel.y > 0) visit(pixel.x, pixel.y - 1);
                if (pixel.y + 1 < size.y) visit(pixel.x, pixel.y + 1);
            }

            // Black inside, the outside keeps the soft edges of the drawing
            sf::Image result(size, sf::Color::Transparent);
            for (unsigned y = 0; y < size.y; ++y)
            {
                for (unsigned x = 0; x < size.x; ++x)
                {
                    const size_t index = static_cast<size_t>(y) * size.x + x;
                    const std::uint8_t alpha = isOutside[index] ? source.getPixel({ x, y }).a : 255;
                    result.setPixel({ x, y }, sf::Color(0, 0, 0, alpha));
                }
            }
            if (!texture->loadFromImage(result)) texture = std::make_unique<sf::Texture>();
            texture->setSmooth(true);
        }

        return *silhouettes.emplace(path, std::move(texture)).first->second;
    }
}

void EntityPortraits::draw_unit(sf::RenderTarget& target, const std::string& iconPath, const sf::FloatRect& area, bool isSilhouette)
{
    UISkin::draw_cropped(target, isSilhouette ? silhouette_of(iconPath) : UISkin::texture_file(iconPath), area);
}

void EntityPortraits::draw_enemy(sf::RenderTarget& target, const EntityData& enemy, const sf::FloatRect& area, bool isSilhouette)
{
    const sf::Texture& sheet = isSilhouette ? silhouette_of(enemy.texture) : UISkin::texture_file(enemy.texture);
    const sf::Vector2i sheetSize(sheet.getSize());
    if (sheetSize.x <= 0 || sheetSize.y <= 0) return;

    // First frame, back half (BattleEnemy: the hitbox is the left half of the frame)
    const int frameWidth = sheetSize.x / std::max(1, enemy.frameCount);
    const sf::IntRect body({ 0, 0 }, { std::max(1, frameWidth / 2), sheetSize.y });
    UISkin::draw_fitted(target, sheet, body, area);
}
