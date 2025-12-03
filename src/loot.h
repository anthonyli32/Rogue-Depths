#pragma once

#include <vector>
#include <random>
#include "entity.h"
#include "types.h"

namespace loot {
    // Generate a random item based on floor depth
    Item generate_item(int depth, std::mt19937& rng);
    
    // Generate a weapon with potential affixes
    Item generate_weapon(int depth, std::mt19937& rng);
    
    // Generate armor with potential affixes
    Item generate_armor(int depth, std::mt19937& rng);
    
    // Generate a consumable
    Item generate_consumable(int depth, std::mt19937& rng);
    
    // Determine rarity based on depth
    Rarity roll_rarity(int depth, std::mt19937& rng);
    
    // Roll for an affix based on rarity
    ItemAffix roll_affix(Rarity rarity, ItemType type, std::mt19937& rng);
    
    // Get affix strength multiplier based on rarity
    float get_affix_strength(Rarity rarity, std::mt19937& rng);
    
    // Generate loot drop from enemy death
    std::vector<Item> generate_enemy_drops(EnemyType enemy, int depth, std::mt19937& rng);
    
    // Generate treasure room loot
    std::vector<Item> generate_treasure_room_loot(int depth, std::mt19937& rng);
    
    // Generate boss loot (guaranteed legendary)
    std::vector<Item> generate_boss_loot(EnemyType boss, int depth, std::mt19937& rng);
    
    // Item name generation
    std::string generate_weapon_name(Rarity rarity, ItemAffix affix);
    std::string generate_armor_name(Rarity rarity, ItemAffix affix, EquipmentSlot slot);
}


