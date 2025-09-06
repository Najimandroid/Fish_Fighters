#include "UICashUp.h"

#include "../stages/Stage.h"

UICashUp::UICashUp(std::shared_ptr<Stage> stage):
	UIButtonElement({ 200.f, 200.f }, {0.f, 520.f}),
	m_stage(stage), 
	m_icon(m_texture),
	m_text(m_font)
{
	m_position = { 0.f, 520.f };

	m_shape.setFillColor(sf::Color(0, 0, 0, 0));

	//Icon texture and sprite
	m_texture.loadFromFile("assets/images/textures/icons/uis/cash_up.png");
	m_icon.setTextureRect(sf::IntRect(
		{ 0, 0 },
		{ static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y)
	}));
	m_icon.setPosition(m_position);

	//Font and text
	m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");
	m_text.setFont(m_font);
	m_text.setCharacterSize(20);
	m_text.setFillColor(sf::Color::Green);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(2.f);

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
		m_position.x + m_shape.getSize().x / 2.f - 22.f, //not totally centered
		m_position.y - 20.f //x pixels above the icon
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
