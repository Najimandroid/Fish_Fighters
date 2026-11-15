#pragma once

#include "UIElement.h"

#include <string>

/*
 * UICallout
 * ---------
 * A little yellow speech pill pointing at a button, like the hub callouts of The Battle Cats ("Lucky Capsules
 * Available Now!"): dark outlined, its tail on the button, bobbing gently; it pops in. Nothing to click.
 *
 * Example usage:
 *   add_ui_element(std::make_shared<UICallout>("New fish to unlock!", sf::Vector2f{ 650.f, 420.f }, UICallout::Tail::LEFT));
 */
class UICallout : public UIElement
{
public:
    // Where its tail points from: LEFT (the pill right of the tip), DOWN (the pill above it)
    enum class Tail { LEFT, DOWN };

    // tip: the tail's point, on the button's edge
    UICallout(const std::string& text, sf::Vector2f tip, Tail tail);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }   // Nothing to click: the button under it gets them

private:
    sf::Text m_text;
    sf::Vector2f m_tip;
    Tail m_tail;
    float m_time = 0.f;
};
