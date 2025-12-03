#include "loot.h"
#include "logger.h"

#include <algorithm>

namespace loot {
    // Weapon base names by tier
    static const char* common_weapons[] = {"Rusty Sword", "Wooden Club", "Dull Knife", "Iron Dagger"};
    static const char* uncommon_weapons[] = {"Steel Sword", "War Hammer", "Battle Axe", "Longbow"};
    static const char* rare_weapons[] = {"Mithril Blade", "Enchanted Staff", "Elven Bow", "Runic Axe"};
    static const char* epic_weapons[] = {"Dragon Slayer", "Arcane Wand", "Phoenix Bow", "Doom Hammer"};
    static const char* legendary_weapons[] = {"Excalibur", "Staff of Ages", "Godslayer", "Worldbreaker"};
    
    // Armor base names by tier
    static const char* common_armor[] = {"Leather Vest", "Cloth Robe", "Wooden Shield"};
    static const char* uncommon_armor[] = {"Chainmail", "Studded Leather", "Iron Shield"};
    static const char* rare_armor[] = {"Plate Armor", "Mithril Chain", "Tower Shield"};
    static const char* epic_armor[] = {"Dragon Scale", "Arcane Vestments", "Aegis Shield"};
    static const char* legendary_armor[] = {"Celestial Plate", "Void Robes", "Divine Aegis"};
    
    // Affix prefixes for weapons
    static const char* weapon_affix_prefix[] = {
        "",              // NONE
        "Vampiric ",     // LIFESTEAL
        "Blazing ",      // BURNING
        "Frozen ",       // FROST
        "Venomous ",     // POISON_COAT
        "Slowing ",      // SLOW_TARGET
        "Vorpal ",       // VORPAL
        "Soul-Drinking " // VAMPIRIC
    };
    
    // Affix prefixes for armor
    static const char* armor_affix_prefix[] = {
        "",             // NONE
        "",             // LIFESTEAL (weapon only)
        "",             // BURNING (weapon only)
        "",             // FROST (weapon only)
        "",             // POISON_COAT (weapon only)
        "",             // SLOW_TARGET (weapon only)
        "",             // VORPAL (weapon only)
        "",             // VAMPIRIC (weapon only)
        "Thorny ",      // THORNS
        "Fireproof ",   // FIRE_RESIST
        "Frostproof ",  // COLD_RESIST
        "Evasive ",     // EVASION
        "Regenerating ",// HEALTH_REGEN
        "Reflective "   // REFLECTIVE
    };
    
    // Roll rarity based on depth
    Rarity roll_rarity(int depth, std::mt19937& rng) {
        std::uniform_int_distribution<int> roll(0, 100);
        int r = roll(rng);
        
        // Base chances, improved by depth
        int legendaryChance = 1 + depth / 3;      // 1% base, +1% per 3 floors
        int epicChance = 5 + depth;                // 5% base, +1% per floor
        int rareChance = 15 + depth * 2;           // 15% base, +2% per floor
        int uncommonChance = 30 + depth;           // 30% base, +1% per floor
        
        if (r < legendaryChance) return Rarity::Legendary;
        if (r < legendaryChance + epicChance) return Rarity::Epic;
        if (r < legendaryChance + epicChance + rareChance) return Rarity::Rare;
        if (r < legendaryChance + epicChance + rareChance + uncommonChance) return Rarity::Uncommon;
        return Rarity::Common;
    }
    
    // Roll affix based on rarity and item type
    ItemAffix roll_affix(Rarity rarity, ItemType type, std::mt19937& rng) {
        std::uniform_int_distribution<int> roll(0, 100);
        int r = roll(rng);
        
        // Chance to have an affix based on rarity
        int affixChance = 0;
        switch (rarity) {
            case Rarity::Common:    affixChance = 0; break;    // No affixes
            case Rarity::Uncommon:  affixChance = 20; break;   // 20% chance
            case Rarity::Rare:      affixChance = 60; break;   // 60% chance
            case Rarity::Epic:      affixChance = 100; break;  // Always has affix
            case Rarity::Legendary: affixChance = 100; break;  // Always has affix
        }
        
        if (r >= affixChance) return ItemAffix::NONE;
        
        // Select affix based on item type
        if (type == ItemType::Weapon) {
            // Weapon affixes
            std::vector<ItemAffix> weaponAffixes = {
                ItemAffix::LIFESTEAL,
                ItemAffix::BURNING,
                ItemAffix::FROST,
                ItemAffix::POISON_COAT,
                ItemAffix::SLOW_TARGET
            };
            
            // Rare affixes for epic/legendary
            if (rarity == Rarity::Epic || rarity == Rarity::Legendary) {
                weaponAffixes.push_back(ItemAffix::VORPAL);
                weaponAffixes.push_back(ItemAffix::VAMPIRIC);
            }
            
            std::uniform_int_distribution<size_t> affixRoll(0, weaponAffixes.size() - 1);
            return weaponAffixes[affixRoll(rng)];
        } else if (type == ItemType::Armor) {
            // Armor affixes
            std::vector<ItemAffix> armorAffixes = {
                ItemAffix::THORNS,
                ItemAffix::FIRE_RESIST,
                ItemAffix::COLD_RESIST,
                ItemAffix::EVASION,
                ItemAffix::HEALTH_REGEN
            };
            
            // Rare affixes for epic/legendary
            if (rarity == Rarity::Epic || rarity == Rarity::Legendary) {
                armorAffixes.push_back(ItemAffix::REFLECTIVE);
            }
            
            std::uniform_int_distribution<size_t> affixRoll(0, armorAffixes.size() - 1);
            return armorAffixes[affixRoll(rng)];
        }
        
        return ItemAffix::NONE;
    }
    
    // Get affix strength based on rarity
    float get_affix_strength(Rarity rarity, std::mt19937& rng) {
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);
        float base = roll(rng);
        
        switch (rarity) {
            case Rarity::Common:    return 0.0f;
            case Rarity::Uncommon:  return 0.5f + base * 0.3f;   // 0.5 - 0.8
            case Rarity::Rare:      return 0.8f + base * 0.4f;   // 0.8 - 1.2
            case Rarity::Epic:      return 1.2f + base * 0.4f;   // 1.2 - 1.6
            case Rarity::Legendary: return 1.6f + base * 0.4f;   // 1.6 - 2.0
        }
        return 1.0f;
    }
    
    // Generate weapon name
    std::string generate_weapon_name(Rarity rarity, ItemAffix affix) {
        std::string name;
        
        // Add affix prefix
        if (affix != ItemAffix::NONE && static_cast<int>(affix) < 8) {
            name = weapon_affix_prefix[static_cast<int>(affix)];
        }
        
        // Add base name
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> roll(0, 3);
        
        switch (rarity) {
            case Rarity::Common:    name += common_weapons[roll(rng)]; break;
            case Rarity::Uncommon:  name += uncommon_weapons[roll(rng)]; break;
            case Rarity::Rare:      name += rare_weapons[roll(rng)]; break;
            case Rarity::Epic:      name += epic_weapons[roll(rng)]; break;
            case Rarity::Legendary: name += legendary_weapons[roll(rng)]; break;
        }
        
        return name;
    }
    
    // Generate armor name
    std::string generate_armor_name(Rarity rarity, ItemAffix affix, EquipmentSlot slot) {
        std::string name;
        
        // Add affix prefix
        if (affix != ItemAffix::NONE && static_cast<int>(affix) >= 8) {
            name = armor_affix_prefix[static_cast<int>(affix)];
        }
        
        // Add base name
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> roll(0, 2);
        
        switch (rarity) {
            case Rarity::Common:    name += common_armor[roll(rng)]; break;
            case Rarity::Uncommon:  name += uncommon_armor[roll(rng)]; break;
            case Rarity::Rare:      name += rare_armor[roll(rng)]; break;
            case Rarity::Epic:      name += epic_armor[roll(rng)]; break;
            case Rarity::Legendary: name += legendary_armor[roll(rng)]; break;
        }
        
        // Suppress unused parameter warning
        (void)slot;
        
        return name;
    }
    
    // Generate a weapon
    Item generate_weapon(int depth, std::mt19937& rng) {
        Item weapon;
        weapon.type = ItemType::Weapon;
        weapon.isEquippable = true;
        weapon.slot = EquipmentSlot::Weapon;
        
        // Roll rarity
        weapon.rarity = roll_rarity(depth, rng);
        
        // Roll affix
        weapon.affix = roll_affix(weapon.rarity, weapon.type, rng);
        weapon.affixStrength = get_affix_strength(weapon.rarity, rng);
        
        // Generate name
        weapon.name = generate_weapon_name(weapon.rarity, weapon.affix);
        
        // Calculate stats based on rarity and depth
        int baseAtk = 2 + depth / 2;
        switch (weapon.rarity) {
            case Rarity::Common:    weapon.attackBonus = baseAtk; break;
            case Rarity::Uncommon:  weapon.attackBonus = baseAtk + 1; break;
            case Rarity::Rare:      weapon.attackBonus = baseAtk + 2; break;
            case Rarity::Epic:      weapon.attackBonus = baseAtk + 4; break;
            case Rarity::Legendary: weapon.attackBonus = baseAtk + 6; break;
        }
        
        LOG_DEBUG("Generated weapon: " + weapon.name + " (ATK+" + std::to_string(weapon.attackBonus) + ")");
        return weapon;
    }
    
    // Generate armor
    Item generate_armor(int depth, std::mt19937& rng) {
        Item armor;
        armor.type = ItemType::Armor;
        armor.isEquippable = true;
        
        // Random slot
        std::uniform_int_distribution<int> slotRoll(0, 2);
        switch (slotRoll(rng)) {
            case 0: armor.slot = EquipmentSlot::Head; break;
            case 1: armor.slot = EquipmentSlot::Chest; break;
            case 2: armor.slot = EquipmentSlot::Offhand; break;
        }
        
        // Roll rarity
        armor.rarity = roll_rarity(depth, rng);
        
        // Roll affix
        armor.affix = roll_affix(armor.rarity, armor.type, rng);
        armor.affixStrength = get_affix_strength(armor.rarity, rng);
        
        // Generate name
        armor.name = generate_armor_name(armor.rarity, armor.affix, armor.slot);
        
        // Calculate stats based on rarity and depth
        int baseDef = 1 + depth / 3;
        switch (armor.rarity) {
            case Rarity::Common:    armor.defenseBonus = baseDef; break;
            case Rarity::Uncommon:  armor.defenseBonus = baseDef + 1; break;
            case Rarity::Rare:      armor.defenseBonus = baseDef + 2; break;
            case Rarity::Epic:      armor.defenseBonus = baseDef + 3; break;
            case Rarity::Legendary: armor.defenseBonus = baseDef + 5; break;
        }
        
        LOG_DEBUG("Generated armor: " + armor.name + " (DEF+" + std::to_string(armor.defenseBonus) + ")");
        return armor;
    }
    
    // Generate consumable
    Item generate_consumable(int depth, std::mt19937& rng) {
        Item consumable;
        consumable.type = ItemType::Consumable;
        consumable.isConsumable = true;
        
        // Roll type
        std::uniform_int_distribution<int> typeRoll(0, 100);
        int r = typeRoll(rng);
        
        if (r < 50) {
            // Health potion
            consumable.name = "Health Potion";
            consumable.healAmount = 10 + depth * 2;
            consumable.rarity = (depth > 5) ? Rarity::Uncommon : Rarity::Common;
        } else if (r < 75) {
            // Greater health potion
            consumable.name = "Greater Health Potion";
            consumable.healAmount = 25 + depth * 3;
            consumable.rarity = Rarity::Rare;
        } else if (r < 90) {
            // Fortify potion
            consumable.name = "Potion of Fortitude";
            consumable.onUseStatus = StatusType::Fortify;
            consumable.onUseDuration = 5;
            consumable.onUseMagnitude = 50;
            consumable.rarity = Rarity::Uncommon;
        } else {
            // Haste potion
            consumable.name = "Potion of Haste";
            consumable.onUseStatus = StatusType::Haste;
            consumable.onUseDuration = 5;
            consumable.onUseMagnitude = 5;
            consumable.rarity = Rarity::Rare;
        }
        
        return consumable;
    }
    
    // Generate a random item
    Item generate_item(int depth, std::mt19937& rng) {
        std::uniform_int_distribution<int> typeRoll(0, 100);
        int r = typeRoll(rng);
        
        if (r < 35) {
            return generate_weapon(depth, rng);
        } else if (r < 65) {
            return generate_armor(depth, rng);
        } else {
            return generate_consumable(depth, rng);
        }
    }
    
    // Generate enemy drops
    std::vector<Item> generate_enemy_drops(EnemyType enemy, int depth, std::mt19937& rng) {
        std::vector<Item> drops;
        std::uniform_int_distribution<int> dropRoll(0, 100);
        
        // Base drop chance
        int dropChance = 30 + depth * 2;  // 30% base, +2% per floor
        
        // Stronger enemies have better drop rates
        switch (enemy) {
            case EnemyType::Rat:
            case EnemyType::Spider:
                dropChance -= 10;
                break;
            case EnemyType::Dragon:
            case EnemyType::Lich:
            case EnemyType::StoneGolem:
            case EnemyType::ShadowLord:
                dropChance = 100;  // Bosses always drop
                break;
            default:
                break;
        }
        
        if (dropRoll(rng) < dropChance) {
            drops.push_back(generate_item(depth, rng));
        }
        
        return drops;
    }
    
    // Generate treasure room loot
    std::vector<Item> generate_treasure_room_loot(int depth, std::mt19937& rng) {
        std::vector<Item> loot;
        
        // Treasure rooms have 3x items with better rarity
        for (int i = 0; i < 3; ++i) {
            loot.push_back(generate_item(depth + 2, rng));  // +2 depth for better loot
        }
        
        return loot;
    }
    
    // Generate boss loot
    std::vector<Item> generate_boss_loot(EnemyType boss, int depth, std::mt19937& rng) {
        std::vector<Item> loot;
        
        // Bosses drop guaranteed legendary
        Item legendary = generate_weapon(depth + 5, rng);  // Very high depth for legendary
        legendary.rarity = Rarity::Legendary;
        legendary.affix = roll_affix(Rarity::Legendary, ItemType::Weapon, rng);
        legendary.affixStrength = get_affix_strength(Rarity::Legendary, rng);
        legendary.name = generate_weapon_name(Rarity::Legendary, legendary.affix);
        legendary.attackBonus = 10 + depth;
        
        loot.push_back(legendary);
        
        // Also drop some consumables
        loot.push_back(generate_consumable(depth, rng));
        loot.push_back(generate_consumable(depth, rng));
        
        // Suppress unused parameter warning
        (void)boss;
        
        return loot;
    }
}


