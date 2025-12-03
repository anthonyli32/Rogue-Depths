#pragma once

#include <string>
#include <vector>
#include "types.h"

struct Stats {
    int maxHp = 10;
    int hp = 10;
    int attack = 6;  // Buffed from 3 to 6 (doubled)
    int defense = 1;
    int speed = 10; // higher is faster; for later initiative
};

struct Item {
    std::string name;
    ItemType type = ItemType::Misc;
    Rarity rarity = Rarity::Common;
    int attackBonus = 0;
    int defenseBonus = 0;
    int hpBonus = 0;
    // Equipment/consumable extensions
    bool isEquippable = false;
    bool isConsumable = false;
    EquipmentSlot slot = EquipmentSlot::Weapon;
    int healAmount = 0;
    StatusType onUseStatus = StatusType::None;
    int onUseMagnitude = 0;
    int onUseDuration = 0;
    
    // Item affix system for enhanced loot
    ItemAffix affix = ItemAffix::NONE;
    float affixStrength = 1.0f;  // 0.5 (weak) to 2.0 (strong)
    
    // Get affix description for display
    std::string get_affix_description() const {
        switch (affix) {
            case ItemAffix::LIFESTEAL:    return "Drains 25% of damage as HP";
            case ItemAffix::BURNING:      return "Sets enemies ablaze";
            case ItemAffix::FROST:        return "Freezes enemies on hit";
            case ItemAffix::POISON_COAT:  return "Poisons enemies on hit";
            case ItemAffix::SLOW_TARGET:  return "Slows enemies on hit";
            case ItemAffix::VORPAL:       return "10% instant kill chance";
            case ItemAffix::VAMPIRIC:     return "Drains 50% of damage as HP";
            case ItemAffix::THORNS:       return "Returns 25% of damage taken";
            case ItemAffix::FIRE_RESIST:  return "Reduces fire damage by 50%";
            case ItemAffix::COLD_RESIST:  return "Reduces cold damage by 50%";
            case ItemAffix::EVASION:      return "+20% dodge chance";
            case ItemAffix::HEALTH_REGEN: return "+1 HP per turn";
            case ItemAffix::REFLECTIVE:   return "Returns 50% of damage taken";
            default:                      return "";
        }
    }
    
    // Get affix color for display (stronger = brighter)
    std::string get_affix_color() const {
        if (affix == ItemAffix::NONE) return "";
        if (affixStrength >= 1.8f) return "\033[95m";  // Bright magenta (very strong)
        if (affixStrength >= 1.5f) return "\033[35m";  // Magenta (strong)
        if (affixStrength >= 1.2f) return "\033[33m";  // Yellow (moderate)
        return "\033[36m";  // Cyan (weak)
    }
    
    // Check if item has an affix
    bool has_affix() const { return affix != ItemAffix::NONE; }
};

struct StatusEffect {
    StatusType type = StatusType::None;
    int remainingTurns = 0;
    int magnitude = 0;
};

struct Entity {
    Position position{};
    char glyph = '?';
    std::string color;
};


