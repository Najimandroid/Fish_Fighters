#include "UIBaseHealthInfo.h"
#include "../stages/Stage.h"

UIBaseHealthInfo::UIBaseHealthInfo(std::shared_ptr<Stage> stage):
	m_stage(stage),
	m_textEnemyBase(m_font), m_textUnitBase(m_font)
{
	isWorldSpaceUi = true;

	auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

	m_textEnemyBase.setCharacterSize(20);
	m_textEnemyBase.setFillColor(sf::Color::White);
	m_textEnemyBase.setOutlineColor(sf::Color::Black);
	m_textEnemyBase.setOutlineThickness(1.f);

	m_textUnitBase.setCharacterSize(20);
	m_textUnitBase.setFillColor(sf::Color::White);
	m_textUnitBase.setOutlineColor(sf::Color::Black);
	m_textUnitBase.setOutlineThickness(1.f);
	
}

void UIBaseHealthInfo::update(float deltaTime)
{
	if (auto stage = m_stage.lock())
	{
		//enemy base health
		m_textEnemyBase.setString("Health: "
			+ std::to_string((int)stage->get_enemy_base().lock()->currentHealth)
			+ " / "
			+ std::to_string((int)stage->get_enemy_base().lock()->maxHealth
		));

		auto boundsEnemy = m_textEnemyBase.getLocalBounds();
		m_textEnemyBase.setOrigin({
			boundsEnemy.position.x + boundsEnemy.size.x / 2.f,
			boundsEnemy.position.y
		});
		m_textEnemyBase.setPosition(
			stage->get_enemy_base().lock()->position + sf::Vector2f{ 100.f, -80.f } //centered above the base
		);

		//unit base health
		m_textUnitBase.setString("Health: "
			+ std::to_string((int)stage->get_unit_base().lock()->currentHealth)
			+ " / "
			+ std::to_string((int)stage->get_unit_base().lock()->maxHealth
		));

		auto boundsUnit = m_textUnitBase.getLocalBounds();
		m_textUnitBase.setOrigin({
			boundsUnit.position.x + boundsUnit.size.x / 2.f,
			boundsUnit.position.y
			});
		m_textUnitBase.setPosition(
			stage->get_unit_base().lock()->position + sf::Vector2f{ 100.f, -80.f } //centered above the base
		);
	}
}


void UIBaseHealthInfo::render(sf::RenderWindow& window)
{
	if (m_isVisible == false) return;

	window.draw(m_textEnemyBase);
	window.draw(m_textUnitBase);
}
