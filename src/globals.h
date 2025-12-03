#pragma once

#include "types.h"

struct DifficultyParams {
    int playerHpBoost = 0;
    float enemyHpMultiplier = 1.0f;      // Multiplier for enemy HP
    float enemyDamageMultiplier = 1.0f;  // Multiplier for enemy ATK
    float lootMultiplier = 1.0f;         // Quality/quantity of loot
    bool permadeath = false;
    bool singleSaveSlot = false;
};

DifficultyParams get_difficulty_params(Difficulty difficulty);


