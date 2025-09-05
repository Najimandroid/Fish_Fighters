#include "UIDeploymentIcons.h"

#include "../stages/Stage.h"

#include <iostream>

UIDeploymentIcons::UIDeploymentIcons(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage):
	m_dataLoader(dataLoader), m_stage(stage)
{
    m_equippedUnits = { 1, 2 };
	init_icons();
}

void UIDeploymentIcons::init_icons()
{
    const int rows = 2;
    const int cols = 5;
    const float iconSize = 96.f;
    const float spacing = 16.f;

    float totalWidth = cols * iconSize + (cols - 1) * spacing;
    float startX = (1280.f - totalWidth) / 2.f;
    float startY = 500.f;

    int uid = 1; // commencer l’UID à 1

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            std::string texturePath;

            if (m_equippedUnits.count(uid))
            {
                texturePath = m_dataLoader->get_unit_icon_texture_path(uid);
            }
            else
            {
                // unité non équipée -> texture vide (ou un chemin vers un sprite gris)
                texturePath = "assets/images/textures/icons/empty.png";
            }

            sf::Vector2f pos{
                startX + col * (iconSize + spacing),
                startY + row * (iconSize + spacing)
            };

            //std::cout << "Loading icon: " << texturePath << std::endl;


            auto icon = std::make_shared<UIBattleIcon>(pos, texturePath);

            icon->set_callback([this, uid]() {
                m_stage->spawn_unit(m_dataLoader->get_unit_data(uid));
			});

            // stocker l’UID dans l’icône
            icon->set_uid(uid);

            m_icons.push_back(icon);
            uid++;
        }
    }
}

void UIDeploymentIcons::update(float deltaTime)
{
    for (auto& icon : m_icons)
    {
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
