#include "shrine.h"
#include "glyphs.h"
#include "input.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace shrine {

BlessingResult get_random_blessing(std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, 100);
    int roll = dist(rng);
    
    BlessingResult result;
    
    // 15% chance of curse
    if (roll < 15) {
        result.type = ShrineBlessing::CURSE;
        result.description = "The shrine radiates dark energy...";
        result.isCurse = true;
    }
    // 10% chance of nothing
    else if (roll < 25) {
        result.type = ShrineBlessing::NOTHING;
        result.description = "The shrine remains silent.";
        result.isCurse = false;
    }
    // 15% chance each for the good blessings
    else if (roll < 40) {
        result.type = ShrineBlessing::STAT_BOOST;
        result.description = "You feel your abilities sharpen!";
        result.isCurse = false;
    }
    else if (roll < 55) {
        result.type = ShrineBlessing::HEALTH_BOOST;
        result.description = "Your vitality surges!";
        result.isCurse = false;
    }
    else if (roll < 70) {
        result.type = ShrineBlessing::DAMAGE_BOOST;
        result.description = "Your weapons gleam with power!";
        result.isCurse = false;
    }
    else if (roll < 85) {
        result.type = ShrineBlessing::PROTECTION;
        result.description = "A protective aura surrounds you!";
        result.isCurse = false;
    }
    else if (roll < 95) {
        result.type = ShrineBlessing::CURSE_REMOVAL;
        result.description = "Cleansing light washes over you!";
        result.isCurse = false;
    }
    else {
        result.type = ShrineBlessing::RESURRECTION;
        result.description = "The shrine grants you a second chance at life!";
        result.isCurse = false;
    }
    
    return result;
}

void apply_blessing(Player& player, ShrineBlessing blessing, MessageLog& log) {
    switch (blessing) {
        case ShrineBlessing::STAT_BOOST:
            player.add_atk(1);
            player.add_def(1);
            player.add_spd(1);
            log.add(MessageType::Info, "\033[93m" + std::string(glyphs::shrine()) + " +1 to all stats!\033[0m");
            LOG_INFO("Player received STAT_BOOST blessing");
            break;
            
        case ShrineBlessing::HEALTH_BOOST:
            player.set_blessing_health_boost(3);  // Lasts 3 floors
            player.set_max_hp(player.max_hp() * 2);
            player.heal(player.max_hp());
            log.add(MessageType::Info, "\033[92m" + std::string(glyphs::shrine()) + " Max HP doubled for 3 floors!\033[0m");
            LOG_INFO("Player received HEALTH_BOOST blessing");
            break;
            
        case ShrineBlessing::DAMAGE_BOOST:
            player.set_blessing_damage_boost(3);  // Lasts 3 floors
            log.add(MessageType::Info, "\033[91m" + std::string(glyphs::shrine()) + " Double damage for 3 floors!\033[0m");
            LOG_INFO("Player received DAMAGE_BOOST blessing");
            break;
            
        case ShrineBlessing::PROTECTION:
            player.set_blessing_protection(3);  // Lasts 3 floors
            log.add(MessageType::Info, "\033[96m" + std::string(glyphs::shrine()) + " 50% damage reduction for 3 floors!\033[0m");
            LOG_INFO("Player received PROTECTION blessing");
            break;
            
        case ShrineBlessing::RESURRECTION:
            player.set_has_resurrection(true);
            log.add(MessageType::Info, "\033[95m" + std::string(glyphs::shrine()) + " You will be revived once upon death!\033[0m");
            LOG_INFO("Player received RESURRECTION blessing");
            break;
            
        case ShrineBlessing::CURSE_REMOVAL:
            player.clear_statuses();
            log.add(MessageType::Info, "\033[97m" + std::string(glyphs::shrine()) + " All status effects cleansed!\033[0m");
            LOG_INFO("Player received CURSE_REMOVAL blessing");
            break;
            
        case ShrineBlessing::CURSE: {
            // Random curse effect
            std::random_device rd;
            std::mt19937 rng(rd());
            std::uniform_int_distribution<int> curseDist(0, 3);
            int curseType = curseDist(rng);
            
            switch (curseType) {
                case 0:
                    player.add_atk(-2);
                    log.add(MessageType::Damage, "\033[31m" + std::string(glyphs::shrine()) + " CURSED: -2 Attack!\033[0m");
                    break;
                case 1:
                    player.add_def(-2);
                    log.add(MessageType::Damage, "\033[31m" + std::string(glyphs::shrine()) + " CURSED: -2 Defense!\033[0m");
                    break;
                case 2:
                    player.take_damage(player.hp() / 4);
                    log.add(MessageType::Damage, "\033[31m" + std::string(glyphs::shrine()) + " CURSED: Lost 25% HP!\033[0m");
                    break;
                case 3:
                    player.apply_status({StatusType::Poison, 5, 2});
                    log.add(MessageType::Damage, "\033[31m" + std::string(glyphs::shrine()) + " CURSED: Poisoned!\033[0m");
                    break;
            }
            LOG_INFO("Player received CURSE from shrine");
            break;
        }
            
        case ShrineBlessing::NOTHING:
            log.add(MessageType::Info, "\033[90m" + std::string(glyphs::shrine()) + " The shrine is dormant.\033[0m");
            LOG_DEBUG("Shrine had no effect");
            break;
    }
}

bool has_blessing(const Player& player, ShrineBlessing blessing) {
    switch (blessing) {
        case ShrineBlessing::HEALTH_BOOST:
            return player.blessing_health_boost() > 0;
        case ShrineBlessing::DAMAGE_BOOST:
            return player.blessing_damage_boost() > 0;
        case ShrineBlessing::PROTECTION:
            return player.blessing_protection() > 0;
        case ShrineBlessing::RESURRECTION:
            return player.has_resurrection();
        default:
            return false;
    }
}

std::string get_blessing_description(ShrineBlessing blessing) {
    switch (blessing) {
        case ShrineBlessing::STAT_BOOST:    return "+1 to all stats (permanent)";
        case ShrineBlessing::HEALTH_BOOST:  return "Double max HP (3 floors)";
        case ShrineBlessing::DAMAGE_BOOST:  return "Double damage (3 floors)";
        case ShrineBlessing::PROTECTION:    return "50% damage reduction (3 floors)";
        case ShrineBlessing::RESURRECTION:  return "Revive once on death";
        case ShrineBlessing::CURSE_REMOVAL: return "Remove all status effects";
        case ShrineBlessing::CURSE:         return "Random negative effect";
        case ShrineBlessing::NOTHING:       return "No effect";
        default:                            return "Unknown";
    }
}

const char* get_blessing_color(ShrineBlessing blessing) {
    switch (blessing) {
        case ShrineBlessing::STAT_BOOST:    return "\033[93m";  // Yellow
        case ShrineBlessing::HEALTH_BOOST:  return "\033[92m";  // Green
        case ShrineBlessing::DAMAGE_BOOST:  return "\033[91m";  // Red
        case ShrineBlessing::PROTECTION:    return "\033[96m";  // Cyan
        case ShrineBlessing::RESURRECTION:  return "\033[95m";  // Magenta
        case ShrineBlessing::CURSE_REMOVAL: return "\033[97m";  // White
        case ShrineBlessing::CURSE:         return "\033[31m";  // Dark red
        case ShrineBlessing::NOTHING:       return "\033[90m";  // Gray
        default:                            return "\033[0m";
    }
}

void tick_blessings(Player& player, MessageLog& log) {
    // Tick health boost
    if (player.blessing_health_boost() > 0) {
        player.set_blessing_health_boost(player.blessing_health_boost() - 1);
        if (player.blessing_health_boost() == 0) {
            // Remove the HP boost
            player.set_max_hp(player.max_hp() / 2);
            if (player.hp() > player.max_hp()) {
                player.set_hp(player.max_hp());
            }
            log.add(MessageType::Info, "\033[90mHealth boost blessing has expired.\033[0m");
            LOG_INFO("Health boost blessing expired");
        }
    }
    
    // Tick damage boost
    if (player.blessing_damage_boost() > 0) {
        player.set_blessing_damage_boost(player.blessing_damage_boost() - 1);
        if (player.blessing_damage_boost() == 0) {
            log.add(MessageType::Info, "\033[90mDamage boost blessing has expired.\033[0m");
            LOG_INFO("Damage boost blessing expired");
        }
    }
    
    // Tick protection
    if (player.blessing_protection() > 0) {
        player.set_blessing_protection(player.blessing_protection() - 1);
        if (player.blessing_protection() == 0) {
            log.add(MessageType::Info, "\033[90mProtection blessing has expired.\033[0m");
            LOG_INFO("Protection blessing expired");
        }
    }
}

bool interact_with_shrine(Player& player, MessageLog& log, std::mt19937& rng) {
    // Legacy function no longer used in main loop; keep simple behavior for any remaining callers.
    BlessingResult result = get_random_blessing(rng);
    log.add(MessageType::Info, std::string(glyphs::shrine()) + " You feel the shrine's power...");
    log.add(MessageType::Info, result.description);
    apply_blessing(player, result.type, log);
    return !result.isCurse;
}

} // namespace shrine
