#pragma once

class EnemyStageData {
public:

    int UID = -1;

    int amount = 0; // -1 for infinite

    float respawnTime = 0;
    float spawnStart = 0; // seconds

    int layer = -1; // -1 means any

    float baseHealthThreshold = 100; // percentage (e.g. 100 = full base hp)

    bool isBoss = false;
    bool bypassEnemyLimit = false; // enable if you want the enemy to spawn even if the stage's enemies limit is reached

    //Intern logic
    int spawnedCount = 0;
    float currentTimer = 0.f;
    bool hasStarted = false;
};