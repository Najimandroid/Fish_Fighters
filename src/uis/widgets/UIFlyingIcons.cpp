#include "UIFlyingIcons.h"
#include "UIPlayerShellsInfo.h"
#include "../UISkin.h"

#include <algorithm>

namespace
{
    const float GAP = 0.07f;      // Between two icons leaving
    const float FLIGHT = 0.65f;
    const float SIZE = 50.f;
}

UIFlyingIcons::UIFlyingIcons() :
    UIElement({ 0.f, 0.f }, { 0.f, 0.f })
{
}

float UIFlyingIcons::random(float from, float to)
{
    return std::uniform_real_distribution<float>(from, std::max(from, to))(m_random);
}

void UIFlyingIcons::launch(const std::string& texture, sf::Vector2f from, std::shared_ptr<UIPlayerShellsInfo> pill, int amount,
    int count, float delay, const std::string& lastSound)
{
    count = std::max(1, count);
    if (pill) pill->hold_back(amount);
    for (int k = 0; k < count; ++k)
    {
        Icon icon;
        icon.texture = texture;
        icon.from = from + sf::Vector2f{ random(-20.f, 20.f), random(-20.f, 20.f) };
        icon.bend = { random(-170.f, 40.f), random(-90.f, 70.f) };
        icon.pill = pill;
        icon.amount = amount / count + (k == count - 1 ? amount % count : 0);
        icon.index = k;
        icon.age = -delay - GAP * static_cast<float>(k);
        icon.spin = random(-540.f, 540.f);
        if (k == count - 1) icon.landSound = lastSound;
        m_icons.push_back(icon);
    }
}

sf::Vector2f UIFlyingIcons::target_of(const Icon& icon) const
{
    auto pill = icon.pill.lock();
    return pill ? pill->get_icon_center() : sf::Vector2f{ 1645.f, 55.f };
}

void UIFlyingIcons::land(Icon& icon, bool quietly)
{
    if (auto pill = icon.pill.lock()) pill->receive(icon.amount);
    icon.amount = 0;
    icon.age = FLIGHT + 1.f;
    if (quietly) return;
    play_sound("reward_tick", 1.f + 0.06f * static_cast<float>(icon.index % 12));
    if (!icon.landSound.empty()) play_sound(icon.landSound);
}

void UIFlyingIcons::land_all()
{
    bool any = false;
    for (Icon& icon : m_icons) { any = true; land(icon, true); }
    m_icons.clear();
    if (any) play_sound("shell_collect");
}

void UIFlyingIcons::update(float deltaTime)
{
    for (Icon& icon : m_icons)
    {
        icon.age += deltaTime;
        if (icon.age >= FLIGHT && icon.age < FLIGHT + 1.f) land(icon, false);
    }
    std::erase_if(m_icons, [](const Icon& icon) { return icon.age >= FLIGHT; });
}

void UIFlyingIcons::render(sf::RenderWindow& window)
{
    for (const Icon& icon : m_icons)
    {
        const float t = icon.age / FLIGHT;
        if (t <= 0.f || t >= 1.f) continue;
        const sf::Texture& texture = UISkin::texture(icon.texture);
        const sf::Vector2f textureSize(texture.getSize());
        if (textureSize.x <= 0.f || textureSize.y <= 0.f) continue;

        // Out sideways at the start's height, then up into the pill from below
        const sf::Vector2f to = target_of(icon);
        const sf::Vector2f control{ to.x + icon.bend.x, icon.from.y + icon.bend.y };
        const float e = t * t * (3.f - 2.f * t);
        const sf::Vector2f position = icon.from * ((1.f - e) * (1.f - e)) + control * (2.f * (1.f - e) * e) + to * (e * e);
        const float size = SIZE * (t < 0.15f ? 0.4f + 0.6f * t / 0.15f : 1.f - 0.3f * (t - 0.15f) / 0.85f);

        sf::Sprite sprite(texture);
        sprite.setOrigin(textureSize / 2.f);
        sprite.setScale({ size / textureSize.x, size / textureSize.y });
        sprite.setRotation(sf::degrees(icon.spin * t));
        sprite.setPosition(position);
        window.draw(sprite);
    }
}
