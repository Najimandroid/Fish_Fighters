#include "UIFishTank.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../data/EntityData.h"
#include "../../data/PlayerData.h"

#include <algorithm>
#include <cmath>

namespace
{
    // The glass of the hub background's porthole (fish_tank.png, 1920 x 1080), and its ring drawn alone
    const sf::Vector2f GLASS_CENTER{ 1388.f, 554.f };
    const sf::Vector2f GLASS_RADII{ 236.f, 204.f };
    const char* RING_TEXTURE = "assets/images/textures/backgrounds/menus/porthole_ring.png";
    const sf::Vector2f RING_POSITION{ 1074.f, 282.f };

    const size_t MAX_SWIMMERS = 6;          // The glass holds about that many without a traffic jam
    const float MAX_FISH_WIDTH = 250.f;     // On screen; a fish is drawn at 0.6 of its battle size at most
    const float FISH_SCALE = 0.6f;
    const float WALK_FRAME_SECONDS = 0.12f; // A bit slower than in battle: they're at home
    const float ATTACK_FRAME_SECONDS = 0.08f;
    const float BOB_PIXELS = 6.f;
    const float BOB_SECONDS = 2.4f;
    const float DEPTH_SPEED = 22.f;         // px/s towards its next depth
    const float BUBBLE_RISE = 70.f;         // px/s
    const float BUBBLE_LIFE = 2.2f;         // Seconds
    const sf::Color BUBBLE_COLOR{ 235, 250, 255, 210 };
}

UIFishTank::UIFishTank(std::shared_ptr<DataLoader> dataLoader) :
    m_dataLoader(std::move(dataLoader))
{
    auto player = m_dataLoader ? m_dataLoader->get_player_data().lock() : nullptr;
    if (!player) return;

    for (int uid : player->equippedUnits)
    {
        if (uid < 0 || m_swimmers.size() >= MAX_SWIMMERS) continue;
        Swimmer swimmer;
        swimmer.data = m_dataLoader->get_unit_data(uid, player->get_unit_form(uid));
        if (!swimmer.data) continue;
        swimmer.texture = &UISkin::texture_file(swimmer.data->texture);
        const sf::Vector2u size = swimmer.texture->getSize();
        if (size.x == 0 || size.y == 0) continue;
        swimmer.frameSize = { static_cast<int>(size.x) / std::max(1, swimmer.data->frameCount), static_cast<int>(size.y) };
        swimmer.scale = std::min(FISH_SCALE, MAX_FISH_WIDTH / static_cast<float>(swimmer.frameSize.x));

        // Somewhere in the glass, each at its own depth, half of them swimming each way
        const float halfHeight = swimmer.frameSize.y * swimmer.scale / 2.f;
        const float top = GLASS_CENTER.y - GLASS_RADII.y + halfHeight + 20.f, bottom = GLASS_CENTER.y + GLASS_RADII.y - halfHeight - 30.f;
        const float slot = static_cast<float>(m_swimmers.size()) / static_cast<float>(MAX_SWIMMERS);
        swimmer.position.y = top + (bottom - top) * std::fmod(slot * 1.7f + 0.15f, 1.f);
        const float halfWidth = glass_half_width(swimmer.position.y, swimmer.frameSize.x * swimmer.scale / 2.f);
        swimmer.position.x = GLASS_CENTER.x + random(-halfWidth, halfWidth);
        swimmer.targetY = swimmer.position.y;
        swimmer.speed = random(35.f, 65.f) * (m_swimmers.size() % 2 ? 1.f : -1.f);
        swimmer.retargetIn = random(2.f, 5.f);
        swimmer.bobPhase = random(0.f, 6.28f);
        swimmer.frame = static_cast<int>(random(0.f, 3.f));
        swimmer.bubblesIn = random(2.f, 8.f);
        m_swimmers.push_back(swimmer);
    }
}

float UIFishTank::random(float from, float to)
{
    if (to <= from) return from;
    return std::uniform_real_distribution<float>(from, to)(m_random);
}

float UIFishTank::glass_half_width(float y, float margin) const
{
    const float dy = (y - GLASS_CENTER.y) / GLASS_RADII.y;
    if (std::abs(dy) >= 1.f) return 0.f;
    return std::max(0.f, GLASS_RADII.x * std::sqrt(1.f - dy * dy) - margin);
}

void UIFishTank::update(float deltaTime)
{
    m_time += deltaTime;

    for (Swimmer& swimmer : m_swimmers)
    {
        const sf::Vector2f half = sf::Vector2f(swimmer.frameSize) * swimmer.scale / 2.f;

        // Across the glass, turning around at its edge (the edge narrows near the top and the bottom)
        swimmer.position.x += swimmer.speed * deltaTime * (swimmer.isAttacking ? 0.3f : 1.f);
        const float halfWidth = glass_half_width(swimmer.position.y, half.x * 0.8f);
        if (swimmer.position.x > GLASS_CENTER.x + halfWidth) { swimmer.position.x = GLASS_CENTER.x + halfWidth; swimmer.speed = -std::abs(swimmer.speed); }
        if (swimmer.position.x < GLASS_CENTER.x - halfWidth) { swimmer.position.x = GLASS_CENTER.x - halfWidth; swimmer.speed = std::abs(swimmer.speed); }

        // Another depth now and then, sometimes a turn for no reason (fish)
        swimmer.retargetIn -= deltaTime;
        if (swimmer.retargetIn <= 0.f)
        {
            swimmer.retargetIn = random(3.f, 6.f);
            const float top = GLASS_CENTER.y - GLASS_RADII.y + half.y + 20.f, bottom = GLASS_CENTER.y + GLASS_RADII.y - half.y - 30.f;
            swimmer.targetY = random(top, std::max(top, bottom));
            if (random(0.f, 1.f) < 0.3f) swimmer.speed = -swimmer.speed;
        }
        const float dy = swimmer.targetY - swimmer.position.y;
        swimmer.position.y += std::clamp(dy, -DEPTH_SPEED * deltaTime, DEPTH_SPEED * deltaTime);

        // Walking frames (0 .. knockback frame - 1) while swimming, the attack frames once after a click
        swimmer.frameTime += deltaTime;
        const float frameSeconds = swimmer.isAttacking ? ATTACK_FRAME_SECONDS : WALK_FRAME_SECONDS;
        if (swimmer.frameTime >= frameSeconds)
        {
            swimmer.frameTime -= frameSeconds;
            const int walkFrames = std::max(1, swimmer.data->knockbackFrameIndex);
            if (swimmer.isAttacking)
            {
                if (++swimmer.frame >= swimmer.data->frameCount) { swimmer.isAttacking = false; swimmer.frame = 0; }
            }
            else
            {
                swimmer.frame = (swimmer.frame + 1) % walkFrames;
            }
        }

        swimmer.bubblesIn -= deltaTime;
        if (swimmer.bubblesIn <= 0.f)
        {
            swimmer.bubblesIn = random(4.f, 10.f);
            release_bubbles(swimmer, 2 + static_cast<int>(random(0.f, 2.f)));
        }
    }

    // Bubbles rise, wobble and pop at the top of the glass
    for (Bubble& bubble : m_bubbles)
    {
        bubble.age += deltaTime;
        bubble.position.y -= BUBBLE_RISE * deltaTime;
        bubble.position.x += std::sin(bubble.age * 6.f + bubble.wobble) * 12.f * deltaTime;
    }
    std::erase_if(m_bubbles, [this](const Bubble& bubble)
        {
            return bubble.age > BUBBLE_LIFE || glass_half_width(bubble.position.y, bubble.radius) <= std::abs(bubble.position.x - GLASS_CENTER.x);
        });
}

/*
 * A few bubbles from the fish's mouth (its front: the left of the frame when it swims left)
 */
void UIFishTank::release_bubbles(const Swimmer& swimmer, int count)
{
    const float front = swimmer.speed > 0.f ? 1.f : -1.f;
    const sf::Vector2f mouth = swimmer.position + sf::Vector2f{ front * swimmer.frameSize.x * swimmer.scale * 0.3f, -swimmer.frameSize.y * swimmer.scale * 0.1f };
    for (int i = 0; i < count; i++)
    {
        Bubble bubble;
        bubble.position = mouth + sf::Vector2f{ random(-6.f, 6.f), -static_cast<float>(i) * 14.f };
        bubble.radius = random(3.f, 7.f);
        bubble.age = -0.12f * static_cast<float>(i);   // One after the other
        bubble.wobble = random(0.f, 6.28f);
        m_bubbles.push_back(bubble);
    }
}

sf::Sprite UIFishTank::sprite_of(const Swimmer& swimmer) const
{
    sf::Sprite sprite(*swimmer.texture, sf::IntRect({ swimmer.frame * swimmer.frameSize.x, 0 }, swimmer.frameSize));
    sprite.setOrigin(sf::Vector2f(swimmer.frameSize) / 2.f);
    // The fish face left; swimming right, they're flipped
    sprite.setScale({ swimmer.speed > 0.f ? -swimmer.scale : swimmer.scale, swimmer.scale });
    const float bob = std::sin((m_time / BOB_SECONDS) * 6.2831853f + swimmer.bobPhase) * BOB_PIXELS;
    sprite.setPosition({ std::round(swimmer.position.x), std::round(swimmer.position.y + bob) });
    return sprite;
}

void UIFishTank::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    for (const Bubble& bubble : m_bubbles)
    {
        if (bubble.age < 0.f) continue;
        sf::CircleShape circle(bubble.radius);
        circle.setOrigin({ bubble.radius, bubble.radius });
        circle.setPosition(bubble.position);
        circle.setFillColor(sf::Color(255, 255, 255, 40));
        circle.setOutlineThickness(2.f);
        sf::Color outline = BUBBLE_COLOR;
        outline.a = static_cast<std::uint8_t>(BUBBLE_COLOR.a * std::clamp(1.f - bubble.age / BUBBLE_LIFE, 0.f, 1.f));
        circle.setOutlineColor(outline);
        window.draw(circle);
    }

    // The deeper ones first (nearer the bottom is nearer the player)
    std::vector<const Swimmer*> order;
    for (const Swimmer& swimmer : m_swimmers) order.push_back(&swimmer);
    std::stable_sort(order.begin(), order.end(), [](const Swimmer* a, const Swimmer* b) { return a->position.y < b->position.y; });
    for (const Swimmer* swimmer : order) window.draw(sprite_of(*swimmer));

    // The ring over them: they swim behind the glass
    sf::Sprite ring(UISkin::texture_file(RING_TEXTURE));
    ring.setPosition(RING_POSITION);
    window.draw(ring);
}

void UIFishTank::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (!pressed || pressed->button != sf::Mouse::Button::Left) return;

    const sf::Vector2f mouse = window.mapPixelToCoords(pressed->position);
    if (!get_bounds().contains(mouse)) return;

    // The fish on top first; only the middle of the frame counts (the frames have room around the fish)
    for (auto it = m_swimmers.rbegin(); it != m_swimmers.rend(); ++it)
    {
        const sf::FloatRect bounds = sprite_of(*it).getGlobalBounds();
        const sf::FloatRect body{ bounds.position + bounds.size * 0.2f, bounds.size * 0.6f };
        if (!body.contains(mouse) || it->isAttacking) continue;

        it->isAttacking = true;
        it->frame = std::max(1, it->data->knockbackFrameIndex) + 1;
        it->frameTime = 0.f;
        if (it->frame >= it->data->frameCount) it->frame = 0;
        release_bubbles(*it, 4);
        play_sound("fish_blub", random(0.9f, 1.15f));
        return;
    }
}

sf::FloatRect UIFishTank::get_bounds() const
{
    return { GLASS_CENTER - GLASS_RADII, GLASS_RADII * 2.f };
}
