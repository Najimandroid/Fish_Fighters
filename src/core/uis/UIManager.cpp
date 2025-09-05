#include "UIManager.h"

void UIManager::init(std::shared_ptr<DataLoader> dataLoader)
{
	m_dataLoader = dataLoader;
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
	for (auto& uiElement : m_uiElements) {
		uiElement->handle_event(event, window);
	}
}

void UIManager::generate_battle_uis(std::shared_ptr<Stage> stage)
{
	auto deploymentIcons = std::make_shared<UIDeploymentIcons>(m_dataLoader, stage);
	add_ui_element(deploymentIcons);
}
