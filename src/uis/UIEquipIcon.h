#pragma once

#include "UITextureElement.h"
#include "tweeny.h"

/*
 * DragInfo
 * --------
 * Info passed when starting a drag from an icon.
 */
struct DragInfo
{
    int uid;                   // Unit ID
    int cost;                  // Deployment cost
    std::string texturePath;   // Path to icon texture
    sf::Vector2f iconSize;     // Size of the icon
};

/*
 * UIEquipIcon
 * ------------
 * Represents a single unit icon in the equip slider.
 *
 * Features:
 * - Displays unit texture, name, level, and cost.
 * - Detects mouse interaction for dragging.
 * - Animates movement and scaling when selected/unselected.
 */

class UIEquipIcon : public UITextureElement
{
public:

    // Constructor
    // iconTexturePath: path to icon texture
    // uid: unique ID of the unit
    // cost: deployment cost of the unit
    UIEquipIcon(const std::string& iconTexturePath, int uid, int cost);

    // Get the unit ID
    int get_uid() const;

    // Mark as selected (enlarges and highlights icon)
    void select(bool isSelected);

    // Set unit info
    void set_unit_name(const std::string& name);
    void set_unit_level(int level);
    void set_deployment_cost(int cost);

    // Standard UI loop
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

    // Detects drag start (fills out DragInfo if successful)
    bool check_drag_start(const sf::Event& event, const sf::RenderWindow& window, DragInfo& outDrag);

    // Override to maintain 4:3 aspect ratio
    void set_size(sf::Vector2f size) override;

public:

    // Tween animations for movement and scaling
    tweeny::tween<float> tweenX;
    tweeny::tween<float> tweenWidth;
    tweeny::tween<float> tweenHeight;

private:

    int m_uid = -1;            // Unit ID
    bool m_isSelected = false; // True if this icon is centered
    int m_deploymentCost = 100;
    int m_currentLevel = 1;

    // Text info
    sf::Font m_font;
    sf::Text m_unitName;
    sf::Text m_currentLevelText;
    sf::Text m_deploymentCostText;

    // Visuals
    sf::RectangleShape m_shape;       // Background frame
    sf::RectangleShape m_blackFilter; // Dark overlay when not selected

    // Layout constants
    const float TITLE_BASE_TEXT_SIZE = 35.f;
    const float INFO_BASE_TEXT_SIZE = 45.f;
    const float INACTIVE_TEXT_SIZE_SCALE = 1.0f;
    const float ACTIVE_TEXT_SIZE_SCALE = 1.4f;

    std::string m_texturePath;        // Path to texture
};
