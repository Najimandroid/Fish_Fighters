#include "UIEquipIcon.h"
#include "../UISkin.h"

UIEquipIcon::UIEquipIcon(const std::string& iconTexturePath, int uid, int cost) :
    UIUnitCard(iconTexturePath),
    m_uid(uid),
    m_deploymentCost(cost),
    m_texturePath(iconTexturePath)
{
}

int UIEquipIcon::get_uid() const
{
    return m_uid;
}

void UIEquipIcon::set_unit_level(int level)
{
    m_currentLevel = level;
}

void UIEquipIcon::set_deployment_cost(int cost)
{
    m_deploymentCost = cost;
}

void UIEquipIcon::set_extra_levels(int extraLevels)
{
    m_extraLevels = extraLevels;
}

void UIEquipIcon::set_in_lineup(bool isInLineup)
{
    set_badge(isInLineup ? "IN USE" : "");
}

/*
 * Plays the tweens, then writes the info lines: level and deploy cost
 */
void UIEquipIcon::update(float deltaTime)
{
    UIUnitCard::update(deltaTime);

    set_level_line("LEVEL " + std::to_string(m_currentLevel), UISkin::NUMBER);
    set_level_extra(m_extraLevels > 0 ? "+" + std::to_string(m_extraLevels) : "");
    set_value_line(std::to_string(m_deploymentCost) + "$", UISkin::NUMBER, false);
}

/*
 * Checks if left mouse button was pressed on this icon
 * If so, fills DragInfo with unit data (uid, cost, texturePath) and returns true
 */
bool UIEquipIcon::check_drag_start(const sf::Event& event, const sf::RenderWindow& window, DragInfo& outDrag)
{
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Left &&
            get_bounds().contains(window.mapPixelToCoords(mousePressed->position)))
        {
            outDrag.uid = m_uid;
            outDrag.cost = m_deploymentCost;
            outDrag.texturePath = m_texturePath;
            return true;
        }
    }
    return false;
}
