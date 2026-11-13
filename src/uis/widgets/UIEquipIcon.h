#pragma once

#include "UIUnitCard.h"

/*
 * Data describing a unit being dragged from the equip carousel
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
 * -----------
 * A unit's card in the equip carousel (UIUnitCard): "LEVEL n", its deploy cost, and "IN USE"
 * when it's already in the lineup. The selected card can be dragged into a lineup slot.
 */

class UIEquipIcon : public UIUnitCard
{
public:

    // Constructor
    // iconTexturePath: path to icon texture
    // uid: unique ID of the unit
    // cost: deployment cost of the unit
    UIEquipIcon(const std::string& iconTexturePath, int uid, int cost);

    // Get the unit ID
    int get_uid() const;

    // Set unit info
    void set_unit_level(int level);
    void set_extra_levels(int extraLevels);   // The Clam Room's "+3" after the level
    void set_deployment_cost(int cost);
    void set_in_lineup(bool isInLineup);

    void update(float deltaTime) override;

    // Detects drag start (fills out DragInfo if successful)
    bool check_drag_start(const sf::Event& event, const sf::RenderWindow& window, DragInfo& outDrag);

private:

    int m_uid = -1;            // Unit ID
    int m_deploymentCost = 100;
    int m_currentLevel = 1;
    int m_extraLevels = 0;
    std::string m_texturePath; // Path to texture
};
