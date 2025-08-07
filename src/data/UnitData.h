#pragma once

#include <string>

enum AttackType;

/*
* UnitData is a template for all Units in the game.
* Check EntityDataLoader to get the data of any unit.
*/

struct UnitData
{
	//Every Unit will have a unique ID
	int UID = -1;

	//General datas
	std::string name = "Unknown Unit";
	std::string description = "???";

	//Battle datas
	float health = 1.0f;

	float attackPower = 1.0f;
	float attackRange = 1.0f;
	AttackType attackType = static_cast<AttackType>(1);
	float attackFrequency = 1.0f;
	float foreswingTime = 0.0f; //Time before the attack is executed
	float backswingTime = 0.0f; //Time after the attack is executed (before going back to idle)

	float movementSpeed = 1.0f;

	int knockbackCount = 1;

	std::string texture;
	int frameCount = 1;
	int knockbackFrameIndex = 1;
};