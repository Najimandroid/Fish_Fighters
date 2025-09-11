#include "UIPlayerShellsInfo.h"

#include "../core/DataLoader.h"

#include <iostream>

UIPlayerShellsInfo::UIPlayerShellsInfo(std::shared_ptr<DataLoader> dataLoader):
	UITextElement({ 1.f, 1.f }, { 1.f, 1.f }),
	m_dataLoader(dataLoader)
{
	m_text.setCharacterSize(72);
	m_text.setFillColor(sf::Color::Cyan);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(3.f);
}

void UIPlayerShellsInfo::update(float deltaTime)
{

	m_text.setString(std::to_string(m_dataLoader->get_player_data().lock()->shells) + "¤");

	auto bounds = m_text.getLocalBounds();
	m_text.setOrigin({
		bounds.position.x + bounds.size.x,
		bounds.position.y
	});

	m_text.setPosition({ 1920.f - OFFSET_Y, 0.f + OFFSET_Y });
}