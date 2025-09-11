#pragma once

#include "UITextureElement.h"

#include "tweeny.h"

class UIUpgradeIcon : public UITextureElement
{
public:

	UIUpgradeIcon(const std::string& iconTexturePath);


	int get_uid() const;

	void select(bool isSelected);

	void set_unit_name(const std::string& name);
	void set_unit_level(int level);
	void set_upgrade_cost(int cost);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;

	void set_size(sf::Vector2f size) override;

public:

	tweeny::tween<float> tweenX;
	tweeny::tween<float> tweenWidth;
	tweeny::tween<float> tweenHeight;

private:

	int m_uid = -1;

	bool m_isSelected = false;

	int m_upgradeCost = 100;
	int m_currentLevel = 1;

	sf::Font m_font;
	sf::Text m_unitName;
	sf::Text m_currentLevelText;
	sf::Text m_upgradeCostText;

	sf::RectangleShape m_shape;
	sf::RectangleShape m_blackFilter;

	const float TITLE_BASE_TEXT_SIZE = 35.f;
	const float INFO_BASE_TEXT_SIZE = 45.f;
	const float INACTIVE_TEXT_SIZE_SCALE = 1.0f;
	const float ACTIVE_TEXT_SIZE_SCALE = 1.4f;
};