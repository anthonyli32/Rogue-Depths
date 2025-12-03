#include "globals.h"

DifficultyParams get_difficulty_params(Difficulty difficulty) {
    DifficultyParams params;
    switch (difficulty) {
        case Difficulty::Explorer:
            // Easy mode: tankier player, weaker enemies, more loot
            params.playerHpBoost = 20;
            params.enemyHpMultiplier = 0.7f;
            params.enemyDamageMultiplier = 0.7f;
            params.lootMultiplier = 1.2f;
            params.permadeath = false;
            params.singleSaveSlot = false;
            break;
        case Difficulty::Adventurer:
            // Normal mode: balanced
            params.playerHpBoost = 0;
            params.enemyHpMultiplier = 1.0f;
            params.enemyDamageMultiplier = 1.0f;
            params.lootMultiplier = 1.0f;
            params.permadeath = false;
            params.singleSaveSlot = false;
            break;
        case Difficulty::Nightmare:
            // Hard mode: weaker player, stronger enemies, less loot
            params.playerHpBoost = -5;
            params.enemyHpMultiplier = 1.5f;
            params.enemyDamageMultiplier = 1.3f;
            params.lootMultiplier = 0.8f;
            params.permadeath = true;
            params.singleSaveSlot = true;
            break;
    }
    return params;
}


