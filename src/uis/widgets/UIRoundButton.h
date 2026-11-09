#pragma once

#include "UIButtonElement.h"

/*
 * UIRoundButton
 * -------------
 * A round yellow button with a white icon (pause, speed...), like the battle buttons of The Battle Cats.
 * It can light up with a ring while an option is on (the speed button while the battle runs faster).
 *
 * Example usage:
 *   auto speedButton = std::make_shared<UIRoundButton>(100.f, sf::Vector2f{ 140.f, 24.f }, "icon_speed");
 *   speedButton->set_active_callback([this]() { return is_faster(); });
 */

class UIRoundButton : public UIButtonElement
{
public:

    // diameter: size on screen, iconName: texture of assets/images/textures/ui ("icon_pause")
    UIRoundButton(float diameter, sf::Vector2f position, const std::string& iconName);

    // Changes the icon
    void set_icon(const std::string& iconName);

    // The ring is shown while this returns true
    void set_active_callback(std::function<bool()> isActive);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:

    sf::Sprite m_button;                 // Round yellow image
    sf::Sprite m_icon;                   // White icon over it
    sf::CircleShape m_ring;              // Shown while the option is on
    std::function<bool()> m_isActive;
    bool m_isActiveNow = false;
};
