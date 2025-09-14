#pragma once

#include "UITextureElement.h"
#include <functional>

/*
 * UIEquipSlot
 * ------------
 * Represents one of the 10 slots (2x5 grid) where the player can
 * equip a unit. Displays the unit’s texture and cost.
 *
 * Features:
 * - Shows a unit sprite inside the slot.
 * - Displays cost text.
 * - Handles right-click unequip via callback.
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

    sf::Texture m_texture;
    sf::Sprite m_sprite;

    sf::RectangleShape m_shape; // Slot background
    sf::Text m_costText;        // Shows cost
    sf::Font m_font;

    std::function<void(int)> m_onUnequip; // Called when right-click unequip
};
