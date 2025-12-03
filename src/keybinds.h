#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Game actions that can be bound to keys
enum class GameAction {
    NONE,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    INVENTORY,
    EQUIP,
    USE,
    DROP,
    HELP,
    DESCEND,
    ASCEND,
    QUIT,
    TURN_LEFT,
    TURN_RIGHT,
    DEBUG_RESET,
    DEBUG_SPAWN_ITEMS,
    DEBUG_SPAWN_ENEMY,
    TAB_NEXT,
    CONFIRM,
    CANCEL
};

namespace keybinds {
    // Initialize keybindings from config file
    // Returns true if loaded successfully, false if using defaults
    bool init(const std::string& configPath = "config/controls.json");
    
    // Get the action for a given key code
    // Returns GameAction::NONE if no binding exists
    GameAction get_action(int keyCode);
    
    // Check if a key is bound to a specific action
    bool is_action(int keyCode, GameAction action);
    
    // Get all keys bound to an action (for display purposes)
    std::vector<std::string> get_keys_for_action(GameAction action);
    
    // Get action name as string (for display/logging)
    const char* action_name(GameAction action);
    
    // Convert key code to display string
    std::string key_to_string(int keyCode);
    
    // Set default bindings (used if config file not found)
    void set_defaults();
    
    // Save current bindings to config file
    bool save(const std::string& configPath = "config/controls.json");
}


