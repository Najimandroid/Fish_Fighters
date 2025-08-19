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

bool BattleEntity::WeakPtrPosXLess::operator()(const std::weak_ptr<BattleEntity>& a, const std::weak_ptr<BattleEntity>& b) const
{
	auto aptr = a.lock();
	auto bptr = b.lock();

	if (!aptr && !bptr) return false; //if the same
	if (!aptr) return false;
	if (!bptr) return true;

	//compares their position
	if (aptr->position.x < bptr->position.x) return true;
	if (aptr->position.x > bptr->position.x) return false;

	return aptr.get() < bptr.get();
}