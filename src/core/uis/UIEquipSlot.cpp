#include "UIEquipSlot.h"
#include <iostream>

UIEquipSlot::UIEquipSlot(int uid, int cost, const std::string& texturePath, int slotIndex) :
    m_uid(uid), m_cost(cost), m_slotIndex(slotIndex),
    m_sprite(m_texture), m_costText(m_font)
{
    // Texture
    if (!m_texture.loadFromFile(texturePath))
    {
        std::cerr << "Failed to load slot texture: " << texturePath << "\n";
        m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
    }
    m_sprite.setTexture(m_texture);

    // Shape
    m_shape.setFillColor(sf::Color(255, 225, 145));
    m_shape.setOutlineColor(sf::Color::Black);
    m_shape.setOutlineThickness(3.f);

    // Font & Cost text
    if (!m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf"))
    {
        std::cerr << "Failed to load font for UIEquipSlot\n";
    }

    m_costText.setFont(m_font);
    m_costText.setString(std::to_string(m_cost) + "$");
    m_costText.setCharacterSize(20);
    m_costText.setFillColor(sf::Color::Yellow);
    m_costText.setOutlineColor(sf::Color::Black);
    m_costText.setOutlineThickness(2.f);
}

void UIEquipSlot::set_position(const sf::Vector2f& pos)
{
    m_shape.setPosition(pos);
    m_sprite.setPosition(pos);

    auto bounds = m_shape.getGlobalBounds();
    m_costText.setPosition({ bounds.position.x + bounds.size.x - 45.f, bounds.position.x + bounds.size.y - 25.f });
}

void UIEquipSlot::set_size(const sf::Vector2f& size)
{
    m_shape.setSize(size);

    // Scale sprite to fit
    sf::FloatRect spriteBounds = m_sprite.getLocalBounds();
    m_sprite.setScale({
        size.x / spriteBounds.size.x,
        size.y / spriteBounds.size.y
        });
}

int UIEquipSlot::get_uid() const
{
    return m_uid;
}

int UIEquipSlot::get_slot_index() const
{
    return m_slotIndex;
}

void UIEquipSlot::update(float deltaTime)
{
    // Animations possibles plus tard
}

void UIEquipSlot::render(sf::RenderWindow& window)
{
    window.draw(m_shape);
    window.draw(m_sprite);
    window.draw(m_costText);
}

void UIEquipSlot::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Right)
        {
            sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position);
            if (m_shape.getGlobalBounds().contains(mousePos))
            {
                if (m_onUnequip)
                    m_onUnequip(m_slotIndex);
            }
        }
    }
}

void UIEquipSlot::set_callback(std::function<void(int)> callback)
{
    m_onUnequip = std::move(callback);
}
