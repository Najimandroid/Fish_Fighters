#pragma once

#include <string>

enum AttackType;

/*
* EnemyData is a template for all Enemies in the game.
* Check EntityDataLoader to get the data of any enemy.
*/

struct EnemyData
{
	//Every Enemy will have a unique ID
	int UID = -1;

	//General datas
	std::string name = "Unknown Enemy";
	std::string description = "???";

	//Battle datas
	float health = 1.0f;

	float attackPower = 1.0f;
	float attackRange = 1.0f;
	AttackType attackType = static_cast<AttackType>(1);
	float attackFrequency = 1.0f;

	float movementSpeed = 1.0f;

	int knockbackCount = 1;

	std::string texture;
};