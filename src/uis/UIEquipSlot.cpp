#include "UIEquipSlot.h"
#include <iostream>

/*
 * Constructor
 * -----------
 * Initializes a UI slot for an equipped unit.
 * - Loads texture (falls back to placeholder if missing)
 * - Configures slot shape appearance
 * - Loads font and prepares cost text
 */
UIEquipSlot::UIEquipSlot(int uid, int cost, const std::string& texturePath, int slotIndex) :
    m_uid(uid), m_cost(cost), m_slotIndex(slotIndex),
    m_sprite(m_texture), m_costText(m_font)
{
    m_shape.setSize({ 144.f, 108.f });
    m_shape.setFillColor(sf::Color(255, 225, 145));
    m_shape.setOutlineColor(sf::Color::Black);
    m_shape.setOutlineThickness(3.f);


    if (!m_texture.loadFromFile(texturePath))
    {
        std::cerr << "Failed to load slot texture: " << texturePath << "\n";
        m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
    }
    m_sprite.setTexture(m_texture);

    m_shape.setFillColor(sf::Color(255, 225, 145));
    m_shape.setOutlineColor(sf::Color::Black);
    m_shape.setOutlineThickness(3.f);

    if (!m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf"))
    {
        std::cerr << "Failed to load font for UIEquipSlot\n";
    }

    m_costText.setFont(m_font);
    m_costText.setString(std::to_string(m_cost) + "$");
    m_costText.setCharacterSize(30);
    m_costText.setFillColor(sf::Color::Yellow);
    m_costText.setOutlineColor(sf::Color::Black);
    m_costText.setOutlineThickness(2.f);
}

/*
 * set_position
 * ------------
 * Sets the position of the slot and aligns its contents
 * (shape, sprite, and cost text).
 */
void UIEquipSlot::set_position(const sf::Vector2f& position)
{
    m_shape.setPosition(position);

    // Align sprite with shape center
    m_sprite.setPosition(position);

    // Cost text bottom-right
    auto textBounds = m_costText.getLocalBounds();
    m_costText.setOrigin({
        textBounds.position.x + textBounds.size.x,
        textBounds.position.y + textBounds.size.y
    });

    float paddingX = -22.f;
    float paddingY = -18.f;

    m_costText.setPosition({
        m_shape.getPosition().x + m_shape.getSize().x - paddingX,
        m_shape.getPosition().y + m_shape.getSize().y - paddingY
    });
}

/*
 * set_size
 * --------
 * Sets the size of the slot shape and scales the sprite to fit inside.
 */
void UIEquipSlot::set_size(const sf::Vector2f& size)
{
    m_shape.setSize(size);

    m_sprite.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y) }));

    // Scale sprite proportionally to fit slot
    sf::FloatRect spriteBounds = m_sprite.getLocalBounds();
    if (spriteBounds.size.x > 0 && spriteBounds.size.y > 0)
    {
        float sx = size.x / spriteBounds.size.x;
        float sy = size.y / spriteBounds.size.y;

        float uniformScale = std::min(sx, sy);
        m_sprite.setScale({ uniformScale, uniformScale });
    }
}

/*
 * get_uid
 * -------
 * Returns the unit ID stored in this slot.
 */
int UIEquipSlot::get_uid() const
{
    return m_uid;
}

/*
 * get_slot_index
 * --------------
 * Returns the index of this slot in the equip grid.
 */
int UIEquipSlot::get_slot_index() const
{
    return m_slotIndex;
}

/*
 * update
 * ------
 * Currently does nothing, but reserved for animations or effects.
 */
void UIEquipSlot::update(float deltaTime)
{
    // Animations could be added later
}

/*
 * render
 * ------
 * Draws the slot shape, unit sprite, and cost text to the screen.
 */
void UIEquipSlot::render(sf::RenderWindow& window)
{
    window.draw(m_shape);
    window.draw(m_sprite);
    window.draw(m_costText);
}

/*
 * handle_event
 * ------------
 * Listens for mouse events.
 * - If right-clicked inside the slot, calls the unequip callback (if set).
 */
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

/*
 * set_callback
 * ------------
 * Sets the callback function to call when the unit in this slot is unequipped.
 */
void UIEquipSlot::set_callback(std::function<void(int)> callback)
{
    m_onUnequip = std::move(callback);
}
