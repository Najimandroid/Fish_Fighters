#include "UIDeploymentIcons.h"

#include "../stages/Stage.h"

#include <iostream>

UIDeploymentIcons::UIDeploymentIcons(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage):
	m_dataLoader(dataLoader), m_stage(stage)
{
	init_icons();
}

void UIDeploymentIcons::init_icons()
{
    const int rows = 2;
    const int cols = 5;

    const float iconSize = 144.f;
    const float spacing = 24.f;

    float totalWidth = cols * iconSize + (cols - 1) * spacing;
    float startX = (1920.f - totalWidth) / 2.f;
    float startY = 750.f;

    int index = 0;

    std::array<int, 10> equippedUnits;
    if (auto player = m_dataLoader->get_player_data().lock())
        equippedUnits = player->equipedUnits; //getting all equipped units
    else
        equippedUnits.fill(-1);

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            sf::Vector2f pos{
                startX + col * (iconSize + spacing),
                startY + row * (iconSize + spacing)
            };

            auto icon = std::make_shared<UIBattleIcon>(pos, "assets/images/textures/icons/empty.png");

            int uid = equippedUnits[index];

            if (uid >= 0)
            {
                std::string texturePath = m_dataLoader->get_unit_icon_texture_path(uid);
                std::shared_ptr<EntityData> unitData = m_dataLoader->get_unit_data(uid);

                icon->set_texture(texturePath);
                icon->set_uid(uid);
                icon->set_cost(unitData->cost);
                icon->set_max_cooldown(unitData->cooldown);

                std::weak_ptr<EntityData> weakData = unitData;

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
                icon->set_uid(-1);
                icon->set_cost(-1);
                icon->set_max_cooldown(-1);
            }

            m_icons.push_back(icon);
            index++;
        }
    }
}

void UIDeploymentIcons::update(float deltaTime)
{
    for (auto& icon : m_icons)
    {
        if(m_stage.lock()->get_cash() < icon->get_cost() || icon->get_if_on_cooldown() || icon->get_uid() < 0)
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

void UIDeploymentIcons::render(sf::RenderWindow& window)
{
	if (m_isVisible == false) return;

    for (auto& icon : m_icons)
    {
        icon->render(window);
    }
}

void UIDeploymentIcons::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    for (auto& icon : m_icons)
    {
        icon->handle_event(event, window);
    }
}
