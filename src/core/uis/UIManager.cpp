#include "UIManager.h"

#include "../stages/Stage.h"

#include "UIDeploymentIcons.h"
#include "UIStageCash.h"
#include "UICashUp.h"
#include "UIBaseHealthInfo.h"
#include "UITextureElement.h"

void UIManager::init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage)
{
	m_dataLoader = dataLoader;
	m_stage = stage;
}

void UIManager::add_ui_element(std::shared_ptr<UIElement> element)
{
	m_uiElements.push_back(element);
}

void UIManager::update_uis(float deltaTime)
{
	for(auto& element : m_uiElements)
	{
		element->update(deltaTime);
	}
}

void UIManager::render_uis(sf::RenderWindow& window)
{
	for(auto& element : m_uiElements)
	{
		element->render(window);
	}
}

void UIManager::handle_ui_events(const sf::Event& event, const sf::RenderWindow& window)
{
    for (auto& uiElement : m_uiElements)
    {
        uiElement->handle_event(event, window);
    }

    if (m_pendingAction)
    {
        auto action = m_pendingAction;
        m_pendingAction = nullptr; 
        action();
    }
}

void UIManager::generate_fish_tank_uis()
{
    m_uiElements.clear();

    //background

    auto bg = std::make_shared<UITextureElement>(
        sf::Vector2f{ 1920.f, 1080.f },
        sf::Vector2f{ 0.f, 0.f },
        "assets/images/textures/backgrounds/fish_tank.png"
    );

    //title text "Fish Tank"
    auto title = std::make_shared<UITextElement>(
        sf::Vector2f{ 0.f, 0.f },
        sf::Vector2f{ 50.f, 50.f },
        "Fish Tank",
        48
    );

	//battle button
    auto battleButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 200.f, 80.f },
        sf::Vector2f{ 100.f, 300.f },
        "BATTLE"
    );
    battleButton->set_callback([this]() 
    {
        m_pendingAction = [this]() {
            if (auto stage = m_stage.lock())
            {
                stage->load(1);
                generate_battle_uis();
            }
            };
    });

	//upgrade button
    auto upgradeButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 200.f, 80.f },
        sf::Vector2f{ 100.f, 400.f },
        "UPGRADE"
    );
    upgradeButton->set_callback([this]() 
    {
		// TODO : open upgrade menu
    });
    
    add_ui_element(bg);
    add_ui_element(title);
    add_ui_element(battleButton);
    add_ui_element(upgradeButton);
}

void UIManager::generate_battle_uis()
{
	m_uiElements.clear();

	auto deploymentIcons = std::make_shared<UIDeploymentIcons>(m_dataLoader, m_stage.lock());
	auto stageCash = std::make_shared<UIStageCash>(m_stage.lock());
	auto cashUp = std::make_shared<UICashUp>(m_stage.lock());
	auto baseHealthInfo = std::make_shared<UIBaseHealthInfo>(m_stage.lock());

	add_ui_element(deploymentIcons);
	add_ui_element(stageCash);
	add_ui_element(cashUp);
	add_ui_element(baseHealthInfo);
}
