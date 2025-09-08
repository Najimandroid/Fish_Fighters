#include "BattleEntity.h"

#include <iostream>

BattleEntity::BattleEntity():

	hitbox(sf::FloatRect(position, { 1.0f, 720.0f })), attackRangeZone(sf::FloatRect(position, { 1.0f, 720.0f })),
	damageZone(sf::FloatRect(position, { 1.0f, 720.0f })),

	texture(sf::Texture()), sprite(sf::Sprite(texture)),

	stateMachine(std::make_shared<StateMachine>())
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

BattleEntity::~BattleEntity()
{
	//std::cout << "BattleEntity Destructor called\n";
}

void BattleEntity::init_state_machine()
{
	stateMachine->set_owner(shared_from_this());
}

void BattleEntity::set_current_stage(std::shared_ptr<Stage> stage)
{
	currentStage = stage;
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

void BattleEntity::update_sprite()
{
	//std::cout << "[Current State: " << state << "]\n";

	if (currentFrameCooldown >= timeUntilNextFrame || stateMachine->get_active_state_id() == "KNOCKBACK") currentFrameCooldown = 0.0f; //the knockback animation will bypass the frame cooldown
	else return;

	//Updating to the corresponding frame
	if (stateMachine->get_active_state_id() == "IDLE") currentFrameIndex = 0;
	else if (stateMachine->get_active_state_id() == "WALK")
	{
		if (currentFrameIndex < data->knockbackFrameIndex - 1) currentFrameIndex++;
		else currentFrameIndex = 0; //Reset to the first frame
	}
	else if (stateMachine->get_active_state_id() == "ATTACK")
	{
		if (currentFrameIndex <= data->knockbackFrameIndex) currentFrameIndex = data->knockbackFrameIndex;
		if (currentFrameIndex * sprite.getTextureRect().size.x < texture.getSize().x - sprite.getTextureRect().size.x) currentFrameIndex++;
		else currentFrameIndex = 0;//Reset to the idle frame
	}
	else if (stateMachine->get_active_state_id() == "KNOCKBACK")
	{
		if (isOnShockwave)
			currentFrameIndex = 0;
		else
			currentFrameIndex = data->knockbackFrameIndex;
	}


	//std::cout << "[Current Frame Index: " << currentFrameIndex << "]\n\n\n";

	sprite.setTextureRect({ {static_cast<int>(texture.getSize().x / data->frameCount * currentFrameIndex), 0},
		{static_cast<int>(texture.getSize().x / data->frameCount), static_cast<int>(texture.getSize().y)}
		});
}