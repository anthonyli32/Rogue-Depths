#pragma once

#include <vector>
#include "enemy.h"
#include "dungeon.h"
#include "player.h"
#include "ui.h"


/**
 * @namespace ai
 * @brief Contains enemy AI logic and utility functions for enemy behavior.
 */
namespace ai {

    /**
     * @brief Executes the enemy's turn, choosing and performing an action.
     * @param enemy The enemy taking its turn.
     * @param player The player character.
     * @param dungeon The current dungeon state.
     * @param log The message log for combat output.
     */
    void take_turn(Enemy& enemy, const Player& player, const Dungeon& dungeon, MessageLog& log);

    /**
     * @brief Checks line of sight between two points using Bresenham's algorithm.
     * @param x1 Start x coordinate.
     * @param y1 Start y coordinate.
     * @param x2 End x coordinate.
     * @param y2 End y coordinate.
     * @param dungeon The dungeon map.
     * @return True if line of sight exists, false otherwise.
     */
    bool has_line_of_sight(int x1, int y1, int x2, int y2, const Dungeon& dungeon);

    /**
     * @brief Computes Manhattan distance between two points.
     * @param x1 Start x coordinate.
     * @param y1 Start y coordinate.
     * @param x2 End x coordinate.
     * @param y2 End y coordinate.
     * @return The Manhattan distance.
     */
    int manhattan_distance(int x1, int y1, int x2, int y2);

    /**
     * @brief Moves an enemy away from the player (used for ranged enemies).
     * @param enemy The enemy to move.
     * @param player The player character.
     * @param dungeon The dungeon map.
     */
    void move_away_from(Enemy& enemy, const Player& player, const Dungeon& dungeon);

    /**
     * @brief Performs a ranged attack from enemy to player.
     * @param enemy The attacking enemy.
     * @param player The player being attacked.
     * @param baseDamage The base damage value.
     * @param depth The dungeon depth (for scaling).
     * @param log The message log for combat output.
     */
    void ranged_attack(Enemy& enemy, Player& player, int baseDamage, int depth, MessageLog& log);

    /**
     * @brief Executes boss-specific behavior patterns.
     * @param enemy The boss enemy.
     * @param player The player character.
     * @param dungeon The dungeon map.
     * @param log The message log for combat output.
     */
    void behavior_boss(Enemy& enemy, const Player& player, const Dungeon& dungeon, MessageLog& log);

    /**
     * @brief Checks if an enemy type is a boss.
     * @param type The enemy type to check.
     * @return True if the type is a boss, false otherwise.
     */
    bool is_boss_type(EnemyType type);
}


