#include "UICashUp.h"

#include "../core/stages/Stage.h"

UICashUp::UICashUp(std::shared_ptr<Stage> stage):
	UIButtonElement({ 300.f, 300.f }, {0.f, 780.f}),
	m_stage(stage), 
	m_icon(m_texture)
{
	m_position = { 0.f, 780.f };

	m_shape.setFillColor(sf::Color(0, 0, 0, 0));

	//Icon texture and sprite
	auto success = m_texture.loadFromFile("assets/images/textures/icons/uis/cash_up.png");
	m_icon.setTextureRect(sf::IntRect(
		{ 0, 0 },
		{ static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y)
	}));
	m_icon.setScale({
		m_shape.getSize().x / static_cast<float>(m_texture.getSize().x),
		m_shape.getSize().y / static_cast<float>(m_texture.getSize().y)
	});
	m_icon.setPosition(m_position);

	//Font and text
	m_text.setCharacterSize(30);
	m_text.setFillColor(sf::Color::Green);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(3.f);

	//Callback
	set_callback(
		[this]()
		{
			if (m_level >= m_maxLevel) return;

			if (auto stage = m_stage.lock())
			{
				if (stage->upgrade_cash(m_level, m_cost))
				{
					m_level++;
					m_cost *= 1.5f;
				}
			}
		}
	);
}

void UICashUp::update(float deltaTime)
{
	if (m_stage.lock()->get_cash() >= m_cost || m_level == m_maxLevel)
	{
		m_texture.loadFromFile("assets/images/textures/icons/uis/cash_up.png");
		m_text.setFillColor(sf::Color::Green);
	}
	else
	{
		m_texture.loadFromFile("assets/images/textures/icons/uis/cash_up_disabled.png");
		m_text.setFillColor(sf::Color(135, 135, 135));
	}


	if(m_level >= m_maxLevel)
	{
		m_text.setString("\t-Cash Up-\nMAX LEVEL: "
			+ std::to_string(m_level));
	}
	else
	{
		m_text.setString("\t-Cash Up-\nLevel: "
			+ std::to_string(m_level)
			+ " : "
			+ std::to_string(m_cost) + "$");
	}


	auto bounds = m_text.getLocalBounds();

	m_text.setOrigin({
		bounds.position.x + bounds.size.x / 2.f,
		bounds.position.y
	});

	m_text.setPosition({
		m_position.x + m_shape.getSize().x / 2.f - 33.f, //not totally centered
		m_position.y - 30.f //x pixels above the icon
	});
}

void UICashUp::render(sf::RenderWindow& window)
{
	if (m_isVisible)
	{
		window.draw(m_shape);
		window.draw(m_icon);
		window.draw(m_text);
	}
}