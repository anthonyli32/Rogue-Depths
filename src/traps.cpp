#include "traps.h"
#include "glyphs.h"
#include "ui.h"
#include <cmath>

namespace traps {

TrapType get_random_trap_type(std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, 4);
    switch (dist(rng)) {
        case 0: return TrapType::SPIKE_PIT;
        case 1: return TrapType::POISON_CLOUD;
        case 2: return TrapType::TELEPORT;
        case 3: return TrapType::EXPLOSIVE;
        case 4: return TrapType::SLOW_FIELD;
        default: return TrapType::SPIKE_PIT;
    }
}

Trap create_trap(int x, int y, TrapType type) {
    Trap trap;
    trap.position.x = x;
    trap.position.y = y;
    trap.type = type;
    trap.triggered = false;
    trap.detected = false;
    return trap;
}

void trigger_trap(Trap& trap, Player& player, Dungeon& dungeon, MessageLog& log, std::mt19937& rng) {
    if (trap.triggered) return;  // Already triggered
    
    trap.triggered = true;
    LOG_INFO("Trap triggered at (" + std::to_string(trap.position.x) + "," + std::to_string(trap.position.y) + ")");
    
    // Visual feedback
    ui::flash_damage();
    ui::play_hit_sound();
    
    switch (trap.type) {
        case TrapType::SPIKE_PIT: {
            int damage = calculate_trap_damage(TrapType::SPIKE_PIT, rng);
            player.take_damage(damage);
            log.add(MessageType::Damage, "\033[91m" + std::string(glyphs::trap()) + " You fall into a spike pit! -" + 
                    std::to_string(damage) + " HP\033[0m");
            LOG_DEBUG("Spike pit dealt " + std::to_string(damage) + " damage");
            break;
        }
        
        case TrapType::POISON_CLOUD: {
            player.apply_status({StatusType::Poison, 5, 3});  // 5 turns, 3 damage per turn
            log.add(MessageType::Damage, "\033[32m" + std::string(glyphs::trap()) + " A cloud of poison gas engulfs you!\033[0m");
            LOG_DEBUG("Poison cloud applied poison status");
            break;
        }
        
        case TrapType::TELEPORT: {
            Position newPos = find_random_walkable(dungeon, rng);
            player.set_position(newPos.x, newPos.y);
            log.add(MessageType::Warning, "\033[95m" + std::string(glyphs::trap()) + " A teleport trap activates!\033[0m");
            LOG_DEBUG("Teleport trap moved player to (" + std::to_string(newPos.x) + "," + std::to_string(newPos.y) + ")");
            break;
        }
        
        case TrapType::EXPLOSIVE: {
            int damage = calculate_trap_damage(TrapType::EXPLOSIVE, rng);
            player.take_damage(damage);
            log.add(MessageType::Damage, "\033[91;1m" + std::string(glyphs::trap()) + " EXPLOSION! -" + 
                    std::to_string(damage) + " HP\033[0m");
            
            // Visual: red flash
            ui::flash_damage();
            LOG_DEBUG("Explosive trap dealt " + std::to_string(damage) + " damage");
            break;
        }
        
        case TrapType::SLOW_FIELD: {
            // Apply slow status (reduce speed)
            StatusEffect slow;
            slow.type = StatusType::Fortify;  // Reuse for now, negative fortify = slow
            slow.remainingTurns = 3;
            slow.magnitude = -2;  // Negative defense simulates slow
            player.apply_status(slow);
            log.add(MessageType::Warning, "\033[96m" + std::string(glyphs::trap()) + " A slow field ensnares you!\033[0m");
            LOG_DEBUG("Slow field applied");
            break;
        }
        
        case TrapType::NONE:
            break;
    }
}

bool player_detects_trap(const Player& player, const Trap& trap, std::mt19937& rng) {
    // Base detection chance: 20%
    int baseChance = 20;
    
    // Rogues have +30% detection
    if (player.player_class() == PlayerClass::Rogue) {
        baseChance += 30;
    }
    
    // Speed adds to detection (perception)
    baseChance += player.spd() * 3;
    
    // Cap at 80%
    baseChance = std::min(baseChance, 80);
    
    std::uniform_int_distribution<int> dist(0, 100);
    bool detected = dist(rng) < baseChance;
    
    if (detected) {
        LOG_DEBUG("Player detected trap at (" + std::to_string(trap.position.x) + "," + 
                  std::to_string(trap.position.y) + ") with " + std::to_string(baseChance) + "% chance");
    }
    
    return detected;
}

std::pair<int, int> get_trap_damage(TrapType type) {
    switch (type) {
        case TrapType::SPIKE_PIT:   return {5, 10};
        case TrapType::EXPLOSIVE:   return {10, 20};
        case TrapType::POISON_CLOUD: return {0, 0};  // Damage over time
        case TrapType::TELEPORT:    return {0, 0};   // No damage
        case TrapType::SLOW_FIELD:  return {0, 0};   // No damage
        default:                    return {0, 0};
    }
}

std::string get_trap_description(TrapType type) {
    switch (type) {
        case TrapType::SPIKE_PIT:   return "Spike Pit - 5-10 damage";
        case TrapType::POISON_CLOUD: return "Poison Cloud - Applies poison";
        case TrapType::TELEPORT:    return "Teleport Trap - Random teleport";
        case TrapType::EXPLOSIVE:   return "Explosive - 10-20 area damage";
        case TrapType::SLOW_FIELD:  return "Slow Field - Reduces speed";
        default:                    return "Unknown trap";
    }
}

const char* get_trap_glyph(TrapType /* type */, bool detected) {
    if (!detected) {
        return ".";  // Hidden trap looks like floor
    }
    return glyphs::trap();
}

const char* get_trap_color(TrapType type) {
    switch (type) {
        case TrapType::SPIKE_PIT:   return "\033[90m";   // Gray
        case TrapType::POISON_CLOUD: return "\033[32m";  // Green
        case TrapType::TELEPORT:    return "\033[95m";   // Magenta
        case TrapType::EXPLOSIVE:   return "\033[91m";   // Red
        case TrapType::SLOW_FIELD:  return "\033[96m";   // Cyan
        default:                    return "\033[0m";
    }
}

int calculate_trap_damage(TrapType type, std::mt19937& rng) {
    auto [minDmg, maxDmg] = get_trap_damage(type);
    if (minDmg == 0 && maxDmg == 0) return 0;
    
    std::uniform_int_distribution<int> dist(minDmg, maxDmg);
    return dist(rng);
}

Position find_random_walkable(const Dungeon& dungeon, std::mt19937& rng) {
    Position pos;
    int attempts = 0;
    const int maxAttempts = 1000;
    
    while (attempts < maxAttempts) {
        std::uniform_int_distribution<int> xDist(1, dungeon.width() - 2);
        std::uniform_int_distribution<int> yDist(1, dungeon.height() - 2);
        
        pos.x = xDist(rng);
        pos.y = yDist(rng);
        
        if (dungeon.is_walkable(pos.x, pos.y) && 
            !dungeon.is_hazardous(pos.x, pos.y) &&
            dungeon.get_tile(pos.x, pos.y) != TileType::Trap) {
            return pos;
        }
        attempts++;
    }
    
    // Fallback: return center of map
    pos.x = dungeon.width() / 2;
    pos.y = dungeon.height() / 2;
    return pos;
}

} // namespace traps
