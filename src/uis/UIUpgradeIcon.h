#pragma once

#include "UITextureElement.h"
#include "tweeny.h"

/*
 * UIUpgradeIcon
 * -------------
 * Represents a unit's icon in the upgrade slider.
 * Displays the unit image, current level, upgrade cost, and name.
 * Supports selection highlighting and smooth tween animations for position and size.
 */

class UIUpgradeIcon : public UITextureElement
{
public:

    // Constructor: initializes the icon with a texture path
    UIUpgradeIcon(const std::string& iconTexturePath);

    // Returns the unique ID of the unit associated with this icon
    int get_uid() const;

    // Selects or deselects the icon (affects outline and text size)
    void select(bool isSelected);

    // Sets the displayed unit name
    void set_unit_name(const std::string& name);

    // Sets the current level of the unit
    void set_unit_level(int level);

    // Sets the upgrade cost displayed on the icon
    void set_upgrade_cost(int cost);

    // Updates the icon state each frame (handles tween animations)
    void update(float deltaTime) override;

    // Renders the icon, including sprite, texts, and overlays
    void render(sf::RenderWindow& window) override;

    // Overrides base method to scale sprite, shape, and overlay
    void set_size(sf::Vector2f size) override;

public:

    // Tween objects
    tweeny::tween<float> tweenX;       // Horizontal movement tween
    tweeny::tween<float> tweenWidth;   // Width tween
    tweeny::tween<float> tweenHeight;  // Height tween

private:

    int m_uid = -1;           // Unique ID of the unit
    bool m_isSelected = false; // Whether this icon is currently selected

    int m_upgradeCost = 100;  // Cost to upgrade the unit
    int m_currentLevel = 1;   // Current unit level

    sf::Font m_font;          // Font used for texts
    sf::Text m_unitName;
    sf::Text m_currentLevelText; // Text showing current unit level
    sf::Text m_upgradeCostText;  // Text showing upgrade cost

    sf::RectangleShape m_shape;     // Background rectangle for the icon
    sf::RectangleShape m_blackFilter; // Overlay for unselected/darkened state

    // Text size constants
    const float TITLE_BASE_TEXT_SIZE = 35.f; // Unit name base size
    const float INFO_BASE_TEXT_SIZE = 45.f;  // Level and cost text base size
    const float INACTIVE_TEXT_SIZE_SCALE = 1.0f; // Scale for unselected icon
    const float ACTIVE_TEXT_SIZE_SCALE = 1.4f;   // Scale for selected icon
};
