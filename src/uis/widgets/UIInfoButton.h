#pragma once

#include "UIButtonElement.h"

/*
 * UIInfoButton
 * ------------
 * A small round "i" that explains a screen or a feature: its callback opens a UIInfoPanel. Bigger on a phone, and
 * its touch area reaches past its circle there (a finger is bigger than a mouse pointer).
 *
 * Example usage:
 *   auto info = std::make_shared<UIInfoButton>(titleBar->get_info_position());
 *   info->set_callback([weakPanel]() { if (auto panel = weakPanel.lock()) panel->open(); });
 */
class UIInfoButton : public UIButtonElement
{
public:
    explicit UIInfoButton(sf::Vector2f center);

    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override;
};
