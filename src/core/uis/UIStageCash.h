#pragma once

#include "UIElement.h"

class Stage;

class UIStageCash : public UIElement
{
public:

	UIStageCash(std::shared_ptr<Stage> stage);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;

private:

	std::weak_ptr<Stage> m_stage;

	sf::Font m_font;
	sf::Text m_text;
};