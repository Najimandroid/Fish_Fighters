#pragma once

#include "UIButtonElement.h"

#include <string>

class UIBattleIcon : public UIButtonElement
{
public:

	UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath);

	void render(sf::RenderWindow& window) override;

	int get_uid() const;

	void set_uid(int uid);

private:

	int m_uid = -1; //unit uid

	sf::Texture m_texture;
	sf::Sprite m_sprite;
};