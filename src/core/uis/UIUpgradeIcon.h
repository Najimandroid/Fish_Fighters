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
	void set_unit_level(const std::string& name);
	void set_upgrade_cost(const std::string& name);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;

public:

	tweeny::tween<float> tweenX;
	tweeny::tween<float> tweenWidth;
	tweeny::tween<float> tweenHeight;

private:

	int m_uid = -1;

	bool m_isSelected = false;

	sf::Font m_font;
	sf::Text m_unitName;
	sf::Text m_unitCurrentLevel;
	sf::Text m_upgradeCost;

	sf::RectangleShape m_shape;
	sf::RectangleShape m_blackFilter;
};