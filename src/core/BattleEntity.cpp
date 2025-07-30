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

bool BattleEntity::BattleEntityRangeComparator::operator()(const std::weak_ptr<BattleEntity>& a, const std::weak_ptr<BattleEntity>& b) const
{
	//The set is sorted by attackRange so if a single attacker wants to attack, it will always attack the closest entity.

	auto spA = a.lock();
	auto spB = b.lock();

	if (!spA) return false;
	if (!spB) return true;

	return spA->attackRangeZone.size.x < spB->attackRangeZone.size.x;
}
