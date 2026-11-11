#include "UIFishParade.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../data/EntityData.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace
{
    const float FLOOR_Y = 1000.f;           // Where they walk: the floor strip of the title background
    const float SPEED = 80.f;               // px/s, to the left
    const float GAP = 140.f;                // Between two fish
    const float FISH_SCALE = 0.6f;          // Of their battle size
    const float MAX_FISH_WIDTH = 260.f;
    const float FRAME_SECONDS = 0.1f;       // As in battle
    const float SCREEN_WIDTH = 1920.f;
}

UIFishParade::UIFishParade(std::shared_ptr<DataLoader> dataLoader)
{
    if (!dataLoader) return;

    // Every fish of the game, in its first form, by number
    std::set<int> uids;
    for (const auto& [key, data] : dataLoader->get_units_database()) uids.insert(key.first);

    float x = SCREEN_WIDTH + 100.f;
    for (int uid : uids)
    {
        Marcher marcher;
        marcher.data = dataLoader->get_unit_data(uid, 0);
        if (!marcher.data) continue;
        marcher.texture = &UISkin::texture_file(marcher.data->texture);
        const sf::Vector2u size = marcher.texture->getSize();
        if (size.x == 0 || size.y == 0) continue;
        marcher.frameSize = { static_cast<int>(size.x) / std::max(1, marcher.data->frameCount), static_cast<int>(size.y) };
        marcher.scale = std::min(FISH_SCALE, MAX_FISH_WIDTH / static_cast<float>(marcher.frameSize.x));
        const float width = marcher.frameSize.x * marcher.scale;
        marcher.x = x + width / 2.f;
        marcher.frame = static_cast<int>(m_marchers.size()) % std::max(1, marcher.data->knockbackFrameIndex);
        x += width + GAP;
        m_marchers.push_back(marcher);
    }
    // The parade starts already on screen, its head near the left
    m_loopWidth = std::max(x - SCREEN_WIDTH, SCREEN_WIDTH + 400.f);
    for (Marcher& marcher : m_marchers) marcher.x -= SCREEN_WIDTH * 0.75f;
}

void UIFishParade::update(float deltaTime)
{
    for (Marcher& marcher : m_marchers)
    {
        marcher.x -= SPEED * deltaTime;
        const float halfWidth = marcher.frameSize.x * marcher.scale / 2.f;
        if (marcher.x < -halfWidth) marcher.x += m_loopWidth;   // Back on the right, the gaps kept

        // Walking frames: 0 .. knockback frame - 1 (BattleEntity's)
        marcher.frameTime += deltaTime;
        if (marcher.frameTime >= FRAME_SECONDS)
        {
            marcher.frameTime -= FRAME_SECONDS;
            marcher.frame = (marcher.frame + 1) % std::max(1, marcher.data->knockbackFrameIndex);
        }
    }
}

void UIFishParade::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    for (const Marcher& marcher : m_marchers)
    {
        sf::Sprite sprite(*marcher.texture, sf::IntRect({ marcher.frame * marcher.frameSize.x, 0 }, marcher.frameSize));
        sprite.setOrigin({ marcher.frameSize.x / 2.f, static_cast<float>(marcher.frameSize.y) });   // Standing on the floor
        sprite.setScale({ marcher.scale, marcher.scale });
        sprite.setPosition({ std::round(marcher.x), FLOOR_Y });
        window.draw(sprite);
    }
}
