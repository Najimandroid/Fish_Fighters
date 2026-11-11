#include "UIAbilityList.h"
#include "../UISkin.h"
#include "../../core/combat/AbilityGlossary.h"

#include <algorithm>

namespace
{
    const unsigned int TEXT_SIZE = 30;
    const unsigned int BUBBLE_TEXT_SIZE = 26;
    const float ABILITY_ICON_SIZE = 42.f;
    const float TYPE_ICON_SIZE = 34.f;
    const float GAP = 10.f;
    const float BUBBLE_MAX_WIDTH = 620.f;
    const float BUBBLE_PADDING = 18.f;
}

UIAbilityList::UIAbilityList() :
    m_text(UISkin::heavy_font()),
    m_bubbleText(UISkin::regular_font())
{
    UISkin::style_text(m_text, TEXT_SIZE);
    UISkin::style_body_text(m_bubbleText, BUBBLE_TEXT_SIZE);
    m_bubbleText.setLineSpacing(1.1f);
}

void UIAbilityList::set_abilities(const std::vector<AbilityData>& abilities)
{
    m_lines.clear();
    m_hoveredLine = -1;
    m_pinnedLine = -1;

    for (const AbilityData& ability : abilities)
    {
        if (ability.type == AbilityType::NONE) continue;
        m_lines.push_back({ ability, AbilityGlossary::ability_explanation(ability), {} });
    }
    m_size = { m_size.x, get_height() };
}

bool UIAbilityList::is_empty() const
{
    return m_lines.empty();
}

float UIAbilityList::get_height() const
{
    return static_cast<float>(m_lines.size()) * LINE_HEIGHT;
}

sf::Color UIAbilityList::type_color(EntityType type)
{
    switch (type)
    {
    case EntityType::CRUSTACEAN: return { 255, 150, 80 };
    case EntityType::SHELLED:    return { 196, 140, 255 };
    case EntityType::JELLY:      return { 255, 130, 214 };
    case EntityType::FISH:       return { 120, 210, 255 };
    default:                     return UISkin::TEXT;
    }
}

sf::Color UIAbilityList::ability_color(AbilityType type)
{
    switch (type)
    {
    case AbilityType::STRONG_AGAINST:    return { 255, 110, 90 };
    case AbilityType::RESISTANT_AGAINST: return { 120, 185, 255 };
    case AbilityType::WEAKEN_ATTACK:     return { 210, 160, 255 };
    case AbilityType::PARALYZE_ATTACK:   return { 255, 226, 80 };
    case AbilityType::SLOW_ATTACK:       return { 130, 230, 255 };
    default:                             return UISkin::TEXT;
    }
}

float UIAbilityList::render_line(sf::RenderWindow& window, Line& line, sf::Vector2f topLeft)
{
    const float centerY = topLeft.y + LINE_HEIGHT / 2.f;
    float x = topLeft.x;

    // Ability icon
    const int abilityIcon = AbilityGlossary::ability_icon_index(line.ability.type);
    if (abilityIcon >= 0)
    {
        const sf::Texture& sheet = UISkin::texture_file(AbilityGlossary::ABILITY_ICONS_TEXTURE);
        UISkin::draw_fitted(window, sheet, UISkin::icon_cell(sheet, abilityIcon, AbilityGlossary::ICON_CELL_SIZE),
            { { x, centerY - ABILITY_ICON_SIZE / 2.f }, { ABILITY_ICON_SIZE, ABILITY_ICON_SIZE } });
        x += ABILITY_ICON_SIZE + GAP;
    }

    auto drawText = [&](const std::string& text, sf::Color color)
        {
            m_text.setString(text);
            UISkin::set_text_color(m_text, color);
            UISkin::set_text_anchor(m_text, { 0.f, 0.5f });
            m_text.setPosition({ x, centerY });
            window.draw(m_text);
            x += m_text.getLocalBounds().size.x + m_text.getOutlineThickness() * 2.f;
        };

    drawText(AbilityGlossary::ability_label(line.ability.type), ability_color(line.ability.type));

    // Target types: icon and name in the type's color, separated by commas
    const sf::Texture& typeSheet = UISkin::texture_file(AbilityGlossary::TYPE_ICONS_TEXTURE);
    for (size_t i = 0; i < line.ability.targetTypes.size(); ++i)
    {
        const EntityType type = line.ability.targetTypes[i];
        x += GAP;
        if (type != EntityType::FISH)
        {
            UISkin::draw_fitted(window, typeSheet, UISkin::icon_cell(typeSheet, AbilityGlossary::type_icon_index(type), AbilityGlossary::ICON_CELL_SIZE),
                { { x, centerY - TYPE_ICON_SIZE / 2.f }, { TYPE_ICON_SIZE, TYPE_ICON_SIZE } });
            x += TYPE_ICON_SIZE + GAP / 2.f;
        }
        drawText(AbilityGlossary::type_name(type), type_color(type));
        if (i + 1 < line.ability.targetTypes.size()) drawText(",", UISkin::TEXT);
    }

    line.bounds = { topLeft, { x - topLeft.x, LINE_HEIGHT } };
    return x;
}

void UIAbilityList::render_bubble(sf::RenderWindow& window, const Line& line)
{
    m_bubbleText.setString(UISkin::wrap_text(line.explanation, BUBBLE_TEXT_SIZE, BUBBLE_MAX_WIDTH, UISkin::regular_font()));
    UISkin::set_text_anchor(m_bubbleText, { 0.f, 0.f });
    const sf::FloatRect textBounds = m_bubbleText.getLocalBounds();
    const sf::Vector2f size{ textBounds.size.x + 2.f * BUBBLE_PADDING, textBounds.size.y + 2.f * BUBBLE_PADDING };

    // Above the line, kept on the screen
    sf::Vector2f position{ line.bounds.position.x, line.bounds.position.y - size.y - 6.f };
    position.x = std::clamp(position.x, 10.f, 1910.f - size.x);
    if (position.y < 10.f) position.y = line.bounds.position.y + line.bounds.size.y + 6.f;

    const sf::FloatRect bubble{ position, size };
    const float border = 5.f;
    UISkin::draw_nine_slice(window, UISkin::BUTTON_YELLOW, { position - sf::Vector2f{ border, border }, size + sf::Vector2f{ border, border } * 2.f }, 0.4f);
    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, bubble, 0.6f);
    m_bubbleText.setPosition(position + sf::Vector2f{ BUBBLE_PADDING, BUBBLE_PADDING });
    window.draw(m_bubbleText);
}

void UIAbilityList::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    for (size_t i = 0; i < m_lines.size(); ++i)
        render_line(window, m_lines[i], { m_position.x, m_position.y + static_cast<float>(i) * LINE_HEIGHT });

    const int shown = m_pinnedLine >= 0 ? m_pinnedLine : m_hoveredLine;
    if (shown >= 0 && shown < static_cast<int>(m_lines.size()))
        render_bubble(window, m_lines[shown]);
}

int UIAbilityList::line_at(sf::Vector2f point) const
{
    for (size_t i = 0; i < m_lines.size(); ++i)
        if (m_lines[i].bounds.contains(point)) return static_cast<int>(i);
    return -1;
}

void UIAbilityList::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        m_hoveredLine = line_at(window.mapPixelToCoords(mouseMoved->position));
    }
    else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button != sf::Mouse::Button::Left) return;
        const int line = line_at(window.mapPixelToCoords(mousePressed->position));
        m_pinnedLine = (line >= 0 && line != m_pinnedLine) ? line : -1;
    }
}
