#pragma once

#include <string>

// ==========================================================
// CONSTANTS HEADER
// ==========================================================
// NOTE: For all glyph/symbol access, use the glyphs.h header instead.
// This file contains only numeric constants and color codes.
// The glyphs system provides Unicode/ASCII fallback support.
// ==========================================================

namespace constants {
    // ==========================================================
    // MAP DIMENSIONS
    // ==========================================================
    constexpr int map_width = 80;
    constexpr int map_height = 40;
    constexpr int fov_radius = 8;

    // ==========================================================
    // VIEWPORT DIMENSIONS (for camera-centered display)
    // ==========================================================
    constexpr int viewport_width = 60;
    constexpr int viewport_height = 25;

    // ==========================================================
    // ANSI ESCAPE CODES
    // ==========================================================
    inline const std::string ansi_reset = "\033[0m";
    inline const std::string ansi_bold = "\033[1m";

    // ==========================================================
    // ENTITY COLORS
    // ==========================================================
    inline const std::string color_player = "\033[38;5;208m";      // Orange
    inline const std::string color_wall = "\033[38;5;245m";        // Gray
    inline const std::string color_floor = "\033[38;5;239m";       // Dark gray
    inline const std::string color_ui = "\033[38;5;81m";           // Cyan

    // ==========================================================
    // MONSTER COLORS BY TIER
    // ==========================================================
    inline const std::string color_monster_weak = "\033[38;5;34m";    // Green (rats, spiders)
    inline const std::string color_monster_common = "\033[38;5;178m"; // Yellow (goblins, kobolds)
    inline const std::string color_monster_strong = "\033[38;5;160m"; // Red (orcs, zombies)
    inline const std::string color_monster_elite = "\033[38;5;129m";  // Purple (ogres, trolls)
    inline const std::string color_monster_boss = "\033[38;5;196m";   // Bright red (dragons, liches)
    inline const std::string color_corpse = "\033[38;5;94m";          // Brown (corpse run enemy)

    // ==========================================================
    // ITEM COLORS BY RARITY
    // ==========================================================
    inline const std::string color_item_common = "\033[38;5;250m";    // White
    inline const std::string color_item_uncommon = "\033[38;5;34m";   // Green
    inline const std::string color_item_rare = "\033[38;5;33m";       // Blue
    inline const std::string color_item_epic = "\033[38;5;129m";      // Purple
    inline const std::string color_item_legendary = "\033[38;5;214m"; // Gold

    // ==========================================================
    // SPECIAL TILE COLORS
    // ==========================================================
    inline const std::string color_trap = "\033[38;5;196m";           // Red
    inline const std::string color_shrine = "\033[38;5;51m";          // Bright cyan
    inline const std::string color_stairs = "\033[38;5;255m";         // Bright white
    
    // ==========================================================
    // ENVIRONMENTAL HAZARD COLORS
    // ==========================================================
    inline const std::string color_water = "\033[38;5;39m";           // Blue
    inline const std::string color_deep_water = "\033[38;5;21m";      // Dark blue
    inline const std::string color_lava = "\033[38;5;202m";           // Orange-red
    inline const std::string color_chasm = "\033[38;5;232m";          // Near black

    // ==========================================================
    // UI FRAME COLORS
    // ==========================================================
    inline const std::string color_frame_main = "\033[38;5;51m";      // Cyan (main frame)
    inline const std::string color_frame_status = "\033[38;5;226m";   // Yellow (status)
    inline const std::string color_frame_inventory = "\033[38;5;45m"; // Teal (inventory)
    inline const std::string color_frame_message = "\033[38;5;252m";  // Light gray (messages)

    // ==========================================================
    // DISTANCE-BASED SHADING COLORS (depth perception)
    // ==========================================================
    // Close (0-2): Full brightness
    inline const std::string shade_close = "\033[38;5;255m";      // Bright white
    inline const std::string shade_wall_close = "\033[38;5;250m"; // Light gray
    inline const std::string shade_floor_close = "\033[38;5;244m";// Medium gray
    
    // Medium (3-4): Slightly dimmed
    inline const std::string shade_medium = "\033[38;5;245m";     // Gray
    inline const std::string shade_wall_medium = "\033[38;5;240m";// Darker gray
    inline const std::string shade_floor_medium = "\033[38;5;238m";
    
    // Far (5-6): Dim
    inline const std::string shade_far = "\033[38;5;238m";        // Dark gray
    inline const std::string shade_wall_far = "\033[38;5;236m";   // Very dark gray
    inline const std::string shade_floor_far = "\033[38;5;234m";
    
    // Very far (7+): Almost invisible (fog of war edge)
    inline const std::string shade_fog = "\033[38;5;232m";        // Near black

    // ==========================================================
    // MESSAGE TYPE COLORS
    // ==========================================================
    inline const std::string color_msg_combat = "\033[38;5;208m";     // Bright orange/red (combat - more distinct)
    inline const std::string color_msg_damage = "\033[38;5;196m";     // Red (damage taken)
    inline const std::string color_msg_heal = "\033[38;5;46m";        // Green (healing)
    inline const std::string color_msg_warning = "\033[38;5;226m";    // Yellow (warning)
    inline const std::string color_msg_info = "\033[38;5;252m";       // White (info)
    inline const std::string color_msg_loot = "\033[38;5;214m";       // Gold (loot)
    inline const std::string color_msg_level = "\033[38;5;51m";       // Cyan (level up)
    inline const std::string color_msg_death = "\033[38;5;196m";      // Red (death)

    // ==========================================================
    // STATUS EFFECT COLORS
    // ==========================================================
    inline const std::string color_status_poison = "\033[38;5;34m";   // Green
    inline const std::string color_status_bleed = "\033[38;5;196m";   // Red
    inline const std::string color_status_fortify = "\033[38;5;33m";  // Blue
    inline const std::string color_status_haste = "\033[38;5;226m";   // Yellow
    inline const std::string color_status_regen = "\033[38;5;46m";    // Bright green
    inline const std::string color_status_burn = "\033[38;5;208m";    // Orange
    inline const std::string color_status_freeze = "\033[38;5;117m";  // Cyan
    inline const std::string color_status_stun = "\033[38;5;201m";    // Magenta
}

// ==========================================================
// COMBAT BALANCE CONSTANTS
// ==========================================================
namespace combat_balance {
    // Distance thresholds
    constexpr int DISTANCE_MELEE_MAX = 1;
    constexpr int DISTANCE_CLOSE_MAX = 3;
    constexpr int DISTANCE_MEDIUM_MAX = 6;
    constexpr int DISTANCE_FAR_MAX = 10;
    constexpr float DEPTH_WEIGHT = 1.5f;
    
    // Damage modifiers
    constexpr float DAMAGE_MELEE = 1.0f;
    constexpr float DAMAGE_CLOSE = 0.95f;
    constexpr float DAMAGE_MEDIUM = 0.90f;  // Tuned from 0.85f for better ranged balance
    constexpr float DAMAGE_FAR = 0.75f;     // Tuned from 0.70f for better ranged balance
    constexpr float DAMAGE_EXTREME = 0.55f;  // Tuned from 0.50f for better ranged balance
    
    // Accuracy (hit chance %)
    constexpr int ACCURACY_MELEE = 90;
    constexpr int ACCURACY_CLOSE = 80;
    constexpr int ACCURACY_MEDIUM = 70;
    constexpr int ACCURACY_FAR = 50;
    constexpr int ACCURACY_EXTREME = 30;
    
    // Movement distances
    constexpr int ADVANCE_DISTANCE = 2;
    constexpr int RETREAT_DISTANCE = 2;
    constexpr int DEPTH_MIN = 0;
    constexpr int DEPTH_MAX = 10;
}

// ==========================================================
// GAME BALANCE CONSTANTS
// ==========================================================
namespace game_constants {
    // Save slot limits
    constexpr int MIN_SAVE_SLOT = 1;
    constexpr int MAX_SAVE_SLOT = 3;
    constexpr int CORPSE_SAVE_SLOT = 2; // Special slot for corpse run data
    
    // Boss floor depths
    constexpr int BOSS_FLOOR_1 = 2;
    constexpr int BOSS_FLOOR_2 = 4;
    constexpr int BOSS_FLOOR_3 = 6; // Final boss (6 floors total)
    
    // Enemy spawning limits
    constexpr int MAX_ENEMIES_PER_FLOOR = 1000; // Safety limit
    constexpr int MAX_INVENTORY_SIZE = 1000; // Safety limit
    constexpr int MAX_EQUIPMENT_SLOTS = 10;
    constexpr int MAX_STATUS_EFFECTS = 100;
    constexpr int MAX_CORPSES = 10;
    
    // Combat action limits
    constexpr int MULTISHOT_MAX_TARGETS = 3;
    constexpr int MULTISHOT_ENERGY_COST = 80;
    
    // Test/debug item values
    constexpr int TEST_WEAPON_ATTACK = 3;
    constexpr int TEST_ARMOR_DEFENSE = 2;
    constexpr int TEST_POTION_HEAL = 10;
    
    // Boss spawning
    constexpr int BOSS_HP_MULTIPLIER_PER_DEPTH = 5;
    constexpr int BOSS_SPAWN_SEARCH_RADIUS = 3;
    constexpr int MAX_SPAWN_ATTEMPTS = 100;
    
    // Enemy scaling
    constexpr int ENEMY_HP_SCALING_PER_DEPTH = 1;
    constexpr int ENEMY_ATK_SCALING_DIVISOR = 2; // depth / 2
    
    // UI constants
    constexpr int UI_HEARTBEAT_INTERVAL_SECONDS = 5;
    constexpr int UI_FRAME_LOG_INTERVAL = 100;
    constexpr int UI_STATUS_FRAME_HEIGHT = 3;
    constexpr int UI_MESSAGE_FRAME_HEIGHT = 5;
    constexpr int UI_BORDER_WIDTH = 2;
    
    // Viewport defaults
    constexpr int DEFAULT_VIEWPORT_WIDTH = 60;
    constexpr int DEFAULT_VIEWPORT_HEIGHT = 25;
    constexpr int MIN_VIEWPORT_WIDTH = 40;
    constexpr int MAX_VIEWPORT_WIDTH = 100;
    constexpr int MIN_VIEWPORT_HEIGHT = 15;
    constexpr int MAX_VIEWPORT_HEIGHT = 35;
}

