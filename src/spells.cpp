#include "spells.h"
#include "glyphs.h"
#include "constants.h"
#include "input.h"
#include "logger.h"

#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>

// Spell glyph
const char* Spell::glyph() const {
    switch (type) {
        case SpellType::FIREBALL:   return glyphs::fire();
        case SpellType::BLINK:      return glyphs::sparkle();
        case SpellType::HEAL:       return glyphs::heart_full();
        case SpellType::FROST_NOVA: return glyphs::ice();
        case SpellType::LIGHTNING:  return glyphs::status_haste();
        default:                    return glyphs::magic();
    }
}

// Spell color
std::string Spell::color() const {
    switch (type) {
        case SpellType::FIREBALL:   return "\033[38;5;196m";  // Red
        case SpellType::BLINK:      return "\033[38;5;129m";  // Purple
        case SpellType::HEAL:       return "\033[38;5;46m";   // Green
        case SpellType::FROST_NOVA: return "\033[38;5;51m";   // Cyan
        case SpellType::LIGHTNING:  return "\033[38;5;226m";  // Yellow
        default:                    return "\033[38;5;33m";   // Blue
    }
}

namespace spells {
    // Initialize mage spells
    std::vector<Spell> create_mage_spells() {
        std::vector<Spell> spellList;
        
        // Fireball - 8 mana, 2-tile radius, 10 damage + burn
        Spell fireball;
        fireball.type = SpellType::FIREBALL;
        fireball.name = "Fireball";
        fireball.manaCost = 8;
        fireball.cooldown = 3;
        fireball.currentCooldown = 0;
        fireball.description = "2-tile radius, 10 dmg + burn";
        spellList.push_back(fireball);
        
        // Blink - 6 mana, teleport to random walkable tile
        Spell blink;
        blink.type = SpellType::BLINK;
        blink.name = "Blink";
        blink.manaCost = 6;
        blink.cooldown = 2;
        blink.currentCooldown = 0;
        blink.description = "Teleport to random tile";
        spellList.push_back(blink);
        
        // Heal - 10 mana, restore 18 HP
        Spell heal;
        heal.type = SpellType::HEAL;
        heal.name = "Heal";
        heal.manaCost = 10;
        heal.cooldown = 4;
        heal.currentCooldown = 0;
        heal.description = "Restore 18 HP";
        spellList.push_back(heal);
        
        return spellList;
    }
    
    // Cast Fireball - area damage
    bool cast_fireball(Player& caster, std::vector<Enemy>& enemies, 
                       const Position& target, MessageLog& log) {
        Position ppos = caster.get_position();
        
        // Check mana
        if (caster.get_mana() < 8) {
            log.add(MessageType::Warning, "Not enough mana for Fireball!");
            return false;
        }
        
        caster.use_mana(8);
        
        // Damage all enemies within 2 tiles of target
        int radius = 2;
        int damage = 10;
        int hitCount = 0;
        
        for (auto& enemy : enemies) {
            Position epos = enemy.get_position();
            int dist = std::abs(epos.x - target.x) + std::abs(epos.y - target.y);
            if (dist <= radius) {
                enemy.stats().hp -= damage;
                hitCount++;
                LOG_DEBUG("Fireball hit " + enemy.name() + " for " + std::to_string(damage));
            }
        }
        
        log.add(MessageType::Combat, glyphs::fire() + std::string(" FIREBALL! ") + 
                std::to_string(hitCount) + " enemies hit for " + std::to_string(damage) + " damage!");
        ui::flash_critical();
        ui::play_critical_sound();
        
        // Suppress unused parameter warning
        (void)ppos;
        
        return true;
    }
    
    // Cast Blink - random teleport
    bool cast_blink(Player& caster, const Dungeon& dungeon, MessageLog& log) {
        // Check mana
        if (caster.get_mana() < 6) {
            log.add(MessageType::Warning, "Not enough mana for Blink!");
            return false;
        }
        
        caster.use_mana(6);
        
        Position ppos = caster.get_position();
        std::random_device rd;
        std::mt19937 rng(rd());
        
        // Find a random walkable tile within 10 tiles
        for (int attempts = 0; attempts < 100; ++attempts) {
            int dx = std::uniform_int_distribution<int>(-10, 10)(rng);
            int dy = std::uniform_int_distribution<int>(-10, 10)(rng);
            int nx = ppos.x + dx;
            int ny = ppos.y + dy;
            
            if (dungeon.in_bounds(nx, ny) && dungeon.is_walkable(nx, ny)) {
                caster.set_position(nx, ny);
                log.add(MessageType::Combat, glyphs::sparkle() + std::string(" BLINK! You teleport to safety!"));
                return true;
            }
        }
        
        // Failed to find a valid tile
        log.add(MessageType::Warning, "Blink failed - no valid destination!");
        caster.restore_mana(6);  // Refund mana
        return false;
    }
    
    // Cast Heal - restore HP
    bool cast_heal(Player& caster, MessageLog& log) {
        // Check mana
        if (caster.get_mana() < 10) {
            log.add(MessageType::Warning, "Not enough mana for Heal!");
            return false;
        }
        
        caster.use_mana(10);
        
        int healAmount = 18;
        int oldHp = caster.get_stats().hp;
        caster.get_stats().hp = std::min(
            caster.get_stats().hp + healAmount,
            caster.get_stats().maxHp
        );
        int actualHeal = caster.get_stats().hp - oldHp;
        
        log.add(MessageType::Heal, glyphs::heart_full() + std::string(" HEAL! Restored ") + 
                std::to_string(actualHeal) + " HP!");
        ui::flash_heal();
        
        return true;
    }
    
    // Cast Frost Nova - freeze nearby enemies
    bool cast_frost_nova(Player& caster, std::vector<Enemy>& enemies, MessageLog& log) {
        // Check mana
        if (caster.get_mana() < 12) {
            log.add(MessageType::Warning, "Not enough mana for Frost Nova!");
            return false;
        }
        
        caster.use_mana(12);
        
        Position ppos = caster.get_position();
        int radius = 3;
        int hitCount = 0;
        
        for (auto& enemy : enemies) {
            Position epos = enemy.get_position();
            int dist = std::abs(epos.x - ppos.x) + std::abs(epos.y - ppos.y);
            if (dist <= radius) {
                // Slow enemy
                enemy.stats().speed = std::max(1, enemy.stats().speed - 5);
                hitCount++;
            }
        }
        
        log.add(MessageType::Combat, glyphs::ice() + std::string(" FROST NOVA! ") + 
                std::to_string(hitCount) + " enemies frozen!");
        
        return true;
    }
    
    // Cast Lightning - chain damage
    bool cast_lightning(Player& caster, std::vector<Enemy>& enemies, MessageLog& log) {
        // Check mana
        if (caster.get_mana() < 15) {
            log.add(MessageType::Warning, "Not enough mana for Lightning!");
            return false;
        }
        
        caster.use_mana(15);
        
        int damage = 8;
        int hitCount = 0;
        
        // Hit up to 3 enemies
        for (auto& enemy : enemies) {
            if (hitCount >= 3) break;
            enemy.stats().hp -= damage;
            hitCount++;
        }
        
        log.add(MessageType::Combat, glyphs::status_haste() + std::string(" LIGHTNING! Chain hits ") + 
                std::to_string(hitCount) + " enemies for " + std::to_string(damage) + " each!");
        ui::flash_critical();
        
        return true;
    }
    
    // Dispatcher
    bool cast(SpellType type, Player& caster, std::vector<Enemy>& enemies,
              const Dungeon& dungeon, const Position& target, MessageLog& log) {
        switch (type) {
            case SpellType::FIREBALL:
                return cast_fireball(caster, enemies, target, log);
            case SpellType::BLINK:
                return cast_blink(caster, dungeon, log);
            case SpellType::HEAL:
                return cast_heal(caster, log);
            case SpellType::FROST_NOVA:
                return cast_frost_nova(caster, enemies, log);
            case SpellType::LIGHTNING:
                return cast_lightning(caster, enemies, log);
            default:
                return false;
        }
    }
    
    // Show spell selection menu
    SpellType show_spell_menu(const Player& player, const std::vector<Spell>& spells,
                              int screenRow, int screenCol) {
        const int menuWidth = 50;
        const int menuHeight = 10;
        
        // Draw menu box
        ui::fill_rect(screenRow, screenCol, menuWidth, menuHeight);
        ui::draw_box_double(screenRow, screenCol, menuWidth, menuHeight, constants::color_frame_main);
        
        // Title
        ui::move_cursor(screenRow, screenCol + 2);
        ui::set_color(constants::color_frame_main);
        std::cout << " " << glyphs::magic() << " SPELLS (Mana: " << player.get_mana() << "/" << player.get_max_mana() << ") ";
        ui::reset_color();
        
        // List spells
        int row = screenRow + 2;
        int idx = 1;
        for (const auto& spell : spells) {
            ui::move_cursor(row++, screenCol + 2);
            
            if (spell.is_ready() && player.get_mana() >= spell.manaCost) {
                ui::set_color(spell.color());
                std::cout << "[" << idx << "] " << spell.glyph() << " " << spell.name;
                ui::reset_color();
                std::cout << " (" << spell.manaCost << " MP) - " << spell.description;
            } else {
                ui::set_color(constants::color_floor);
                std::cout << "[" << idx << "] " << spell.glyph() << " " << spell.name;
                if (!spell.is_ready()) {
                    std::cout << " (CD: " << spell.currentCooldown << ")";
                } else {
                    std::cout << " (Need " << spell.manaCost << " MP)";
                }
                ui::reset_color();
            }
            idx++;
        }
        
        // Instructions
        ui::move_cursor(screenRow + menuHeight - 2, screenCol + 2);
        ui::set_color(constants::color_floor);
        std::cout << "1-3: Cast spell | ESC: Cancel";
        ui::reset_color();
        
        std::cout.flush();
        
        // Wait for input
        while (true) {
            int key = input::read_key_blocking();
            if (key == '1' && spells.size() >= 1) return spells[0].type;
            if (key == '2' && spells.size() >= 2) return spells[1].type;
            if (key == '3' && spells.size() >= 3) return spells[2].type;
            if (key == 27) return static_cast<SpellType>(-1);  // Cancel
        }
    }
}


