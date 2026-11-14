#include "UIHubMascot.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../data/BattlefieldLayout.h"
#include "../../data/PlayerData.h"

#include <algorithm>
#include <cmath>

namespace
{
    const int FISH_UID = 1;                         // The Fish
    const sf::Vector2f HOME{ 1040.f, 922.f };       // The fish's center
    const sf::Vector2f FISH_BOX{ 190.f, 140.f };    // It fits in this
    const float FLOOR_Y = 1000.f;                   // Its shadow
    const float SHADOW_WIDTH = 150.f, SHADOW_HEIGHT = 34.f;
    const float WALK_FRAME_SECONDS = 0.14f;
    const float HOP_SECONDS = 0.45f, HOP_HEIGHT = 70.f;

    const float BUBBLE_LEFT = 644.f, BUBBLE_WIDTH = 440.f, BUBBLE_BOTTOM = 838.f;   // Clear of the porthole's ring
    const float PADDING = 26.f, CORNER = 30.f, OUTLINE = 5.f;
    const sf::Vector2f TAIL_TIP{ 1012.f, 872.f };   // The bubble's tail points at the fish
    const unsigned int TEXT_SIZE = 30;
    const sf::Color PAPER{ 252, 253, 255 };

    const float CHARS_PER_SECOND = 42.f;
    const float POP_SECONDS = 0.25f;
    const float HOLD_SECONDS = 6.5f;                // Once all typed
    const float FADE_SECONDS = 0.3f, GAP_SECONDS = 1.4f;

    float ease_out_back(float t)
    {
        const float c = 1.70158f;
        t -= 1.f;
        return 1.f + (c + 1.f) * t * t * t + c * t * t;
    }

    // A rounded rectangle (the bubble)
    sf::ConvexShape rounded_rect(const sf::FloatRect& rect, float radius)
    {
        const int perCorner = 8;
        sf::ConvexShape shape(perCorner * 4);
        const sf::Vector2f centers[] = {
            { rect.position.x + rect.size.x - radius, rect.position.y + radius },
            { rect.position.x + rect.size.x - radius, rect.position.y + rect.size.y - radius },
            { rect.position.x + radius, rect.position.y + rect.size.y - radius },
            { rect.position.x + radius, rect.position.y + radius } };
        for (int corner = 0; corner < 4; ++corner)
            for (int i = 0; i < perCorner; ++i)
            {
                const float angle = (-90.f + 90.f * static_cast<float>(corner) + 90.f * static_cast<float>(i) / (perCorner - 1)) * 3.14159265f / 180.f;
                shape.setPoint(static_cast<std::size_t>(corner * perCorner + i), centers[corner] + sf::Vector2f{ std::cos(angle), std::sin(angle) } * radius);
            }
        return shape;
    }
}

UIHubMascot::UIHubMascot(std::shared_ptr<DataLoader> dataLoader, std::vector<std::string> tips) :
    UIElement({ FISH_BOX.x, FISH_BOX.y }, HOME - FISH_BOX / 2.f),
    m_dataLoader(std::move(dataLoader)),
    m_tips(std::move(tips)),
    m_text(UISkin::regular_font())
{
    UISkin::style_body_text(m_text, TEXT_SIZE, UISkin::OUTLINE);
    m_text.setOutlineThickness(0.f);
    if (!m_tips.empty()) next_tip();
    m_tipTime = -0.9f;   // The first one once the screen is in
}

void UIHubMascot::next_tip()
{
    if (m_tips.empty()) return;
    m_tipIndex = (m_tipIndex + 1) % static_cast<int>(m_tips.size());
    m_wrapped = UISkin::wrap_text(m_tips[m_tipIndex], TEXT_SIZE, BUBBLE_WIDTH - 2.f * PADDING, UISkin::regular_font());
    m_text.setString(m_wrapped);
    m_bubbleHeight = m_text.getLocalBounds().size.y + 2.f * PADDING + 8.f;
    m_tipTime = 0.f;
}

sf::FloatRect UIHubMascot::bubble_rect() const
{
    return { { BUBBLE_LEFT, BUBBLE_BOTTOM - m_bubbleHeight }, { BUBBLE_WIDTH, m_bubbleHeight } };
}

sf::Vector2f UIHubMascot::fish_center() const
{
    float hop = 0.f;
    if (m_hopTime >= 0.f && m_hopTime < HOP_SECONDS)
    {
        const float t = m_hopTime / HOP_SECONDS;
        hop = 4.f * HOP_HEIGHT * t * (1.f - t);
    }
    return HOME + sf::Vector2f{ 0.f, std::sin(m_time * 2.2f) * 8.f - hop };
}

void UIHubMascot::update(float deltaTime)
{
    m_time += deltaTime;
    if (m_hopTime >= 0.f) m_hopTime += deltaTime;
    if (m_tips.empty()) return;

    m_tipTime += deltaTime;
    const float typing = static_cast<float>(m_wrapped.size()) / CHARS_PER_SECOND;
    if (m_tipTime > typing + HOLD_SECONDS + FADE_SECONDS + GAP_SECONDS) next_tip();
}

void UIHubMascot::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const sf::Vector2f center = fish_center();

    // Its shadow on the floor: smaller and fainter while it hops
    const sf::Texture& shadowTexture = UISkin::texture_file(SHADOW_TEXTURE);
    const sf::Vector2f shadowSize(shadowTexture.getSize());
    if (shadowSize.x > 0.f && shadowSize.y > 0.f)
    {
        const float lift = std::clamp((HOME.y - center.y) / HOP_HEIGHT, 0.f, 1.f);
        sf::Sprite shadow(shadowTexture);
        shadow.setOrigin(shadowSize / 2.f);
        shadow.setScale({ SHADOW_WIDTH * (1.f - 0.3f * lift) / shadowSize.x, SHADOW_HEIGHT * (1.f - 0.3f * lift) / shadowSize.y });
        shadow.setPosition({ HOME.x, FLOOR_Y });
        shadow.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255.f * (1.f - 0.5f * lift))));
        window.draw(shadow);
    }

    // The fish, swimming on the spot (the sheets face left: towards the bubble and the menu)
    if (auto player = m_dataLoader->get_player_data().lock())
        if (auto data = m_dataLoader->get_unit_data(FISH_UID, player->get_unit_form(FISH_UID)); data && !data->texture.empty())
        {
            const UISkin::SwimFrames& swim = UISkin::swim_frames(data->texture, data->frameCount, data->knockbackFrameIndex);
            if (swim.box.size.x > 0 && swim.box.size.y > 0)
            {
                const int frame = static_cast<int>(m_time / WALK_FRAME_SECONDS) % std::max(1, swim.count);
                sf::Sprite fish(UISkin::texture_file(data->texture), { { frame * swim.frameWidth + swim.box.position.x, swim.box.position.y }, swim.box.size });
                const sf::Vector2f boxSize(swim.box.size);
                float fit = std::min(FISH_BOX.x / boxSize.x, FISH_BOX.y / boxSize.y);
                if (m_isHovered) fit *= 1.05f;
                // A little squash when it lands from a hop
                float squash = 0.f;
                if (m_hopTime >= HOP_SECONDS && m_hopTime < HOP_SECONDS + 0.2f)
                    squash = 0.12f * std::sin((m_hopTime - HOP_SECONDS) / 0.2f * 3.14159265f);
                fish.setOrigin(boxSize / 2.f);
                fish.setScale({ fit * (1.f + squash), fit * (1.f - squash) });
                fish.setRotation(sf::degrees(std::sin(m_time * 1.3f) * 4.f));
                fish.setPosition(center);
                window.draw(fish);
            }
        }

    // The speech bubble: pops out of its tail, types the tip, fades away
    if (m_tips.empty() || m_tipTime < 0.f) return;
    const float typing = static_cast<float>(m_wrapped.size()) / CHARS_PER_SECOND;
    const float fadeStart = typing + HOLD_SECONDS;
    const float alpha = m_tipTime < fadeStart ? 1.f : std::max(0.f, 1.f - (m_tipTime - fadeStart) / FADE_SECONDS);
    if (alpha <= 0.f) return;
    const float pop = m_tipTime < POP_SECONDS ? 0.6f + 0.4f * ease_out_back(m_tipTime / POP_SECONDS) : 1.f;

    sf::RenderStates states;
    states.transform.translate(TAIL_TIP).scale({ pop, pop }).translate(-TAIL_TIP);
    const auto with_alpha = [alpha](sf::Color color) { color.a = static_cast<std::uint8_t>(color.a * alpha); return color; };

    const sf::FloatRect rect = bubble_rect();
    const float tailX = TAIL_TIP.x - 34.f;
    sf::ConvexShape tail(3);
    tail.setPoint(0, { tailX - 26.f, rect.position.y + rect.size.y - 6.f });
    tail.setPoint(1, { tailX + 18.f, rect.position.y + rect.size.y - 6.f });
    tail.setPoint(2, TAIL_TIP);
    tail.setFillColor(with_alpha(PAPER));
    tail.setOutlineThickness(OUTLINE);
    tail.setOutlineColor(with_alpha(UISkin::OUTLINE));
    window.draw(tail, states);

    sf::ConvexShape body = rounded_rect(rect, CORNER);
    body.setFillColor(with_alpha(PAPER));
    body.setOutlineThickness(OUTLINE);
    body.setOutlineColor(with_alpha(UISkin::OUTLINE));
    window.draw(body, states);
    tail.setOutlineThickness(0.f);   // Over the bubble's outline: the tail comes out of it
    window.draw(tail, states);

    const std::size_t shown = std::min(m_wrapped.size(), static_cast<std::size_t>(std::max(0.f, m_tipTime) * CHARS_PER_SECOND));
    m_text.setString(m_wrapped.substr(0, shown));
    UISkin::set_text_color(m_text, with_alpha(UISkin::OUTLINE));
    m_text.setOutlineThickness(0.f);
    UISkin::set_text_anchor(m_text, { 0.f, 0.f });
    m_text.setPosition(rect.position + sf::Vector2f{ PADDING, PADDING });
    window.draw(m_text, states);
    m_text.setString(m_wrapped);   // Its size stays the whole tip's
}

void UIHubMascot::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
    {
        m_isHovered = get_bounds().contains(window.mapPixelToCoords(moved->position));
    }
    else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (pressed->button != sf::Mouse::Button::Left) return;
        const sf::Vector2f point = window.mapPixelToCoords(pressed->position);
        if (get_bounds().contains(point))
        {
            // Tapped: a hop, a blub, and something else to say
            if (m_hopTime < 0.f || m_hopTime > HOP_SECONDS + 0.2f) m_hopTime = 0.f;
            play_sound("fish_blub", 1.1f);
            next_tip();
        }
        else if (m_tipTime >= 0.f && bubble_rect().contains(point))
        {
            const float typing = static_cast<float>(m_wrapped.size()) / CHARS_PER_SECOND;
            if (m_tipTime < typing) m_tipTime = typing;   // The whole tip at once
            else next_tip();
        }
    }
}

sf::FloatRect UIHubMascot::get_bounds() const
{
    return { HOME - FISH_BOX / 2.f - sf::Vector2f{ 0.f, 10.f }, FISH_BOX + sf::Vector2f{ 0.f, 20.f } };
}
