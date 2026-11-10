#pragma once

#include "UIButtonElement.h"
#include <string>

/*
 * UIBattleIcon
 * ------------
 * Represents a clickable icon in the deployment UI for spawning units during a stage.
 * - Inherits from UIButtonElement for click handling
 * - Drawn like a Battle Cats deploy slot: light blue slot, the unit icon under a rounded frame,
 *   the cost in yellow outlined digits, a cyan bar while it cools down, darkened when it can't be used
 */

class UIBattleIcon : public UIButtonElement
{
public:

    // Size of a slot on screen
    static constexpr float WIDTH = 166.f;
    static constexpr float HEIGHT = 124.f;

    // Constructor
    // position: position of the icon on the screen
    // iconTexturePath: path to the unit's icon texture
    UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath);

    // Updates cooldown progress and price text
    void update(float deltaTime) override;

    // Draws the slot, unit icon, shade, cooldown bar, frame and price
    void render(sf::RenderWindow& window) override;

    // Getters
    int get_uid() const;                // returns the unit ID
    bool get_if_on_cooldown() const;    // returns true if icon is cooling down
    int get_cost() const;               // returns the unit cost

    // Starts the cooldown timer and visual overlay
    void start_cooldown();

    // Setters
    void set_uid(int uid);
    void set_max_cooldown(float cooldown);
    void set_cost(int cost);
    void set_texture(const std::string& texturePath);
    void set_darkened(bool isDarkened); // visually darkens the icon when unavailable

    // How much of the cost the wallet holds (0 to 1): a "water level" rises in the slot until it's affordable
    void set_affordability(float fraction);

    // The key shown in the corner (its hotkey), "" for none
    void set_key_label(const std::string& label);

    // A short shake: the click was refused (cooldown, cash, unit limit)
    void shake();

private:

    // Unit info
    int m_uid = -1;           // Unique ID of the unit
    int m_cost = 50;          // Cost to deploy the unit
    bool m_isOnCooldown = false;
    float m_currentCooldown = 0.0f;
    float m_maxCooldown = 1.0f;
    bool m_isDarkened = false;
    float m_affordability = 1.f;

    // Feedback
    bool m_wasReady = true;   // Deployable last frame (the first frame doesn't flash)
    bool m_isFirstUpdate = true;
    float m_readyTime = -1.f; // Seconds since it became deployable (-1: no flash running)
    float m_shakeTime = -1.f; // Seconds since a refused click (-1: no shake)

    // Visuals
    sf::Texture m_texture;    // Unit icon texture (drawn cropped into the slot)
    sf::Text m_keyText;       // The hotkey, in a navy chip
    sf::Text m_secondsText;   // Cooldown seconds left
};
