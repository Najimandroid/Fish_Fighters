#include "UIDeploymentIcons.h"
#include "../core/stages/Stage.h"

#include <iostream>

/*
 * Stores references to the DataLoader and Stage, then initializes the icons grid.
 */
UIDeploymentIcons::UIDeploymentIcons(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage) :
    m_dataLoader(dataLoader), m_stage(stage)
{
    init_icons();
}

/*
 * Creates a grid of icons for unit deployment.
 * - Arranges icons in 2 rows and 5 columns
 * - Reads equipped units from player data
 * - Sets textures, cooldowns, costs, and click callbacks for each icon
 */
void UIDeploymentIcons::init_icons()
{
    const int rows = 2;
    const int cols = 5;

    const float iconWidth = 144.f * 1.15f;
    const float iconHeight = 108.f * 1.15f;
    const float spacingX = 24.f * 1.15f;
    const float spacingY = 24.f * 1.15f;

    float totalWidth = cols * iconWidth + (cols - 1) * spacingX;
    float startX = (1920.f - totalWidth) / 2.f; // center horizontally
    float startY = 775.f; // vertical position for icons

    int index = 0;

    std::array<int, 10> equippedUnits;
    if (auto player = m_dataLoader->get_player_data().lock())
        equippedUnits = player->equippedUnits; // retrieve equipped unit IDs
    else
        equippedUnits.fill(-1); // no units equipped

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            sf::Vector2f pos{
                startX + col * (iconWidth + spacingX),
                startY + row * (iconHeight + spacingY)
            };

            auto icon = std::make_shared<UIBattleIcon>(pos, "assets/images/textures/icons/empty.png");

            int uid = equippedUnits[index];
            int form = m_dataLoader->get_player_data().lock()->get_unit_form(uid);

            if (uid >= 0)
            {
                // Load the unit's texture and data
                std::string texturePath = m_dataLoader->get_unit_icon_texture_path(uid, form);
                std::shared_ptr<EntityData> unitData = m_dataLoader->get_unit_data(uid, form);

                icon->set_texture(texturePath);
                icon->set_uid(uid);
                icon->set_cost(unitData->cost);
                icon->set_max_cooldown(unitData->cooldown);

                std::weak_ptr<EntityData> weakData = unitData;

                // Set callback to spawn unit when icon is clicked
                icon->set_callback([this, icon, weakData]()
                    {
                        if (icon->get_if_on_cooldown()) return;

                        if (auto data = weakData.lock())
                        {
                            if (m_stage.lock()->get_cash() < data->cost) return;
                            m_stage.lock()->spawn_unit(data);
                            icon->start_cooldown();
                        }
                    });
            }
            else
            {
                // No unit equipped in this slot
                icon->set_uid(-1);
                icon->set_cost(-1);
                icon->set_max_cooldown(-1);
            }

            m_icons[index] = icon;
            index++;
        }
    }
}

/*
 * Updates each icon's state:
 * - Darkens icons if unit cannot be deployed (insufficient cash, cooldown, or empty slot)
 * - Updates cooldown timers
 */
void UIDeploymentIcons::update(float deltaTime)
{
    for (auto& icon : m_icons)
    {
        if (m_stage.lock()->get_cash() < icon->get_cost() || icon->get_if_on_cooldown() || icon->get_uid() < 0)
        {
            icon->set_darkened(true);
        }
        else
        {
            icon->set_darkened(false);
        }

        icon->update(deltaTime);
    }
}

/*
 * Draws all icons to the window if visible.
 */
void UIDeploymentIcons::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    for (auto& icon : m_icons)
    {
        icon->render(window);
    }
}

/*
 * Delegates input events to each icon for interaction handling.
 */
void UIDeploymentIcons::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    for (auto& icon : m_icons)
    {
        icon->handle_event(event, window);
    }
}

/*
 * Returns the bounding rectangle that encompasses all icons.
 * Useful for detecting if the mouse is over the deployment area.
 */
sf::FloatRect UIDeploymentIcons::get_bounds() const
{
    bool first = true;
    float minX = 0.f, minY = 0.f;
    float maxX = 0.f, maxY = 0.f;

    for (const auto& icon : m_icons)
    {
        if (!icon) continue;
        sf::FloatRect b = icon->get_bounds();

        float bx = b.position.x;
        float by = b.position.y;
        float bw = b.size.x;
        float bh = b.size.y;

        if (first)
        {
            minX = bx;
            minY = by;
            maxX = bx + bw;
            maxY = by + bh;
            first = false;
        }
        else
        {
            minX = std::min(minX, bx);
            minY = std::min(minY, by);
            maxX = std::max(maxX, bx + bw);
            maxY = std::max(maxY, by + bh);
        }
    }

    if (first) // No icons
        return sf::FloatRect({ 0.f, 0.f }, { 0.f, 0.f });

    return sf::FloatRect(
        { minX, minY },
        { maxX - minX, maxY - minY }
    );
}
