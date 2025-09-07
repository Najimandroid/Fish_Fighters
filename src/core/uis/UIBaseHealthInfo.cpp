#include "UIBaseHealthInfo.h"
#include "../stages/Stage.h"

UIBaseHealthInfo::UIBaseHealthInfo(std::shared_ptr<Stage> stage):
	m_stage(stage),
	m_textEnemyBase(m_font), m_textUnitBase(m_font)
{
	m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

	m_textEnemyBase.setCharacterSize(20);
	m_textEnemyBase.setFillColor(sf::Color::White);
	m_textEnemyBase.setOutlineColor(sf::Color::Black);
	m_textEnemyBase.setOutlineThickness(1.f);
	m_textEnemyBase.setPosition({ 10.f, 120.f });

	m_textUnitBase.setCharacterSize(20);
	m_textUnitBase.setFillColor(sf::Color::White);
	m_textUnitBase.setOutlineColor(sf::Color::Black);
	m_textUnitBase.setOutlineThickness(1.f);
	m_textUnitBase.setPosition({ 900.f, 120.f });
}

void UIBaseHealthInfo::update(float deltaTime)
{
	if (auto stage = m_stage.lock())
	{
		m_textEnemyBase.setString("Health: " + std::to_string(stage->get_enemy_base_health()) + " / " + std::to_string(stage->get_enemy_base_max_health()));
		m_textUnitBase.setString("Health: " + std::to_string(stage->get_unit_base_health()) + " / " + std::to_string(stage->get_unit_base_max_health()));
	}
}

void UIBaseHealthInfo::render(sf::RenderWindow& window)
{
	if (m_isVisible == false) return;

	window.draw(m_textEnemyBase);
	window.draw(m_textUnitBase);
}
