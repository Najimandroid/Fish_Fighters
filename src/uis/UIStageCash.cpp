#include "UIStageCash.h"

#include "../core/stages/Stage.h"

#include <iostream>

UIStageCash::UIStageCash(std::shared_ptr<Stage> stage) :
	UITextElement({ 1.f, 1.f }, { 1.f, 1.f }),
	m_stage(stage)
{
	m_text.setCharacterSize(72);
	m_text.setFillColor(sf::Color::Yellow);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(3.f);
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

	m_text.setPosition({ 1920.f - OFFSET_X, 0.f + OFFSET_Y});
}