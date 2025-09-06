#include "UIStageCash.h"

#include "../stages/Stage.h"

#include <iostream>

UIStageCash::UIStageCash(std::shared_ptr<Stage> stage)
	: m_stage(stage), m_text(m_font)
{
	if (!m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf"))
	{
		std::cout << "Failed to load font from UIStageCash\n";
	}

	m_text.setFont(m_font);
	m_text.setCharacterSize(48);
	m_text.setFillColor(sf::Color::Yellow);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(2.f);
}

void UIStageCash::update(float deltaTime)
{

	if(auto stage = m_stage.lock())
	{
		m_text.setString(std::to_string(stage->get_cash()) + "$ / " + std::to_string(stage->get_max_cash()) + "$\n");
	}

	auto bounds = m_text.getLocalBounds();
	m_text.setOrigin({
		bounds.position.x + bounds.size.x,
		bounds.position.y
	});

	m_text.setPosition({ 1280.f, 0.f });
}

void UIStageCash::render(sf::RenderWindow& window)
{
	if (m_isVisible)
	{
		window.draw(m_text);
	}
}
