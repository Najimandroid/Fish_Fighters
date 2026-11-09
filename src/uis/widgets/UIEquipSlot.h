#pragma once

#include "UIElement.h"
#include <functional>

/*
 * UIEquipSlot
 * -----------
 * A unit placed in the lineup of the Equip screen, drawn like a deploy slot in battle
 * (light blue slot, the icon under a rounded frame, the deploy cost in yellow).
 * Right-click removes it from the lineup.
 */

class UIEquipSlot : public UIElement
{
public:

    // Constructor
    // uid: unit ID
    // cost: deployment cost
    // texturePath: path to icon texture
    // slotIndex: position in the 2x5 grid
    UIEquipSlot(int uid, int cost, const std::string& texturePath, int slotIndex);

    // Position and size
    void set_position(const sf::Vector2f& position);
    void set_size(const sf::Vector2f& size);

    // Getters
    int get_uid() const;
    int get_slot_index() const;

    // Standard UI loop
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // Register callback when unequipping
    void set_callback(std::function<void(int)> callback);

private:

    int m_uid = -1;          // Unit ID
    int m_cost = 0;          // Deployment cost
    int m_slotIndex = -1;    // Slot position index

    sf::Texture m_texture;   // Unit icon
    sf::Text m_costText;     // Shows cost

    std::function<void(int)> m_onUnequip; // Called when right-click unequip
};
