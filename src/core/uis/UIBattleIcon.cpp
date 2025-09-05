#include "UIBattleIcon.h"

#include <iostream>

UIBattleIcon::UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath):
	UIButtonElement({ 96.f, 96.f }, position),
	m_texture(sf::Texture()), m_sprite(m_texture)
{
	std::cout << "Loading icon texture from: " << iconTexturePath << std::endl;

	if(!m_texture.loadFromFile(iconTexturePath))
	{
		m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
	}

	std::cout << "Texture size: " << m_texture.getSize().x << "x" << m_texture.getSize().y << std::endl;

	m_sprite.setTexture(m_texture);
	m_sprite.setTextureRect(sf::IntRect(
		{ 0, 0 }, 
		{static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y)
	}));
	m_sprite.setPosition(position);
	m_sprite.setScale({
		96.f / m_texture.getSize().x,
		96.f / m_texture.getSize().y
		});
}

void UIBattleIcon::render(sf::RenderWindow& window)
{
	if(m_isVisible)
	{
		window.draw(m_shape);
		window.draw(m_sprite);
	}
}

int UIBattleIcon::get_uid() const
{
	return m_uid;
}

void UIBattleIcon::set_uid(int uid)
{
	m_uid = uid;
}
