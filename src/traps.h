#pragma once

#include "types.h"
#include "player.h"
#include "dungeon.h"
#include "ui.h"
#include "logger.h"
#include <random>
#include <string>

namespace traps {
    // Trap data structure
    struct Trap {
        Position position;
        TrapType type = TrapType::NONE;
        bool triggered = false;
        bool detected = false;
    };
    
    // Get a random trap type
    TrapType get_random_trap_type(std::mt19937& rng);
    
    // Create a trap at position
    Trap create_trap(int x, int y, TrapType type);
    
    // Trigger a trap on the player
    void trigger_trap(Trap& trap, Player& player, Dungeon& dungeon, MessageLog& log, std::mt19937& rng);
    
    // Check if player detects a trap (based on perception/class)
    bool player_detects_trap(const Player& player, const Trap& trap, std::mt19937& rng);
    
    // Get trap damage range
    std::pair<int, int> get_trap_damage(TrapType type);
    
    // Get trap description
    std::string get_trap_description(TrapType type);
    
    // Get trap glyph
    const char* get_trap_glyph(TrapType type, bool detected);
    
    // Get trap color
    const char* get_trap_color(TrapType type);
    
    // Calculate trap damage
    int calculate_trap_damage(TrapType type, std::mt19937& rng);
    
    // Find random walkable position for teleport
    Position find_random_walkable(const Dungeon& dungeon, std::mt19937& rng);
}

