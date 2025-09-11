#pragma once

#include "UIButtonElement.h"

class Stage;

class UICashUp : public UIButtonElement
{
public:

	UICashUp(std::shared_ptr<Stage> stage);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;

private:

	int m_level = 1;
	int m_maxLevel = 3;
	int m_cost = 250;

	sf::Sprite m_icon;
	sf::Texture m_texture;

	std::weak_ptr<Stage> m_stage;
};