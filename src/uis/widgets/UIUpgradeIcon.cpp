#include "UIUpgradeIcon.h"
#include "../UISkin.h"

UIUpgradeIcon::UIUpgradeIcon(const std::string& iconTexturePath) :
    UIUnitCard(iconTexturePath)
{
}

int UIUpgradeIcon::get_uid() const
{
    return m_uid;
}

void UIUpgradeIcon::set_unit_level(int level)
{
    m_currentLevel = level;
}

void UIUpgradeIcon::set_extra_levels(int extraLevels)
{
    m_extraLevels = extraLevels;
}

void UIUpgradeIcon::set_upgrade_cost(int cost)
{
    m_upgradeCost = cost;
}

void UIUpgradeIcon::set_affordable(bool isAffordable)
{
    m_isAffordable = isAffordable;
}

void UIUpgradeIcon::set_in_lineup(bool isInLineup)
{
    set_badge(isInLineup ? "IN USE" : "");
}

void UIUpgradeIcon::set_max_level(int maxLevel)
{
    m_maxLevel = maxLevel;
}

void UIUpgradeIcon::set_caption(const std::string& caption)
{
    m_caption = caption;
}

/*
 * Plays the tweens, then writes the info lines: level (or LOCKED) and the price of the next level (or MAX)
 */
void UIUpgradeIcon::update(float deltaTime)
{
    UIUnitCard::update(deltaTime);

    if (!m_caption.empty())
    {
        set_locked(false);
        set_level_line(m_caption, UISkin::TITLE);
        set_level_extra("");
        set_value_line("", UISkin::TEXT, false);
        return;
    }

    const bool isLocked = m_currentLevel <= 0;
    set_locked(isLocked);
    set_level_line(isLocked ? "LOCKED" : "LEVEL " + std::to_string(m_currentLevel), isLocked ? UISkin::TEXT : UISkin::NUMBER);
    set_level_extra(!isLocked && m_extraLevels > 0 ? "+" + std::to_string(m_extraLevels) : "");

    if (m_currentLevel >= m_maxLevel)
        set_value_line("MAX", UISkin::TEXT, false);
    else
        set_value_line(std::to_string(m_upgradeCost), m_isAffordable ? UISkin::NUMBER : UISkin::NUMBER_DISABLED, true);
}
