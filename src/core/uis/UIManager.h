#pragma once

#include "UIElement.h"
#include "../DataLoader.h"

#include <SFML/Graphics.hpp>

#include <vector>

class Stage;

class UIManager
{
public:

	UIManager() = default;

	void init(std::shared_ptr<DataLoader> dataLoader);

	void add_ui_element(std::shared_ptr<UIElement> element);
	void update_uis(float deltaTime);
	void render_uis(sf::RenderWindow& window);
	void handle_ui_events(const sf::Event& event, const sf::RenderWindow& window);

	void generate_battle_uis(std::shared_ptr<Stage> stage);

private:

	std::shared_ptr<DataLoader> m_dataLoader = nullptr;

	std::vector<std::shared_ptr<UIElement>> m_uiElements;
};