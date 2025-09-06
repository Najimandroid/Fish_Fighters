#include "UIBattleIcon.h"

#include <iostream>

UIBattleIcon::UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath):
	UIButtonElement({ 96.f, 96.f }, position),
	m_sprite(m_texture),
	m_priceTag(m_font, "0$", 20)
{

	//Loading texture and icon sprite

	if(!m_texture.loadFromFile(iconTexturePath))
	{
		m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
	}

	m_sprite.setTextureRect(sf::IntRect(
		{ 0, 0 }, 
		{static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y)
	}));
	m_sprite.setPosition(position);
	m_sprite.setScale({
		96.f / m_texture.getSize().x,
		96.f / m_texture.getSize().y
	});

	//Loading font and price text

	if (!m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf"))
	{
		std::cout << "Failed to load font from UIBattleIcon\n";
	}

	m_priceTag.setFillColor(sf::Color::Yellow);
	m_priceTag.setOutlineColor(sf::Color::Black);
	m_priceTag.setOutlineThickness(1.f);

	set_cost(m_cost);

	//Setting up cooldown overlay
	m_blackFilter.setSize(m_shape.getSize());
	m_blackFilter.setPosition(m_shape.getPosition());
	m_blackFilter.setFillColor(sf::Color(50, 50, 50, 0));

	float barWidth = m_shape.getSize().x - 10.f;
	float barHeight = 8.f;
	m_barBackground.setSize({ barWidth, barHeight });
	m_barBackground.setFillColor(sf::Color(0, 0, 0, 0));
	m_barBackground.setPosition({
		position.x + (96.f - barWidth) / 2.f,
		position.y + (96.f - barHeight - 15.f)
	});

	m_barFill.setSize({ 0.f, barHeight });
	m_barFill.setFillColor(sf::Color(0, 255, 255, 0));
	m_barFill.setPosition(m_barBackground.getPosition());
}

void UIBattleIcon::update(float deltaTime)
{
	if (m_isOnCooldown)
	{
		m_currentCooldown += deltaTime;

		if (m_currentCooldown >= m_maxCooldown)
		{
			m_isOnCooldown = false;
			m_currentCooldown = 0.0f;

			sf::Color filterColor = m_blackFilter.getFillColor();
			filterColor.a = 0;
			m_blackFilter.setFillColor(filterColor);

			sf::Color bgColor = m_barBackground.getFillColor();
			bgColor.a = 0;
			m_barBackground.setFillColor(bgColor);

			sf::Color fillColor = m_barFill.getFillColor();
			fillColor.a = 0;
			m_barFill.setFillColor(fillColor);
		}
	}

	float progress = m_isOnCooldown ? (m_currentCooldown / m_maxCooldown) : 0.f;
	progress = std::clamp(progress, 0.f, 1.f);

	float fullWidth = m_barBackground.getSize().x;
	sf::Vector2f size = m_barFill.getSize();
	size.x = fullWidth * progress;
	m_barFill.setSize(size);
}

void UIBattleIcon::render(sf::RenderWindow& window)
{
	if(m_isVisible)
	{
		window.draw(m_shape);
		window.draw(m_sprite);

		window.draw(m_blackFilter);
		window.draw(m_barBackground);
		window.draw(m_barFill);

		window.draw(m_priceTag);
	}
}

int UIBattleIcon::get_uid() const
{
	return m_uid;
}

bool UIBattleIcon::get_if_on_cooldown() const
{
	return m_isOnCooldown;
}

void UIBattleIcon::set_uid(int uid)
{
	m_uid = uid;
}

void UIBattleIcon::set_max_cooldown(int cooldown)
{
	m_maxCooldown = cooldown;
}

void UIBattleIcon::set_cost(int cost)
{
	m_cost = cost;

	if (m_cost < 0)
		m_priceTag.setString("");
	else
		m_priceTag.setString(std::to_string(m_cost) + "$");

	auto bounds = m_shape.getGlobalBounds();

	auto textBounds = m_priceTag.getLocalBounds();
	m_priceTag.setOrigin({
		textBounds.position.x + textBounds.size.x,
		textBounds.position.y + textBounds.size.y
		});

	sf::Vector2f posOffset = { 5.f, 8.f };
	m_priceTag.setPosition(bounds.position + bounds.size + posOffset);
}

void UIBattleIcon::start_cooldown()
{
	m_isOnCooldown = true;
	m_currentCooldown = 0.0f;

	sf::Color filterColor = m_blackFilter.getFillColor();
	filterColor.a = 128;
	m_blackFilter.setFillColor(filterColor);

	sf::Color bgColor = m_barBackground.getFillColor();
	bgColor.a = 255;
	m_barBackground.setFillColor(bgColor);

	sf::Color fillColor = m_barFill.getFillColor();
	fillColor.a = 255;
	m_barFill.setFillColor(fillColor);
}

void UIBattleIcon::set_texture(const std::string& texturePath)
{
	if(!m_texture.loadFromFile(texturePath))
	{
		m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
	}
}
