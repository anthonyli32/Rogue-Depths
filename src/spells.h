#pragma once

#include <string>
#include <vector>
#include "player.h"
#include "enemy.h"
#include "dungeon.h"
#include "ui.h"

// Spell types available to mage class
enum class SpellType {
    FIREBALL,   // Area damage + burn
    BLINK,      // Random teleport
    HEAL,       // Restore HP
    FROST_NOVA, // Freeze nearby enemies
    LIGHTNING   // Chain lightning
};

// Spell definition
struct Spell {
    SpellType type;
    std::string name;
    int manaCost;
    int cooldown;          // Turns between uses
    int currentCooldown;   // Current cooldown counter
    std::string description;
    
    // Get glyph for spell
    const char* glyph() const;
    
    // Get color for spell
    std::string color() const;
    
    // Check if spell is ready to cast
    bool is_ready() const { return currentCooldown <= 0; }
    
    // Tick cooldown
    void tick() { if (currentCooldown > 0) currentCooldown--; }
};

namespace spells {
    // Initialize spell list for mage
    std::vector<Spell> create_mage_spells();
    
    // Cast a spell
    bool cast_fireball(Player& caster, std::vector<Enemy>& enemies, 
                       const Position& target, MessageLog& log);
    
    bool cast_blink(Player& caster, const Dungeon& dungeon, MessageLog& log);
    
    bool cast_heal(Player& caster, MessageLog& log);
    
    bool cast_frost_nova(Player& caster, std::vector<Enemy>& enemies, MessageLog& log);
    
    bool cast_lightning(Player& caster, std::vector<Enemy>& enemies, MessageLog& log);
    
    // Cast spell by type (dispatcher)
    bool cast(SpellType type, Player& caster, std::vector<Enemy>& enemies,
              const Dungeon& dungeon, const Position& target, MessageLog& log);
    
    // Show spell selection menu
    SpellType show_spell_menu(const Player& player, const std::vector<Spell>& spells,
                              int screenRow, int screenCol);
}


