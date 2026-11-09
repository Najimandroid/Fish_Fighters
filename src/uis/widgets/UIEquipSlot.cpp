#include "UIEquipSlot.h"
#include "../UISkin.h"

#include <iostream>

namespace
{
    const float SLOT_WIDTH = 166.f;         // Same slots as the deploy slots in battle (UIBattleIcon)
    const unsigned int COST_SIZE = 32;
    const sf::Vector2f COST_PADDING{ 12.f, 10.f };
}

/*
 * Initializes a lineup slot: loads the icon (placeholder if missing) and the cost text
 */
UIEquipSlot::UIEquipSlot(int uid, int cost, const std::string& texturePath, int slotIndex) :
    UIElement({ SLOT_WIDTH, 124.f }, { 0.f, 0.f }),
    m_uid(uid), m_cost(cost), m_slotIndex(slotIndex),
    m_costText(UISkin::heavy_font(), std::to_string(cost) + "$")
{
    if (!m_texture.loadFromFile(texturePath))
    {
        std::cerr << "Failed to load slot texture: " << texturePath << "\n";
        if (!m_texture.loadFromFile("assets/images/textures/icons/placeholder.png"))
            std::cerr << "Failed to load placeholder.png\n";
    }
    m_texture.setSmooth(true);

    UISkin::style_text(m_costText, COST_SIZE, UISkin::NUMBER);
}

void UIEquipSlot::set_position(const sf::Vector2f& position)
{
    m_position = position;
}

void UIEquipSlot::set_size(const sf::Vector2f& size)
{
    m_size = size;
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
}

/*
 * Slot, icon cropped into it, frame over the icon edges, cost at the bottom right
 */
void UIEquipSlot::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const sf::FloatRect rect = get_bounds();
    const float scale = rect.size.x / SLOT_WIDTH;

    UISkin::draw_nine_slice(window, UISkin::SLOT, rect, scale);
    UISkin::draw_cropped(window, m_texture, UISkin::slot_inner(rect, scale));
    UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, rect, scale);

    UISkin::set_text_anchor(m_costText, { 1.f, 1.f });
    m_costText.setScale({ scale, scale });
    m_costText.setPosition(rect.position + rect.size - COST_PADDING * scale);
    window.draw(m_costText);
}

/*
 * Right-click inside the slot calls the unequip callback (if set)
 */
void UIEquipSlot::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Right)
        {
            sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position);
            if (get_bounds().contains(mousePos))
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
