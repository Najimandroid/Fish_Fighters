#include "UIBattleIcon.h"

#include <iostream>

UIBattleIcon::UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath):
	UIButtonElement({ 144.f * 1.15f, 108.f * 1.15f }, position, "0$"),
	m_sprite(m_texture)
{
	//Loading icon texture and setting up sprite
	set_texture(iconTexturePath);

	//Loading price text
	m_text.setCharacterSize(30);
	m_text.setFillColor(sf::Color::Yellow);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(1.5f);

	set_cost(m_cost);

	//Setting up cooldown overlay
	float barWidth = m_shape.getSize().x - 15.f;
	float barHeight = m_shape.getSize().y * 0.1f;

	m_barBackground.setSize({ barWidth, barHeight });
	m_barBackground.setFillColor(sf::Color(0, 0, 0, 0));

	m_barBackground.setPosition({
		position.x + (m_shape.getSize().x - barWidth) / 2.f,
		position.y + m_shape.getSize().y - barHeight - 7.5f //offset of 7.5f
	});

	m_barFill.setSize({ 0.f, barHeight });
	m_barFill.setFillColor(sf::Color(0, 255, 255, 0));
	m_barFill.setPosition(m_barBackground.getPosition());

	//Adding m_shape outlines
	m_shape.setOutlineColor(sf::Color::Black);
	m_shape.setOutlineThickness(4.5f);

	//Setting up the black filter
	m_blackFilter.setSize(m_shape.getSize());
	m_blackFilter.setPosition(m_shape.getPosition());
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

	std::string priceTageText = (m_isOnCooldown || m_uid < 0) ? "" : std::to_string(m_cost) + "$";
	m_text.setString(priceTageText);

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

		window.draw(m_text);
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

int UIBattleIcon::get_cost() const
{
	return m_cost;
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
	{
		m_text.setString("");
		m_text.setFillColor(sf::Color::Transparent);
	}
	else
	{
		m_text.setString(std::to_string(m_cost) + "$");
		m_text.setFillColor(sf::Color::Yellow);
	}

	auto bounds = m_shape.getGlobalBounds();
	auto textBounds = m_text.getLocalBounds();

	m_text.setOrigin({
		textBounds.position.x + textBounds.size.x,
		textBounds.position.y + textBounds.size.y
	});

	float paddingX = -5.f;
	float paddingY = -5.f;

	m_text.setPosition({
		bounds.position.x + bounds.size.x - paddingX,
		bounds.position.y + bounds.size.y - paddingY
	});
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
		m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");

	m_sprite.setTextureRect(sf::IntRect(
		{ 0, 0 },
		{ static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y)
	}));

	//Fit icon inside the icon
	sf::Vector2f targetSize = m_shape.getSize();
	sf::Vector2u texSize = m_texture.getSize();

	float scaleX = targetSize.x / texSize.x;
	float scaleY = targetSize.y / texSize.y;

	m_sprite.setScale({ scaleX, scaleY });

	// centrer l’image dans le bouton
	sf::FloatRect bounds = m_sprite.getGlobalBounds();
	sf::Vector2f spritePos = m_shape.getPosition();

	// Décalage pour centrer
	spritePos.x += (targetSize.x - bounds.size.x) / 2.f;
	spritePos.y += (targetSize.y - bounds.size.y) / 2.f;

	m_sprite.setPosition(spritePos);
}

void UIBattleIcon::set_darkened(bool isDarkened)
{
	sf::Color bgColor = m_barBackground.getFillColor();

	if (isDarkened)
	{
		bgColor.a = 128;
		m_text.setFillColor(sf::Color(128, 128, 0));
	}
	else
	{
		bgColor.a = 0;
		m_text.setFillColor(sf::Color::Yellow);
	}

	m_blackFilter.setFillColor(bgColor);
}
