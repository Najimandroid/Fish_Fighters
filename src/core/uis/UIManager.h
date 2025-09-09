#pragma once

#include "UIElement.h"
#include "../DataLoader.h"

#include <SFML/Graphics.hpp>

#include <vector>
#include <functional>

class Stage;

class UIManager
{
public:

	UIManager() = default;

	void init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage);

	void add_ui_element(std::shared_ptr<UIElement> element);
	void update_uis(float deltaTime);
	void render_uis(sf::RenderWindow& window, sf::View& uiView, sf::View& worldView);
	void handle_ui_events(const sf::Event& event, const sf::RenderWindow& window);

	void generate_fish_tank_uis();
	void generate_upgrade_menu_uis();
	void generate_battle_uis();

	bool is_mouse_over_ui(const sf::Vector2i& worldPosition) const;

private:

	std::shared_ptr<DataLoader> m_dataLoader = nullptr;
	std::weak_ptr<Stage> m_stage;

	std::vector<std::shared_ptr<UIElement>> m_uiElements;

	std::function<void()> m_pendingAction;
};