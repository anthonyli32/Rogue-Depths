#pragma once

#include <string>
#include <vector>
#include "player.h"
#include "enemy.h"
#include "ui.h"
#include "types.h"

// Forward declaration
class Dungeon;

// Action metadata for combat system
struct CombatActionContext {
    CombatAction action;
    CombatDistance minDistance;  // Minimum distance required
    int cooldownTurns;           // Cooldown turns (replaces energy cost)
    float executionTime;           // Duration (affects turn order)
    bool isTelegraphed;           // Can enemy see it coming?
    std::string description;       // Visual flavor text
    float baseDamage;             // Damage multiplier (1.0 = normal)
    StatusType statusEffect;      // Effect applied if any
    int cooldown;                 // Turns before can use again (0 = always available)
    bool requiresWeapon;           // Needs weapon equipped
    bool requiresRanged;           // Needs ranged weapon
};

// Combat context for tracking combat state
struct CombatContext {
    CombatAction action;
    int targetIndex;          // For ranged/skills targeting
    int consumableUsedIndex = -1; // Index in inventory for consumable action
    bool wasSuccessful;       // Result of action
    bool isDefending;         // Player is in defensive stance
    int skillCooldown;        // Turns until skill available
    
    // 3D positioning for tactical combat
    Position3D playerPos;
    Position3D enemyPos;
    CombatDistance currentDistance;
};

namespace combat {
    // Calculate combat distance between two 3D positions
    CombatDistance calculate_combat_distance(const Position3D& from, const Position3D& to);
    
    // Convert raw distance value to CombatDistance category
    CombatDistance distance_to_category(int rawDistance);
    
    // Get action context metadata
    const CombatActionContext& get_action_context(CombatAction action);
    
    // Get available actions based on distance and equipment
    std::vector<CombatAction> get_available_actions(const Player& player, 
                                                     CombatDistance distance);
    
    // Distance-based damage modifier
    float get_distance_damage_modifier(CombatDistance distance);
    
    // Distance-based accuracy (hit chance)
    int get_hit_chance(CombatDistance distance);
    
    // Resolve combat movement - updates both positions and recalculates distance
    void resolve_combat_movement(Position3D& playerPos, Position3D& enemyPos,
                                CombatAction playerAction, CombatAction enemyAction,
                                CombatDistance& currentDistance, MessageLog& log,
                                const Dungeon& dungeon, const CombatArena* arena = nullptr);
    
    // Legacy function for backward compatibility
    void update_combat_position(Position3D& pos, CombatAction action, CombatDistance& currentDistance);
    
    // Standard melee attack (can only hit grounded enemies)
    void melee(Player& player, Enemy& enemy, MessageLog& log, 
               CombatDistance distance = CombatDistance::MELEE);
    
    // Ranged attack (can hit any height)
    void ranged(Player& player, Enemy& enemy, MessageLog& log,
                CombatDistance distance = CombatDistance::CLOSE);
    
    // Check if attack type can hit enemy at given height
    bool can_hit(AttackType attack, HeightLevel height);
    
    // Get attack type based on equipped weapon
    AttackType get_player_attack_type(const Player& player);
    
    // Combat action menu system
    CombatAction show_combat_menu(const Player& player, const std::vector<Enemy>& enemies,
                                   int screenRow, int screenCol, bool hasRangedWeapon,
                                   CombatDistance currentDistance,
                                   const Position3D& playerPos, const Position3D& enemyPos,
                                   const CombatArena* arena, const MessageLog& log);
    
    // Execute player's chosen combat action
    void execute_action(Player& player, std::vector<Enemy>& enemies, 
                        CombatContext& ctx, MessageLog& log, const Dungeon& dungeon,
                        const CombatArena* arena = nullptr);
    
    // Perform defensive stance (reduces incoming damage)
    void perform_defensive_stance(Player& player, MessageLog& log);
    
    // Perform class-specific ability
    void perform_class_ability(Player& player, Enemy& enemy, MessageLog& log);
    
    // Use consumable item during combat
    void use_consumable_in_combat(Player& player, Item& item, MessageLog& log);
    
    // Attempt to retreat from combat
    bool perform_retreat(Player& player, const Enemy& enemy, MessageLog& log);
    
    // Enter tactical combat mode - full combat loop with menu
    // Returns: true if player won/retreated, false if player died
    bool enter_combat_mode(Player& player, Enemy& enemy, Dungeon& dungeon, MessageLog& log);
    
    // Apply weapon affixes during combat
    void apply_weapon_affixes(const Item& weapon, Enemy& target, Player& attacker, MessageLog& log);
    
    // Apply armor affixes when taking damage
    int apply_armor_affixes(const Item& armor, int incomingDamage, Player& wearer, MessageLog& log);
}


