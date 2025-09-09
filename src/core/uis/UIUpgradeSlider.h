#pragma once

#include "UIUpgradeIcon.h"
#include "UIButtonElement.h"

class DataLoader;

class UIUpgradeSlider : public UIElement
{
public:

	UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader);

	void init_icons();
	//void add_available_upgrade(int uid);
	int calculate_upgrade_cost(int uid, int level);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;
	void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:

	int m_currentIndex = 0; //index of the upgrade being displayed

	std::vector<int> m_availableUpgrades = { 1, 2 }; //it will be a list of all units uids that need to be displayed in the slider
	std::vector<std::shared_ptr<UIUpgradeIcon>> m_upgradeIcons;

	UIButtonElement m_upgradeButton;

	std::shared_ptr<DataLoader> m_dataLoader;

	//Lower part
	sf::Font m_font;
	sf::Text m_description;

	sf::RectangleShape m_lowerPart;

	const sf::Vector2f baseIconSize = { 350.0f, 350.0f };
	const float ICON_SPACING = 475.0f;
	const float CENTER_OFFSET_X = 40.f;
	const float CENTER_SCALE = 1.3f;
	const float SIDE_SCALE = 1.0f;
	const float TWEEN_DURATION = 0.3f; //secondes
};