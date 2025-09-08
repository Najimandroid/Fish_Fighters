#pragma once

#include "UIButtonElement.h"

#include <string>

class UIBattleIcon : public UIButtonElement
{
public:

	UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;

	int get_uid() const;
	bool get_if_on_cooldown() const;
	int get_cost() const;

	void start_cooldown();

	void set_uid(int uid);
	void set_max_cooldown(int cooldown);
	void set_cost(int cost);
	void set_texture(const std::string& texturePath);
	void set_darkened(bool isDarkened);

private:

	int m_uid = -1; //unit uid

	bool m_isOnCooldown = false;

	float m_currentCooldown = 0.0f;
	int m_maxCooldown = 1;

	int m_cost = 50;

	//Box
	sf::Texture m_texture;
	sf::Sprite m_sprite;

	//Cooldown overlay
	sf::RectangleShape m_blackFilter;
	sf::RectangleShape m_barBackground;
	sf::RectangleShape m_barFill;
};