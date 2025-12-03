#pragma once

#include "types.h"
#include "player.h"
#include "ui.h"
#include "logger.h"
#include <random>
#include <string>

namespace shrine {
    // Shrine blessing result
    struct BlessingResult {
        ShrineBlessing type;
        std::string description;
        bool isCurse;
    };
    
    // Get a random blessing/curse from a shrine
    BlessingResult get_random_blessing(std::mt19937& rng);
    
    // Apply a blessing to the player
    void apply_blessing(Player& player, ShrineBlessing blessing, MessageLog& log);
    
    // Check if player has a blessing active
    bool has_blessing(const Player& player, ShrineBlessing blessing);
    
    // Get blessing description
    std::string get_blessing_description(ShrineBlessing blessing);
    
    // Get blessing color for UI
    const char* get_blessing_color(ShrineBlessing blessing);
    
    // Tick blessings (called when descending floors)
    void tick_blessings(Player& player, MessageLog& log);
    
    // Show shrine interaction menu
    bool interact_with_shrine(Player& player, MessageLog& log, std::mt19937& rng);
}

