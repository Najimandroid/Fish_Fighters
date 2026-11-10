#pragma once

#include "UIElement.h"
#include "../../data/AbilityData.h"

#include <vector>

/*
 * UIAbilityList
 * -------------
 * The abilities of a unit or an enemy, one line each, like The Battle Cats' ability text:
 *   [icon] Strong against [type icon] Jelly
 * The ability is written in its own color and every target type in its type's color.
 * Hovering a line (or clicking it, which keeps it open) shows what the ability does with its numbers
 * (AbilityGlossary), in a bubble above the line.
 * Its position is its top left corner; get_height() says how tall it ended up.
 *
 * Example usage:
 *   m_abilities.set_abilities(unitData->abilities);
 *   m_abilities.set_position({ 620.f, 820.f });
 */

class UIAbilityList : public UIElement
{
public:

    UIAbilityList();

    void set_abilities(const std::vector<AbilityData>& abilities);
    bool is_empty() const;
    float get_height() const;

    void update(float deltaTime) override {}

    // Lines, then the bubble of the hovered (or clicked) line on top
    void render(sf::RenderWindow& window) override;

    // Mouse moves (hover) and left clicks (keep the bubble open / close it)
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // Color of a type's name (crustacean orange, shelled purple, jelly pink, fish blue)
    static sf::Color type_color(EntityType type);

    // Color of an ability's label
    static sf::Color ability_color(AbilityType type);

    static constexpr float LINE_HEIGHT = 48.f;

private:

    struct Line
    {
        AbilityData ability;
        std::string explanation;
        sf::FloatRect bounds;   // Screen area of the line (set when drawn)
    };

    // Draws one line from (x, y) and returns its right end
    float render_line(sf::RenderWindow& window, Line& line, sf::Vector2f topLeft);
    void render_bubble(sf::RenderWindow& window, const Line& line);

    int line_at(sf::Vector2f point) const;

    std::vector<Line> m_lines;
    int m_hoveredLine = -1;
    int m_pinnedLine = -1;
    sf::Text m_text;           // Reused for every piece of text
    sf::Text m_bubbleText;
};
