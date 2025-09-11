#pragma once

#include "UITextureElement.h"
#include <functional>

class UIEquipSlot : public UIElement
{
public:
    UIEquipSlot(int uid, int cost, const std::string& texturePath, int slotIndex);

    void set_position(const sf::Vector2f& pos);
    void set_size(const sf::Vector2f& size);

    int get_uid() const;
    int get_slot_index() const;

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // Nouveau : définir callback quand une unit est déséquipée
    void set_callback(std::function<void(int)> callback);

private:
    int m_uid = -1;
    int m_cost = 0;
    int m_slotIndex = -1;

    sf::Texture m_texture;
    sf::Sprite m_sprite;

    sf::RectangleShape m_shape;
    sf::Text m_costText;
    sf::Font m_font;

    std::function<void(int)> m_onUnequip;
};
