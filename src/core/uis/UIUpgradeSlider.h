#pragma once

#include "UIUpgradeIcon.h"

class DataLoader;

class UIUpgradeSlider : public UIElement
{
public:

	UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader);

	void init_icons();

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;
	void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:

	int m_currentIndex = 0; //index of the upgrade being displayed

	std::array<int, 4> m_availableUpgrades = { 1, 2, 1, 2 }; //it will be a list of all units uids that need to be displayed in the slider

	std::vector<std::shared_ptr<UIUpgradeIcon>> m_upgradeIcons;

	std::shared_ptr<DataLoader> m_dataLoader;

	const sf::Vector2f baseIconSize = { 300.0f, 300.0f };
	const float ICON_SPACING = 450.0f;
	const float CENTER_SCALE = 1.2f;
	const float SIDE_SCALE = 1.0f;
	const float TWEEN_DURATION = 0.3f; // secondes
};