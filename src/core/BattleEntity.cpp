#include "BattleEntity.h"

BattleEntity::BattleEntity():

	hitbox(sf::FloatRect(position, { 1.0f, 720.0f })), attackRangeZone(sf::FloatRect(position, { 1.0f, 720.0f })),
	damageZone(sf::FloatRect(position, { 1.0f, 720.0f })),

	texture(sf::Texture()), sprite(sf::Sprite(texture))
{

#ifdef DEBUG_MODE
	rHitbox.setPosition(position);
	rHitbox.setSize(hitbox.size);
	rHitbox.setOutlineColor(sf::Color::Blue);
	rHitbox.setFillColor(sf::Color::Transparent);
	rHitbox.setOutlineThickness(1.f);

	rAttackRangeZone.setPosition(position);
	rAttackRangeZone.setSize(attackRangeZone.size);
	rAttackRangeZone.setOutlineColor(sf::Color::Yellow);
	rAttackRangeZone.setFillColor(sf::Color::Transparent);
	rAttackRangeZone.setOutlineThickness(2.f);

	rDamageZone.setPosition(position);
	rDamageZone.setSize(damageZone.size);
	rDamageZone.setOutlineColor(sf::Color::Red);
	rDamageZone.setFillColor(sf::Color::Transparent);
	rDamageZone.setOutlineThickness(3.f);
#endif
}