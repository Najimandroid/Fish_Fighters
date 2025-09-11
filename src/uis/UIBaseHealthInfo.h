#pragma once

#include "UIElement.h"

class Stage;

class UIBaseHealthInfo : public UIElement
{
public:

	UIBaseHealthInfo(std::shared_ptr<Stage> stage);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;

private:

	std::weak_ptr<Stage> m_stage;

	sf::Font m_font;
	sf::Text m_textUnitBase;
	sf::Text m_textEnemyBase;
};