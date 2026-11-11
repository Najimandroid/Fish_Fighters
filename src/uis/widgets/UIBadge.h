#pragma once

#include "UIElement.h"

#include <string>

/*
 * UIBadge
 * -------
 * A small round coral badge with a short text ("!", "NEW", a number), pinned to the corner of a button to
 * say that something waits there. It pulses gently. Nothing to click (the button under it gets the clicks).
 */
class UIBadge : public UIElement
{
public:
    // center: the badge's center on screen
    UIBadge(sf::Vector2f center, const std::string& text);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }

private:
    sf::Vector2f m_center;
    sf::Text m_text;
    float m_time = 0.f;
};
