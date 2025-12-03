#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "player.h"
#include "enemy.h"
#include "dungeon.h"
#include "types.h"


/**
 * @struct GameState
 * @brief Represents the complete persistent state of a game run.
 *
 * Stores player, enemies, dungeon depth, RNG seed, stairs position, and corpse run data for save/load.
 */
struct GameState {
    Difficulty difficulty = Difficulty::Adventurer; /**< Current game difficulty. */
    Player player;                                 /**< Player state. */
    std::vector<Enemy> enemies;                    /**< All enemies on the current floor. */
    int depth = 1;                                 /**< Current dungeon depth. */
    unsigned int seed = 0;                         /**< RNG seed for reproducibility. */
    Position stairsDown{};                         /**< Position of stairs to next floor. */

    /**
     * @brief List of player corpses for corpse run feature (max 3).
     * Each corpse stores inventory and stats from a previous death.
     */
    std::vector<CorpseData> corpses;  // Last 3 deaths max
};


/**
 * @namespace fileio
 * @brief File input/output for game state persistence and corpse management.
 *
 * Provides save/load/delete for game slots and corpse run data.
 */
namespace fileio {
    /**
     * @brief Save the current game state to a save slot.
     * @param state The GameState to save.
     * @param slot The save slot index.
     * @return True if save succeeded, false otherwise.
     */
    bool save_to_slot(const GameState& state, int slot);

    /**
     * @brief Load a game state from a save slot.
     * @param outState Output parameter for loaded GameState.
     * @param slot The save slot index.
     * @return True if load succeeded, false otherwise.
     */
    bool load_from_slot(GameState& outState, int slot);

    /**
     * @brief Delete a save slot.
     * @param slot The save slot index to delete.
     * @return True if deletion succeeded, false otherwise.
     */
    bool delete_slot(int slot);

    /**
     * @brief Save a corpse record for the corpse run feature.
     * @param corpse CorpseData to save.
     */
    void save_corpse(const CorpseData& corpse);

    /**
     * @brief Load all saved corpses for corpse run.
     * @return Vector of CorpseData records.
     */
    std::vector<CorpseData> load_corpses();

    /**
     * @brief Increment runsSinceDeath for all saved corpses.
     */
    void age_corpses();
}


