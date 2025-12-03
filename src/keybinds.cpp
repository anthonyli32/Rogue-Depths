#include "keybinds.h"
#include "input.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace keybinds {

// Internal storage: maps key codes to actions
static std::unordered_map<int, GameAction> g_keyToAction;
// Reverse mapping for display: action to list of key codes
static std::unordered_map<GameAction, std::vector<int>> g_actionToKeys;

// Convert string key name to key code
static int string_to_keycode(const std::string& str) {
    if (str.empty()) return -1;
    
    // Special keys
    if (str == "UP" || str == "ARROW_UP") return input::KEY_UP;
    if (str == "DOWN" || str == "ARROW_DOWN") return input::KEY_DOWN;
    if (str == "LEFT" || str == "ARROW_LEFT") return input::KEY_LEFT;
    if (str == "RIGHT" || str == "ARROW_RIGHT") return input::KEY_RIGHT;
    if (str == "TAB") return '\t';
    if (str == "ENTER" || str == "RETURN") return '\n';
    if (str == "SPACE") return ' ';
    if (str == "ESC" || str == "ESCAPE") return 27;
    
    // Single character
    if (str.length() == 1) {
        return static_cast<int>(str[0]);
    }
    
    return -1;
}

// Convert action string to enum
static GameAction string_to_action(const std::string& str) {
    if (str == "MOVE_UP") return GameAction::MOVE_UP;
    if (str == "MOVE_DOWN") return GameAction::MOVE_DOWN;
    if (str == "MOVE_LEFT") return GameAction::MOVE_LEFT;
    if (str == "MOVE_RIGHT") return GameAction::MOVE_RIGHT;
    if (str == "INVENTORY") return GameAction::INVENTORY;
    if (str == "EQUIP") return GameAction::EQUIP;
    if (str == "USE") return GameAction::USE;
    if (str == "DROP") return GameAction::DROP;
    if (str == "HELP") return GameAction::HELP;
    if (str == "DESCEND") return GameAction::DESCEND;
    if (str == "ASCEND") return GameAction::ASCEND;
    if (str == "QUIT") return GameAction::QUIT;
    if (str == "TURN_LEFT") return GameAction::TURN_LEFT;
    if (str == "TURN_RIGHT") return GameAction::TURN_RIGHT;
    if (str == "DEBUG_RESET") return GameAction::DEBUG_RESET;
    if (str == "DEBUG_SPAWN_ITEMS") return GameAction::DEBUG_SPAWN_ITEMS;
    if (str == "DEBUG_SPAWN_ENEMY") return GameAction::DEBUG_SPAWN_ENEMY;
    if (str == "TAB_NEXT") return GameAction::TAB_NEXT;
    if (str == "CONFIRM") return GameAction::CONFIRM;
    if (str == "CANCEL") return GameAction::CANCEL;
    return GameAction::NONE;
}

const char* action_name(GameAction action) {
    switch (action) {
        case GameAction::MOVE_UP: return "MOVE_UP";
        case GameAction::MOVE_DOWN: return "MOVE_DOWN";
        case GameAction::MOVE_LEFT: return "MOVE_LEFT";
        case GameAction::MOVE_RIGHT: return "MOVE_RIGHT";
        case GameAction::INVENTORY: return "INVENTORY";
        case GameAction::EQUIP: return "EQUIP";
        case GameAction::USE: return "USE";
        case GameAction::DROP: return "DROP";
        case GameAction::HELP: return "HELP";
        case GameAction::DESCEND: return "DESCEND";
        case GameAction::ASCEND: return "ASCEND";
        case GameAction::QUIT: return "QUIT";
        case GameAction::TURN_LEFT: return "TURN_LEFT";
        case GameAction::TURN_RIGHT: return "TURN_RIGHT";
        case GameAction::DEBUG_RESET: return "DEBUG_RESET";
        case GameAction::DEBUG_SPAWN_ITEMS: return "DEBUG_SPAWN_ITEMS";
        case GameAction::DEBUG_SPAWN_ENEMY: return "DEBUG_SPAWN_ENEMY";
        case GameAction::TAB_NEXT: return "TAB_NEXT";
        case GameAction::CONFIRM: return "CONFIRM";
        case GameAction::CANCEL: return "CANCEL";
        default: return "NONE";
    }
}

std::string key_to_string(int keyCode) {
    if (keyCode == input::KEY_UP) return "UP";
    if (keyCode == input::KEY_DOWN) return "DOWN";
    if (keyCode == input::KEY_LEFT) return "LEFT";
    if (keyCode == input::KEY_RIGHT) return "RIGHT";
    if (keyCode == '\t') return "TAB";
    if (keyCode == '\n') return "ENTER";
    if (keyCode == ' ') return "SPACE";
    if (keyCode == 27) return "ESC";
    if (keyCode >= 32 && keyCode < 127) {
        return std::string(1, static_cast<char>(keyCode));
    }
    return "?";
}

void set_defaults() {
    g_keyToAction.clear();
    g_actionToKeys.clear();
    
    // Movement
    g_keyToAction['w'] = GameAction::MOVE_UP;
    g_keyToAction['W'] = GameAction::MOVE_UP;
    g_keyToAction[input::KEY_UP] = GameAction::MOVE_UP;
    
    g_keyToAction['s'] = GameAction::MOVE_DOWN;
    g_keyToAction['S'] = GameAction::MOVE_DOWN;
    g_keyToAction[input::KEY_DOWN] = GameAction::MOVE_DOWN;
    
    g_keyToAction['a'] = GameAction::MOVE_LEFT;
    g_keyToAction['A'] = GameAction::MOVE_LEFT;
    g_keyToAction[input::KEY_LEFT] = GameAction::MOVE_LEFT;
    
    g_keyToAction['d'] = GameAction::MOVE_RIGHT;
    g_keyToAction['D'] = GameAction::MOVE_RIGHT;
    g_keyToAction[input::KEY_RIGHT] = GameAction::MOVE_RIGHT;
    
    // Inventory
    g_keyToAction['i'] = GameAction::INVENTORY;
    g_keyToAction['I'] = GameAction::INVENTORY;
    
    g_keyToAction['e'] = GameAction::EQUIP;
    g_keyToAction['E'] = GameAction::EQUIP;
    
    g_keyToAction['u'] = GameAction::USE;
    g_keyToAction['U'] = GameAction::USE;
    
    // Note: 'd' is overloaded - MOVE_RIGHT in map, DROP in inventory
    // This is handled contextually in main.cpp
    
    // Game actions
    g_keyToAction['?'] = GameAction::HELP;
    g_keyToAction['>'] = GameAction::DESCEND;
    g_keyToAction['<'] = GameAction::ASCEND;
    g_keyToAction['q'] = GameAction::QUIT;
    g_keyToAction['Q'] = GameAction::QUIT;
    
    // Debug
    g_keyToAction['R'] = GameAction::DEBUG_RESET;
    g_keyToAction['g'] = GameAction::DEBUG_SPAWN_ITEMS;
    g_keyToAction['G'] = GameAction::DEBUG_SPAWN_ITEMS;
    g_keyToAction['n'] = GameAction::DEBUG_SPAWN_ENEMY;
    g_keyToAction['N'] = GameAction::DEBUG_SPAWN_ENEMY;
    
    // UI
    g_keyToAction['\t'] = GameAction::TAB_NEXT;
    
    // Confirmation
    g_keyToAction['y'] = GameAction::CONFIRM;
    g_keyToAction['Y'] = GameAction::CONFIRM;
    
    // Build reverse mapping
    for (const auto& [key, action] : g_keyToAction) {
        g_actionToKeys[action].push_back(key);
    }
    
    LOG_INFO("Keybindings set to defaults");
}

// Simple JSON parser for our specific format
// Parses: { "ACTION": ["key1", "key2"], ... }
static bool parse_json_config(const std::string& content) {
    g_keyToAction.clear();
    g_actionToKeys.clear();
    
    // Find content between outer braces
    size_t start = content.find('{');
    size_t end = content.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return false;
    }
    
    std::string inner = content.substr(start + 1, end - start - 1);
    
    // Parse each "ACTION": ["keys"] pair
    size_t pos = 0;
    while (pos < inner.length()) {
        // Find action name (quoted string)
        size_t actionStart = inner.find('"', pos);
        if (actionStart == std::string::npos) break;
        size_t actionEnd = inner.find('"', actionStart + 1);
        if (actionEnd == std::string::npos) break;
        
        std::string actionName = inner.substr(actionStart + 1, actionEnd - actionStart - 1);
        GameAction action = string_to_action(actionName);
        
        // Find the array start
        size_t arrayStart = inner.find('[', actionEnd);
        if (arrayStart == std::string::npos) break;
        size_t arrayEnd = inner.find(']', arrayStart);
        if (arrayEnd == std::string::npos) break;
        
        std::string arrayContent = inner.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
        
        // Parse keys in array
        size_t keyPos = 0;
        while (keyPos < arrayContent.length()) {
            size_t keyStart = arrayContent.find('"', keyPos);
            if (keyStart == std::string::npos) break;
            size_t keyEnd = arrayContent.find('"', keyStart + 1);
            if (keyEnd == std::string::npos) break;
            
            std::string keyStr = arrayContent.substr(keyStart + 1, keyEnd - keyStart - 1);
            int keyCode = string_to_keycode(keyStr);
            
            if (keyCode >= 0 && action != GameAction::NONE) {
                g_keyToAction[keyCode] = action;
                g_actionToKeys[action].push_back(keyCode);
            }
            
            keyPos = keyEnd + 1;
        }
        
        pos = arrayEnd + 1;
    }
    
    // Add arrow keys as defaults for movement if not specified
    if (g_keyToAction.find(input::KEY_UP) == g_keyToAction.end()) {
        g_keyToAction[input::KEY_UP] = GameAction::MOVE_UP;
        g_actionToKeys[GameAction::MOVE_UP].push_back(input::KEY_UP);
    }
    if (g_keyToAction.find(input::KEY_DOWN) == g_keyToAction.end()) {
        g_keyToAction[input::KEY_DOWN] = GameAction::MOVE_DOWN;
        g_actionToKeys[GameAction::MOVE_DOWN].push_back(input::KEY_DOWN);
    }
    if (g_keyToAction.find(input::KEY_LEFT) == g_keyToAction.end()) {
        g_keyToAction[input::KEY_LEFT] = GameAction::MOVE_LEFT;
        g_actionToKeys[GameAction::MOVE_LEFT].push_back(input::KEY_LEFT);
    }
    if (g_keyToAction.find(input::KEY_RIGHT) == g_keyToAction.end()) {
        g_keyToAction[input::KEY_RIGHT] = GameAction::MOVE_RIGHT;
        g_actionToKeys[GameAction::MOVE_RIGHT].push_back(input::KEY_RIGHT);
    }
    
    return !g_keyToAction.empty();
}

bool init(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        LOG_WARN("Could not open keybindings config: " + configPath + ", using defaults");
        set_defaults();
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    if (parse_json_config(content)) {
        LOG_INFO("Loaded keybindings from: " + configPath);
        return true;
    } else {
        LOG_WARN("Failed to parse keybindings config, using defaults");
        set_defaults();
        return false;
    }
}

GameAction get_action(int keyCode) {
    auto it = g_keyToAction.find(keyCode);
    if (it != g_keyToAction.end()) {
        return it->second;
    }
    return GameAction::NONE;
}

bool is_action(int keyCode, GameAction action) {
    return get_action(keyCode) == action;
}

std::vector<std::string> get_keys_for_action(GameAction action) {
    std::vector<std::string> result;
    auto it = g_actionToKeys.find(action);
    if (it != g_actionToKeys.end()) {
        for (int keyCode : it->second) {
            result.push_back(key_to_string(keyCode));
        }
    }
    return result;
}

bool save(const std::string& configPath) {
    std::ofstream file(configPath);
    if (!file.is_open()) {
        LOG_ERROR("Could not save keybindings to: " + configPath);
        return false;
    }
    
    file << "{\n";
    
    // Group by action
    bool first = true;
    for (const auto& [action, keys] : g_actionToKeys) {
        if (!first) file << ",\n";
        first = false;
        
        file << "  \"" << action_name(action) << "\": [";
        bool firstKey = true;
        for (int keyCode : keys) {
            if (!firstKey) file << ", ";
            firstKey = false;
            file << "\"" << key_to_string(keyCode) << "\"";
        }
        file << "]";
    }
    
    file << "\n}\n";
    file.close();
    
    LOG_INFO("Saved keybindings to: " + configPath);
    return true;
}

} // namespace keybinds


