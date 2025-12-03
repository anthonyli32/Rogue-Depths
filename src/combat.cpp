    /**
     * @brief Executes the player's chosen combat action.
     * @param player The player performing the action.
     * @param enemies The list of enemies in combat.
     * @param ctx The current combat context (action, positions, etc.).
     * @param log The message log for combat output.
     * @param dungeon The current dungeon state.
     * @param arena Optional combat arena pointer.
     */
    /**
     * @brief Handles the POWER_STRIKE combat action (heavy melee attack).
     * @param player The player performing the action.
     * @param target The enemy being attacked.
     * @param ctx The current combat context.
     * @param log The message log for combat output.
     * @param applyTelegraphModifier Function to apply telegraph penalty if needed.
     */
    /**
     * @brief Handles the TACKLE combat action (melee knockdown with stun).
     * @param player The player performing the action.
     * @param target The enemy being attacked.
     * @param ctx The current combat context.
     * @param log The message log for combat output.
     */
    /**
     * @brief Handles the WHIRLWIND combat action (AOE melee attack for Warriors).
     * @param player The player performing the action.
     * @param enemies The list of enemies in combat.
     * @param ctx The current combat context.
     * @param log The message log for combat output.
     */
    /**
     * @brief Handles the SNIPE combat action (precision ranged attack for Rogues).
     * @param player The player performing the action.
     * @param target The enemy being attacked.
     * @param ctx The current combat context.
     * @param log The message log for combat output.
     * @param applyTelegraphModifier Function to apply telegraph penalty if needed.
     */
    /**
     * @brief Handles the MULTISHOT combat action (multi-target ranged attack for Rogues).
     * @param player The player performing the action.
     * @param enemies The list of enemies in combat.
     * @param ctx The current combat context.
     * @param log The message log for combat output.
     * @param applyTelegraphModifier Function to apply telegraph penalty if needed.
     */
    /**
     * @brief Handles the FIREBALL combat action (AOE magic attack for Mages).
     * @param player The player performing the action.
     * @param target The enemy being attacked.
     * @param ctx The current combat context.
     * @param log The message log for combat output.
     * @param applyTelegraphModifier Function to apply telegraph penalty if needed.
     */
    /**
     * @brief Handles the FROST_BOLT combat action (magic freeze attack for Mages).
     * @param player The player performing the action.
     * @param target The enemy being attacked.
     * @param ctx The current combat context.
     * @param log The message log for combat output.
     * @param applyTelegraphModifier Function to apply telegraph penalty if needed.
     */
    /**
     * @brief Handles the TELEPORT combat action (Mage reposition ability).
     * @param player The player performing the action.
     * @param ctx The current combat context.
     * @param log The message log for combat output.
     */
#include "combat.h"
#include "logger.h"
#include "ui.h"
#include "input.h"
#include "glyphs.h"
#include "constants.h"
#include "dungeon.h"
#include "ai.h"
// IMPROVED: constants.h already included, game_constants namespace available

#include <iostream>
#include <algorithm>
#include <random>
#include <map>
#include <unordered_set>
#include <functional>
#include <thread>
#include <chrono>
#include <cctype>

namespace combat {
    static std::mt19937& combat_rng() {
        static std::mt19937 rng(std::random_device{}());
        return rng;
    }

    static bool roll_percentage(int percent) {
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(combat_rng()) <= percent;
    }
    static const std::vector<char> kActionHotkeys = {
        '1','2','3','4','5','6','7','8','9','0','-','='
    };

    static const std::map<CombatAction, std::string> kActionNames = {
        {CombatAction::SLASH, "Slash"},
        {CombatAction::POWER_STRIKE, "Power Strike"},
        {CombatAction::TACKLE, "Tackle"},
        {CombatAction::WHIRLWIND, "Whirlwind"},
        {CombatAction::SHOOT, "Shoot"},
        {CombatAction::SNIPE, "Snipe"},
        {CombatAction::MULTISHOT, "Multishot"},
        {CombatAction::FIREBALL, "Fireball"},
        {CombatAction::FROST_BOLT, "Frost Bolt"},
        {CombatAction::TELEPORT, "Teleport"},
        {CombatAction::ADVANCE, "Advance"},
        {CombatAction::RETREAT, "Retreat"},
        {CombatAction::CIRCLE, "Circle"},
        {CombatAction::REPOSITION, "Reposition"},
        {CombatAction::DEFEND, "Defend"},
        {CombatAction::BRACE, "Brace"},
        {CombatAction::CONSUMABLE, "Consumable"},
        {CombatAction::WAIT, "Wait"},
        {CombatAction::SKILL, "Class Ability"},
        {CombatAction::ATTACK, "Attack"},
        {CombatAction::RANGED, "Ranged"}
    };

    static const std::map<CombatAction, std::string> kActionGlyphs = {
        {CombatAction::SLASH, "⚔"},
        {CombatAction::POWER_STRIKE, "💥"},
        {CombatAction::TACKLE, "🤼"},
        {CombatAction::WHIRLWIND, "🌪"},
        {CombatAction::SHOOT, "🏹"},
        {CombatAction::SNIPE, "🎯"},
        {CombatAction::MULTISHOT, "➹"},
        {CombatAction::FIREBALL, "🔥"},
        {CombatAction::FROST_BOLT, "❄"},
        {CombatAction::TELEPORT, "✨"},
        {CombatAction::ADVANCE, "↑"},
        {CombatAction::RETREAT, "↓"},
        {CombatAction::CIRCLE, "↻"},
        {CombatAction::REPOSITION, "↔"},
        {CombatAction::DEFEND, "🛡"},
        {CombatAction::BRACE, "⛨"},
        {CombatAction::CONSUMABLE, "🧪"},
        {CombatAction::WAIT, "⏸"},
        {CombatAction::SKILL, "⭐"},
        {CombatAction::ATTACK, "⚔"},
        {CombatAction::RANGED, "🏹"}
    };

    // Simplified combat: Only core actions
    static const std::vector<CombatAction> kCategoryMelee = {
        CombatAction::SLASH,        // Basic attack (short CD)
        CombatAction::POWER_STRIKE  // Special attack (long CD, more damage)
    };

    // Ranged and Magic categories removed - combat simplified
    static const std::vector<CombatAction> kCategoryRanged = {
        // All ranged actions removed
    };

    static const std::vector<CombatAction> kCategoryMagic = {
        CombatAction::FIREBALL,
        CombatAction::FROST_BOLT,
    };

    // Movement actions removed - combat simplified
    static const std::vector<CombatAction> kCategoryMovement = {
        // All movement actions removed
    };

    static const std::vector<CombatAction> kCategoryDefense = {
        CombatAction::DEFEND
    };

    static const std::vector<CombatAction> kCategoryUtility = {
        CombatAction::CONSUMABLE,
        CombatAction::WAIT  // Keep as fallback
    };

    // New category for class abilities
    static const std::vector<CombatAction> kCategoryAbilities = {
        CombatAction::SKILL  // Class ability
    };

    static const std::map<CombatAction, CombatActionContext> actionDatabase = {
        // Melee actions - cooldowns based on damage: 1.0x=0, 1.2x=1, 1.5x=2, 2.0x+=3
        // Nerfed weapon damage by ~25% (only weapon-based attacks, not class ability)
        {CombatAction::SLASH, {CombatAction::SLASH, CombatDistance::MELEE, 0, 0.6f, false,
                              "Quick melee strike", 1.0f, StatusType::None, 0, true, false}},  // Nerfed from 0.8f to 0.6f
        {CombatAction::POWER_STRIKE, {CombatAction::POWER_STRIKE, CombatDistance::MELEE, 0, 1.15f, true,
                                      "Heavy melee attack", 1.5f, StatusType::None, 2, true, false}},  // Nerfed from 1.5f to 1.15f
        {CombatAction::TACKLE, {CombatAction::TACKLE, CombatDistance::MELEE, 0, 0.75f, false,
                               "Knockdown attack", 0.8f, StatusType::Stun, 1, true, false}},  // Nerfed from 1.0f to 0.75f
        {CombatAction::WHIRLWIND, {CombatAction::WHIRLWIND, CombatDistance::MELEE, 0, 0.9f, false,
                                  "AOE melee attack", 0.7f, StatusType::None, 1, true, false}},  // Nerfed from 1.2f to 0.9f
        
        // Ranged actions
        {CombatAction::SHOOT, {CombatAction::SHOOT, CombatDistance::CLOSE, 0, 1.0f, true,
                              "Standard ranged shot", 1.0f, StatusType::None, 0, false, true}},  // 1.0x = 0 cooldown
        {CombatAction::SNIPE, {CombatAction::SNIPE, CombatDistance::MEDIUM, 0, 2.5f, true,
                              "Aimed precision shot", 1.5f, StatusType::None, 2, false, true}},  // 1.5x = 2 cooldown
        {CombatAction::MULTISHOT, {CombatAction::MULTISHOT, CombatDistance::FAR, 0, 1.8f, true,
                                  "Multiple arrow volley", 0.8f, StatusType::None, 1, false, true}},  // 0.8x per target, 1 cooldown
        
        // Magic actions
        {CombatAction::FIREBALL, {CombatAction::FIREBALL, CombatDistance::CLOSE, 0, 2.0f, true,
                                 "AOE fire damage + burn", 1.2f, StatusType::None, 1, false, false}},  // 1.2x = 1 cooldown
        {CombatAction::FROST_BOLT, {CombatAction::FROST_BOLT, CombatDistance::MELEE, 0, 1.8f, false,
                                   "Freeze enemy", 1.0f, StatusType::None, 0, false, false}},  // 1.0x = 0 cooldown
        {CombatAction::TELEPORT, {CombatAction::TELEPORT, CombatDistance::MELEE, 0, 1.5f, false,
                                 "Reposition anywhere", 0.0f, StatusType::None, 1, false, false}},  // Utility = 1 cooldown
        
        // Movement actions (removed, but kept for compatibility)
        {CombatAction::ADVANCE, {CombatAction::ADVANCE, CombatDistance::MELEE, 0, 0.5f, false,
                                "Move closer", 0.0f, StatusType::None, 0, false, false}},
        {CombatAction::RETREAT, {CombatAction::RETREAT, CombatDistance::MELEE, 0, 0.5f, false,
                                "Move away", 0.0f, StatusType::None, 0, false, false}},
        {CombatAction::CIRCLE, {CombatAction::CIRCLE, CombatDistance::MELEE, 0, 0.5f, false,
                               "Strafe left/right", 0.0f, StatusType::None, 0, false, false}},
        {CombatAction::REPOSITION, {CombatAction::REPOSITION, CombatDistance::MELEE, 0, 0.5f, false,
                                   "Move to adjacent tile", 0.0f, StatusType::None, 0, false, false}},
        
        // Defensive actions
        {CombatAction::DEFEND, {CombatAction::DEFEND, CombatDistance::MELEE, 0, 0.3f, false,
                               "Hunker down", 0.0f, StatusType::None, 0, false, false}},  // No cooldown
        {CombatAction::BRACE, {CombatAction::BRACE, CombatDistance::MELEE, 0, 0.4f, false,
                              "Prepare for impact", 0.0f, StatusType::None, 0, false, false}},  // No cooldown
        
        // Utility actions
        {CombatAction::CONSUMABLE, {CombatAction::CONSUMABLE, CombatDistance::MELEE, 0, 0.5f, false,
                                   "Use consumable item", 0.0f, StatusType::None, 0, false, false}},  // No cooldown
        {CombatAction::WAIT, {CombatAction::WAIT, CombatDistance::MELEE, 0, 0.0f, false,
                             "Pass turn", 0.0f, StatusType::None, 0, false, false}},  // No cooldown
        
        // Class ability (SKILL) - class-specific attack, no cooldown
        // Note: Display name/description is overridden in show_combat_menu based on player class
        {CombatAction::SKILL, {CombatAction::SKILL, CombatDistance::MELEE, 0, 1.0f, false,
                              "Class ability", 1.0f, StatusType::None, 0, false, false}},  // 1.0x = 0 cooldown
        
        // Legacy mappings
        {CombatAction::ATTACK, {CombatAction::SLASH, CombatDistance::MELEE, 20, 0.8f, false,
                               "Melee attack", 1.0f, StatusType::None, 0, true, false}},
        {CombatAction::RANGED, {CombatAction::SHOOT, CombatDistance::CLOSE, 50, 1.0f, true,
                              "Ranged attack", 1.0f, StatusType::None, 0, false, true}}
    };
    
    // Get action context metadata
    const CombatActionContext& get_action_context(CombatAction action) {
        auto it = actionDatabase.find(action);
        if (it != actionDatabase.end()) {
            return it->second;
        }
        // Default fallback
        static const CombatActionContext defaultContext = {
            CombatAction::WAIT, CombatDistance::MELEE, 0, 0.0f, false,
            "Unknown action", 0.0f, StatusType::None, 0, false, false
        };
        return defaultContext;
    }
    
    // Get available actions based on distance, energy, and equipment
    std::vector<CombatAction> get_available_actions(const Player& player, 
                                                     CombatDistance distance) {
        std::vector<CombatAction> available;
        
        // Check if player has ranged weapon
        bool hasRanged = false;
        auto weaponIt = player.get_equipment().find(EquipmentSlot::Weapon);
        auto offhandIt = player.get_equipment().find(EquipmentSlot::Offhand);
        if (weaponIt != player.get_equipment().end()) {
            // Simple check - could be enhanced
            hasRanged = (weaponIt->second.type == ItemType::Weapon);
        }
        
        // Mages cannot use melee weapon attacks - they only use Magic Sword and magic/ranged attacks
        bool isMage = (player.player_class() == PlayerClass::Mage);
        // Check both Weapon and Offhand slots for weapon
        bool hasWeapon = false;
        if (weaponIt != player.get_equipment().end() && weaponIt->second.type == ItemType::Weapon) {
            hasWeapon = true;
        }
        if (offhandIt != player.get_equipment().end() && offhandIt->second.type == ItemType::Weapon) {
            hasWeapon = true;
        }
        bool hasRareWeapon = false;
        if (hasWeapon) {
            if (weaponIt != player.get_equipment().end() && weaponIt->second.rarity >= Rarity::Rare) {
                hasRareWeapon = true;
            }
            if (offhandIt != player.get_equipment().end() && offhandIt->second.type == ItemType::Weapon && 
                offhandIt->second.rarity >= Rarity::Rare) {
                hasRareWeapon = true;
            }
        }
        
        // For mages: ANY weapon unlocks Magic Sword and Sword Casting
        if (isMage && hasWeapon) {
            // Add Magic Sword (uses FIREBALL action but renamed in display)
            if (!player.is_on_cooldown(CombatAction::FIREBALL)) {
                available.push_back(CombatAction::FIREBALL);
            }
            // Add Sword Casting (uses FROST_BOLT action but renamed in display)
            if (!player.is_on_cooldown(CombatAction::FROST_BOLT)) {
                available.push_back(CombatAction::FROST_BOLT);
            }
        }
        
        // Check each action (mana system removed)
        for (const auto& pair : actionDatabase) {
            const auto& ctx = pair.second;
            
            // Skip legacy actions in new system
            if (pair.first == CombatAction::ATTACK || pair.first == CombatAction::RANGED) {
                continue;
            }
            
            // Mages cannot use melee weapon attacks (SLASH, POWER_STRIKE, TACKLE, WHIRLWIND)
            if (isMage && (pair.first == CombatAction::SLASH || 
                          pair.first == CombatAction::POWER_STRIKE ||
                          pair.first == CombatAction::TACKLE ||
                          pair.first == CombatAction::WHIRLWIND)) {
                continue;
            }
            
            // Skip FIREBALL and FROST_BOLT for mages with weapons - already added above
            // But allow FROST_BOLT if mage has no weapon (it's their basic attack via SKILL)
            if (isMage && hasWeapon && (pair.first == CombatAction::FIREBALL || pair.first == CombatAction::FROST_BOLT)) {
                continue;
            }
            
            // Distance requirements removed - all attacks work at any distance
            
            // Check weapon requirements
            if (ctx.requiresWeapon && weaponIt == player.get_equipment().end()) continue;
            if (ctx.requiresRanged && !hasRanged) continue;
            
            // Mana system removed - no mana checks needed
            
            // Check cooldown
            if (player.is_on_cooldown(pair.first)) continue;
            
            available.push_back(pair.first);
        }
        
        return available;
    }
    
    // Distance-based damage modifier - REMOVED: All attacks do full damage at any distance
    float get_distance_damage_modifier(CombatDistance /*distance*/) {
        return 1.0f;  // Always 100% damage regardless of distance
    }
    
    // Distance-based accuracy (hit chance percentage)
    int get_hit_chance(CombatDistance distance) {
        using namespace combat_balance;
        switch (distance) {
            case CombatDistance::MELEE: return ACCURACY_MELEE;
            case CombatDistance::CLOSE: return ACCURACY_CLOSE;
            case CombatDistance::MEDIUM: return ACCURACY_MEDIUM;
            case CombatDistance::FAR: return ACCURACY_FAR;
            case CombatDistance::EXTREME: return ACCURACY_EXTREME;
            default: return ACCURACY_MEDIUM;
        }
    }
    
    // Convert raw distance value to CombatDistance category
    CombatDistance distance_to_category(int rawDistance) {
        if (rawDistance <= 1) {
            return CombatDistance::MELEE;
        } else if (rawDistance <= 3) {
            return CombatDistance::CLOSE;
        } else if (rawDistance <= 6) {
            return CombatDistance::MEDIUM;
        } else if (rawDistance <= 10) {
            return CombatDistance::FAR;
        } else {
            return CombatDistance::EXTREME;
        }
    }
    
    // Calculate combat distance between two 3D positions
    CombatDistance calculate_combat_distance(const Position3D& from, const Position3D& to) {
        int rawDistance = from.calculate_distance(to);
        return distance_to_category(rawDistance);
    }
    
    // Get attack type based on equipped weapon
    AttackType get_player_attack_type(const Player& player) {
        auto equipment = player.get_equipment();
        // Check main hand first, then offhand
        auto weaponIt = equipment.find(EquipmentSlot::Weapon);
        if (weaponIt == equipment.end()) {
            weaponIt = equipment.find(EquipmentSlot::Offhand);
        }
        
        if (weaponIt == equipment.end()) {
            // No weapon equipped - default to melee
            return AttackType::Melee;
        }
        
        const Item& weapon = weaponIt->second;
        std::string weaponName = weapon.name;
        
        // Convert to lowercase for comparison
        std::transform(weaponName.begin(), weaponName.end(), weaponName.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        
        // Check for ranged weapon keywords
        if (weaponName.find("bow") != std::string::npos ||
            weaponName.find("arrow") != std::string::npos ||
            weaponName.find("crossbow") != std::string::npos ||
            weaponName.find("ranged") != std::string::npos) {
            return AttackType::Ranged;
        }
        
        // Check for magic weapon keywords (staff, wand, etc.)
        if (weaponName.find("staff") != std::string::npos ||
            weaponName.find("wand") != std::string::npos ||
            weaponName.find("spell") != std::string::npos ||
            weaponName.find("magic") != std::string::npos) {
            return AttackType::Magic;
        }
        
        // Default to melee
        return AttackType::Melee;
    }
    
    // Get weapon-based attacks unlocked by equipped weapons (checks both main hand and offhand)
    std::vector<CombatAction> get_weapon_attacks(const Player& player) {
        std::vector<CombatAction> attacks;
        auto equipment = player.get_equipment();
        bool isMage = (player.player_class() == PlayerClass::Mage);
        
        // Check both Weapon and Offhand slots
        std::vector<const Item*> weapons;
        auto mainHand = equipment.find(EquipmentSlot::Weapon);
        if (mainHand != equipment.end() && mainHand->second.type == ItemType::Weapon) {
            weapons.push_back(&mainHand->second);
        }
        auto offHand = equipment.find(EquipmentSlot::Offhand);
        if (offHand != equipment.end() && offHand->second.type == ItemType::Weapon) {
            weapons.push_back(&offHand->second);
        }
        
        // For mages: ANY weapon unlocks magic attacks
        if (isMage && !weapons.empty()) {
            // All weapons unlock FIREBALL for mages
            if (std::find(attacks.begin(), attacks.end(), CombatAction::FIREBALL) == attacks.end()) {
                attacks.push_back(CombatAction::FIREBALL);
            }
            // Check if any weapon is Rare+ to unlock FROST_BOLT
            bool hasRareWeapon = false;
            for (const Item* weapon : weapons) {
                if (weapon && weapon->rarity >= Rarity::Rare) {
                    hasRareWeapon = true;
                    break;
                }
            }
            if (hasRareWeapon) {
                if (std::find(attacks.begin(), attacks.end(), CombatAction::FROST_BOLT) == attacks.end()) {
                    attacks.push_back(CombatAction::FROST_BOLT);
                }
            }
        }
        
        for (const Item* weapon : weapons) {
            if (!weapon) continue;
            
            std::string weaponName = weapon->name;
            std::transform(weaponName.begin(), weaponName.end(), weaponName.begin(), 
                          [](unsigned char c) { return std::tolower(c); });
            
            // Skip melee weapon unlocks for mages (they use magic attacks instead)
            if (isMage) {
                continue;
            }
            
            // Melee weapons (sword, axe, hammer, club, dagger, knife, blade)
            if (weaponName.find("sword") != std::string::npos ||
                weaponName.find("axe") != std::string::npos ||
                weaponName.find("hammer") != std::string::npos ||
                weaponName.find("club") != std::string::npos ||
                weaponName.find("dagger") != std::string::npos ||
                weaponName.find("knife") != std::string::npos ||
                weaponName.find("blade") != std::string::npos) {
                // All melee weapons unlock POWER_STRIKE
                if (std::find(attacks.begin(), attacks.end(), CombatAction::POWER_STRIKE) == attacks.end()) {
                    attacks.push_back(CombatAction::POWER_STRIKE);
                }
                // Rare+ melee weapons unlock TACKLE
                if (weapon->rarity >= Rarity::Rare) {
                    if (std::find(attacks.begin(), attacks.end(), CombatAction::TACKLE) == attacks.end()) {
                        attacks.push_back(CombatAction::TACKLE);
                    }
                }
            }
            
            // Ranged weapons (bow, crossbow, arrow)
            if (weaponName.find("bow") != std::string::npos ||
                weaponName.find("crossbow") != std::string::npos ||
                weaponName.find("arrow") != std::string::npos) {
                // All ranged weapons unlock SHOOT
                if (std::find(attacks.begin(), attacks.end(), CombatAction::SHOOT) == attacks.end()) {
                    attacks.push_back(CombatAction::SHOOT);
                }
                // Rare+ ranged weapons unlock SNIPE
                if (weapon->rarity >= Rarity::Rare) {
                    if (std::find(attacks.begin(), attacks.end(), CombatAction::SNIPE) == attacks.end()) {
                        attacks.push_back(CombatAction::SNIPE);
                    }
                }
            }
            
            // Magic weapons (staff, wand, spell) - for non-mages
            if (weaponName.find("staff") != std::string::npos ||
                weaponName.find("wand") != std::string::npos ||
                weaponName.find("spell") != std::string::npos) {
                // All magic weapons unlock FIREBALL
                if (std::find(attacks.begin(), attacks.end(), CombatAction::FIREBALL) == attacks.end()) {
                    attacks.push_back(CombatAction::FIREBALL);
                }
                // Rare+ magic weapons unlock FROST_BOLT
                if (weapon->rarity >= Rarity::Rare) {
                    if (std::find(attacks.begin(), attacks.end(), CombatAction::FROST_BOLT) == attacks.end()) {
                        attacks.push_back(CombatAction::FROST_BOLT);
                    }
                }
            }
        }
        
        return attacks;
    }
    
    // Resolve combat movement - updates both positions and recalculates distance
    void resolve_combat_movement(Position3D& playerPos, Position3D& enemyPos,
                                CombatAction playerAction, CombatAction /*enemyAction*/,
                                CombatDistance& currentDistance, MessageLog& log,
                                const Dungeon& dungeon, const CombatArena* /*arena*/) {
        using namespace combat_balance;
        
        CombatDistance oldDistance = currentDistance;

        // --- Bidirectional Position Update ---
        // Apply player movement
        switch (playerAction) {
            case CombatAction::ADVANCE:
                playerPos.depth = std::max(DEPTH_MIN, playerPos.depth - ADVANCE_DISTANCE);
                break;
            case CombatAction::RETREAT:
                playerPos.depth = std::min(DEPTH_MAX, playerPos.depth + RETREAT_DISTANCE);
                break;
            case CombatAction::CIRCLE: {
                int direction = (rand() % 2 == 0) ? -1 : 1;
                int newX = playerPos.x + direction;
                if (dungeon.in_bounds(newX, playerPos.y) && dungeon.is_walkable(newX, playerPos.y)) {
                    playerPos.x = newX;
                    log.add(MessageType::Info, glyphs::arrow_right() + std::string(" You circle around!"));
                } else {
                    newX = playerPos.x - direction;
                    if (dungeon.in_bounds(newX, playerPos.y) && dungeon.is_walkable(newX, playerPos.y)) {
                        playerPos.x = newX;
                        log.add(MessageType::Info, glyphs::arrow_right() + std::string(" You circle around!"));
                    } else {
                        log.add(MessageType::Warning, "No room to circle!");
                    }
                }
                break;
            }
            case CombatAction::REPOSITION: {
                int dx = (rand() % 3) - 1;
                int dy = (rand() % 3) - 1;
                if (dx == 0 && dy == 0) {
                    dx = (rand() % 2 == 0) ? -1 : 1;
                }
                int newX = playerPos.x + dx;
                int newY = playerPos.y + dy;
                if (dungeon.in_bounds(newX, newY) && dungeon.is_walkable(newX, newY)) {
                    playerPos.x = newX;
                    playerPos.y = newY;
                    log.add(MessageType::Info, glyphs::arrow_down() + std::string(" You reposition!"));
                } else {
                    log.add(MessageType::Warning, "Cannot reposition there!");
                }
                break;
            }
            default:
                break;
        }

        // --- Enemy movement logic placeholder ---
        // In future, add enemy movement here and update enemyPos accordingly
        // For now, enemy stays put unless specified by enemyAction

        // --- Recalculate distance for both entities ---
        int rawDistance = playerPos.calculate_distance(enemyPos);
        CombatDistance newDistance = combat::distance_to_category(rawDistance);
        currentDistance = newDistance;

        // Log distance change if it changed
        if (newDistance != oldDistance) {
            // Distance changed - could log this if needed
        }
    }

    // --- Action handler implementations ---
                static void perform_power_strike(Player& player, Enemy& target, CombatContext& ctx, MessageLog& log, const std::function<int(int, Enemy&)>& applyTelegraphModifier) {
                    int atk = player.get_stats().attack;
                    int def = target.stats().defense;
                    int baseDamage = std::max(0, atk - def);
                    float distanceMod = get_distance_damage_modifier(ctx.currentDistance);
                    int damage = static_cast<int>(baseDamage * 1.5f * distanceMod);
                    int finalDamage = applyTelegraphModifier ? applyTelegraphModifier(damage, target) : damage;
                    target.stats().hp -= finalDamage;
        player.set_cooldown(CombatAction::POWER_STRIKE, 2);  // 1.5x damage = 2 turn cooldown
                    log.add(MessageType::Combat, "POWER STRIKE! " + std::to_string(finalDamage) + " damage!");
        // Show damage number in viewport (enemy takes damage)
        ui::add_damage_number(finalDamage, 3, 5, false, false);
                }

                static void perform_tackle(Player& player, Enemy& target, CombatContext& ctx, MessageLog& log) {
                    int atk = player.get_stats().attack;
                    int def = target.stats().defense;
                    int baseDamage = std::max(0, atk - def);
                    float distanceMod = get_distance_damage_modifier(ctx.currentDistance);
                    int damage = static_cast<int>(baseDamage * 0.8f * distanceMod);
                    target.stats().hp -= damage;
        player.set_cooldown(CombatAction::TACKLE, 1);  // 0.8x damage = 1 turn cooldown
                    target.apply_status({StatusType::Stun, 1, 0});
                    log.add(MessageType::Combat, "TACKLE! " + std::to_string(damage) + " damage (enemy stunned)!");
        // Show damage number in viewport (enemy takes damage)
        ui::add_damage_number(damage, 3, 5, false, false);
                }

                static void perform_whirlwind(Player& player, std::vector<Enemy>& enemies, CombatContext& ctx, MessageLog& log) {
                    if (player.player_class() != PlayerClass::Warrior) {
                        log.add(MessageType::Warning, "Only Warriors can use Whirlwind!");
                        return;
                    }
                    if (ctx.currentDistance != CombatDistance::MELEE) {
                        log.add(MessageType::Warning, "Whirlwind requires melee range!");
                        return;
                    }
                    int atk = player.get_stats().attack;
                    int hits = 0;
                    for (auto& enemy : enemies) {
                        if (enemy.stats().hp > 0 && enemy.height() == HeightLevel::Ground) {
                            int def = enemy.stats().defense;
                            int damage = static_cast<int>(std::max(0, atk - def) * 0.7f);
                            enemy.stats().hp -= damage;
                            hits++;
                            log.add(MessageType::Combat, "Whirlwind hits " + enemy.name() + " for " + std::to_string(damage) + "!");
                        }
                    }
        player.set_cooldown(CombatAction::WHIRLWIND, 1);  // AOE = 1 turn cooldown
                    if (hits == 0) {
                        log.add(MessageType::Warning, "Whirlwind hits nothing!");
                    }
                }

                static void perform_snipe(Player& player, Enemy& target, CombatContext& ctx, MessageLog& log, const std::function<int(int, Enemy&)>& applyTelegraphModifier) {
                    if (player.player_class() != PlayerClass::Rogue) {
                        log.add(MessageType::Warning, "Only Rogues can use Snipe!");
                        return;
                    }
                    if (ctx.currentDistance < CombatDistance::MEDIUM) {
                        log.add(MessageType::Warning, "Snipe requires medium+ range!");
                        return;
                    }
                    int atk = player.get_stats().attack;
                    int def = target.stats().defense;
                    int baseDamage = std::max(0, atk - def);
                    float distanceMod = get_distance_damage_modifier(ctx.currentDistance);
                    int damage = static_cast<int>(baseDamage * 1.5f * distanceMod);
                    int finalDamage = applyTelegraphModifier ? applyTelegraphModifier(damage, target) : damage;
                    target.stats().hp -= finalDamage;
        player.set_cooldown(CombatAction::SNIPE, 2);  // 1.5x damage = 2 turn cooldown
                    log.add(MessageType::Combat, "SNIPE! " + std::to_string(finalDamage) + " precision damage!");
        // Show damage number in viewport (enemy takes damage)
        ui::add_damage_number(finalDamage, 3, 5, false, false);
                }

                static void perform_multishot(Player& player, std::vector<Enemy>& enemies, CombatContext& ctx, MessageLog& log, const std::function<int(int, Enemy&)>& applyTelegraphModifier) {
                    if (player.player_class() != PlayerClass::Rogue) {
                        log.add(MessageType::Warning, "Only Rogues can use Multishot!");
                        return;
                    }
                    if (ctx.currentDistance < CombatDistance::FAR) {
                        log.add(MessageType::Warning, "Multishot requires far range!");
                        return;
                    }
        // FIXED: Explicit bounds check before accessing enemies vector
        if (enemies.empty()) {
            log.add(MessageType::Warning, "No targets available!");
                        return;
                    }
                    int atk = player.get_stats().attack;
                    int hits = 0;
        // FIXED: Use explicit bounds checking - ensure we don't exceed vector size
        const size_t maxTargets = 3;
        const size_t targetCount = std::min(maxTargets, enemies.size());
        for (size_t i = 0; i < targetCount; ++i) {
            // FIXED: Double-check bounds (defensive programming)
            if (i >= enemies.size()) {
                LOG_ERROR("Multishot: Index " + std::to_string(i) + " out of bounds (enemies.size()=" + std::to_string(enemies.size()) + ")");
                break;
            }
                        int def = enemies[i].stats().defense;
                        int damage = static_cast<int>(std::max(0, atk - def) * 0.8f);
                        int finalDamage = applyTelegraphModifier ? applyTelegraphModifier(damage, enemies[i]) : damage;
                        enemies[i].stats().hp -= finalDamage;
                        hits++;
                        log.add(MessageType::Combat, "Multishot hits " + enemies[i].name() + " for " + std::to_string(finalDamage) + "!");
            // Show damage number in viewport (enemy takes damage)
            ui::add_damage_number(finalDamage, 3, 5, false, false);
                    }
        player.set_cooldown(CombatAction::MULTISHOT, 1);  // AOE = 1 turn cooldown
                }

                static void perform_fireball(Player& player, Enemy& target, CombatContext& ctx, MessageLog& log, const std::function<int(int, Enemy&)>& applyTelegraphModifier) {
                    if (player.player_class() != PlayerClass::Mage) {
                        log.add(MessageType::Warning, "Only Mages can cast Fireball!");
                        return;
                    }
        // Mana system removed - no mana check needed
                    int atk = player.get_stats().attack;
                    int def = target.stats().defense;
                    int baseDamage = std::max(0, atk - def);
                    float distanceMod = get_distance_damage_modifier(ctx.currentDistance);
                    int damage = static_cast<int>(baseDamage * 1.2f * distanceMod);
                    int finalDamage = applyTelegraphModifier ? applyTelegraphModifier(damage, target) : damage;
                    target.stats().hp -= finalDamage;
        player.set_cooldown(CombatAction::FIREBALL, 1);  // 1.2x damage = 1 turn cooldown
                    // Check if mage has weapon - if so, this is Magic Sword, not Fireball
                    bool hasWeapon = (player.get_equipment().find(EquipmentSlot::Weapon) != player.get_equipment().end() ||
                                     player.get_equipment().find(EquipmentSlot::Offhand) != player.get_equipment().end());
                    if (hasWeapon) {
                        log.add(MessageType::Combat, glyphs::weapon() + std::string(" MAGIC SWORD! A sword flies through the air and strikes for ") + 
                                std::to_string(finalDamage) + " damage!");
                    } else {
                        target.apply_status({StatusType::Burn, 3, 1});
                        log.add(MessageType::Combat, "FIREBALL! " + std::to_string(finalDamage) + " fire damage (burn applied)!");
                    }
                }

    static void perform_frost_bolt(Player& player, Enemy& target, CombatContext& /*ctx*/, MessageLog& log, const std::function<int(int, Enemy&)>& applyTelegraphModifier) {
                    if (player.player_class() != PlayerClass::Mage) {
                        log.add(MessageType::Warning, "Only Mages can cast Frost Bolt!");
                        return;
                    }
                    int atk = player.get_stats().attack;
                    int def = target.stats().defense;
                    int damage = std::max(0, atk - def);
                    int finalDamage = applyTelegraphModifier ? applyTelegraphModifier(damage, target) : damage;
                    target.stats().hp -= finalDamage;
        // 1.0x damage = 0 cooldown (basic attack)
                    // When FROST_BOLT is used as a weapon attack (not SKILL), it's Sword Casting
                    // Check if mage has weapon - if so, this is Sword Casting, not basic Frost Bolt
                    bool hasWeapon = (player.get_equipment().find(EquipmentSlot::Weapon) != player.get_equipment().end() ||
                                     player.get_equipment().find(EquipmentSlot::Offhand) != player.get_equipment().end());
                    if (hasWeapon) {
                        // Sword Casting - magical sword projectile (weapon attack)
                        log.add(MessageType::Combat, glyphs::weapon() + std::string(" SWORD CASTING! A magical sword projectile strikes for ") + 
                                std::to_string(finalDamage) + " damage!");
                    } else {
                        // Basic Frost Bolt (shouldn't happen via FROST_BOLT action, but keep for safety)
                        target.apply_status({StatusType::Freeze, 1, 0});
                        log.add(MessageType::Combat, "FROST BOLT! " + std::to_string(finalDamage) + " damage! Enemy frozen!");
                    }
                }

    static void perform_teleport(Player& player, CombatContext& /*ctx*/, MessageLog& log) {
                    if (player.player_class() != PlayerClass::Mage) {
                        log.add(MessageType::Warning, "Only Mages can teleport!");
                        return;
                    }
                    // TODO: Implement actual teleportation in Phase 4
        player.set_cooldown(CombatAction::TELEPORT, 1);  // Utility = 1 turn cooldown
                    log.add(MessageType::Combat, "TELEPORT! You vanish and reappear!");
                }

    void melee(Player& player, Enemy& enemy, MessageLog& log, CombatDistance /*distance*/) {
        // Melee attacks can only hit grounded enemies
        if (enemy.height() != HeightLevel::Ground) {
            log.add(MessageType::Combat, "The " + enemy.name() + " is out of reach!");
            return;
        }
        
        int atk = enemy.stats().attack;
        int def = player.get_stats().defense;
        int damageToPlayer = std::max(0, atk - def);
        
        LOG_DEBUG("Enemy ATK " + std::to_string(atk) + 
                  " - player DEF " + std::to_string(player.get_stats().defense) + 
                  " = " + std::to_string(damageToPlayer) + " damage");
        if (damageToPlayer > 0) {
            player.get_stats().hp -= damageToPlayer;
            // Clamp HP to 0 minimum (player dies if HP <= 0, checked elsewhere)
            if (player.get_stats().hp < 0) {
                player.get_stats().hp = 0;
            }
            ui::flash_damage();  // Visual feedback
            ui::play_hit_sound();  // Audio feedback
            log.add(MessageType::Damage, enemy.name() + " hits you for " + std::to_string(damageToPlayer) + ".");
            // Show damage number in viewport (player takes damage)
            auto termSize = input::get_terminal_size();
            int playerSpriteCol = termSize.width / 4;  // Match viewport positioning
            ui::add_damage_number(damageToPlayer, 3, playerSpriteCol, true, false);
        } else {
            log.add(MessageType::Combat, enemy.name() + " attacks but deals no damage.");
        }
        LOG_DEBUG("Player HP after combat: " + std::to_string(player.get_stats().hp));
    }

    void ranged(Player& player, Enemy& enemy, MessageLog& log, CombatDistance distance) {
        // Ranged attacks can hit any height
        int atk = player.get_stats().attack;
        int def = enemy.stats().defense;
        int baseDamage = std::max(0, atk - def);
        
        // Apply distance modifier
        float distanceMod = get_distance_damage_modifier(distance);
        
        // Apply accuracy check for ranged attacks
        int hitChance = get_hit_chance(distance);
        bool hit = (rand() % 100) < hitChance;
        
        if (!hit) {
            log.add(MessageType::Combat, "Your arrow misses the " + enemy.name() + "!");
            return;
        }
        
        int damageToEnemy = static_cast<int>(baseDamage * distanceMod);
        bool isCritical = false;
        if (roll_percentage(15)) {
            damageToEnemy = static_cast<int>(damageToEnemy * 1.5f);
            isCritical = true;
            log.add(MessageType::Combat, glyphs::bow() + std::string(" Critical shot!"));
        }
        enemy.stats().hp -= damageToEnemy;
        
        // Show damage number in viewport (enemy takes damage)
        auto termSize = input::get_terminal_size();
        int enemySpriteCol = termSize.width * 2 / 3;  // Match viewport positioning
        ui::add_damage_number(damageToEnemy, 3, enemySpriteCol, false, isCritical);
        
        std::string heightDesc;
        switch (enemy.height()) {
            case HeightLevel::Flying:
                heightDesc = " out of the sky";
                break;
            case HeightLevel::LowAir:
                heightDesc = " from the air";
                break;
            default:
                heightDesc = "";
                break;
        }
        
        log.add(MessageType::Combat, "Your arrow strikes the " + enemy.name() + heightDesc + " for " + std::to_string(damageToEnemy) + ".");
        if (enemy.stats().hp <= 0) {
            log.add(MessageType::Combat, enemy.name() + " defeated.");
            return;
        }
        // Flying enemies can still retaliate
        int damageToPlayer = std::max(0, enemy.stats().attack - player.get_stats().defense);
        if (damageToPlayer > 0) {
            player.get_stats().hp -= damageToPlayer;
            // Clamp HP to 0 minimum (player dies if HP <= 0, checked elsewhere)
            if (player.get_stats().hp < 0) {
                player.get_stats().hp = 0;
            }
            ui::flash_damage();  // Visual feedback
            ui::play_hit_sound();  // Audio feedback
            log.add(MessageType::Damage, enemy.name() + " retaliates for " + std::to_string(damageToPlayer) + ".");
            // Show damage number in viewport (player takes damage)
            auto termSize = input::get_terminal_size();
            int playerSpriteCol = termSize.width / 4;  // Match viewport positioning
            ui::add_damage_number(damageToPlayer, 3, playerSpriteCol, true, false);
        } else {
            log.add(MessageType::Combat, enemy.name() + " retaliates but deals no damage.");
        }
    }

    // Static variable to store last selected consumable name
    static std::string g_lastSelectedConsumableName;

    // Show combat action menu and return selected action
    CombatAction show_combat_menu(const Player& player, const std::vector<Enemy>& enemies,
                                   int /*screenRow*/, int /*screenCol*/, bool /*hasRangedWeapon*/,
                                   CombatDistance currentDistance, const Position3D& playerPos,
                                   const Position3D& enemyPos, const CombatArena* arena,
                                   const MessageLog& log) {
        LOG_OP_START("show_combat_menu");
        
        // Get terminal size for layout calculation
        auto termSize = input::get_terminal_size();
        
        // NEW LAYOUT: Top viewport (Pokemon-style), bottom menu + arena
        const int topViewportHeight = std::max(15, termSize.height / 2); // Full top half, min 15 rows
        const int topViewportRow = 0;
        const int topViewportCol = 0;
        const int topViewportWidth = termSize.width;
        
        // Bottom section: Menu on left, Arena on right
        const int bottomSectionRow = topViewportHeight;
        const int bottomSectionHeight = termSize.height - topViewportHeight;
        const int menuWidth = (termSize.width - 2) / 2;            // Half width for menu (inner layout)
        const int arenaWidth = termSize.width - menuWidth - 2;     // Remaining for arena (inner layout)
        const int menuCol = 0;
        const int arenaCol = menuWidth + 1;
        
        
        // Draw top Pokemon-style viewport
        if (!enemies.empty()) {
            ui::draw_combat_viewport(topViewportRow, topViewportCol, topViewportWidth, topViewportHeight,
                                     player, enemies[0], currentDistance);
        }
        
        // Draw bottom menu section (slightly extended so right border lines up with outer frame)
        ui::fill_rect(bottomSectionRow, menuCol, menuWidth + 1, bottomSectionHeight);
        ui::draw_box_double(bottomSectionRow, menuCol, menuWidth + 1, bottomSectionHeight, constants::color_frame_main);
        
        // Draw combat arena visualization on the right side of bottom section
        ui::draw_combat_arena(bottomSectionRow, arenaCol, arenaWidth + 1, playerPos, enemyPos, currentDistance, arena);
        
        // Draw HP bars and status info inside the arena box (a few rows below the title)
        const int arenaHeight = 12;
        const int statusInfoRow = bottomSectionRow + 2;
        const int statusInfoHeight = 4; // HP bars (2 rows) + status (2 rows)
        if (statusInfoRow < termSize.height - 5) {  // Only draw if there's space
            // Slightly indent HP + status inside the arena box for nicer alignment
            ui::draw_combat_status_info(statusInfoRow, arenaCol + 2, player, enemies[0]);
        }
        
        // Draw message log below the arena in bottom right
        const int messageLogRow = bottomSectionRow + arenaHeight + 1;
        // Allow a taller combat message log so more lines are visible, but don't exceed screen
        const int messageLogHeight = std::min(10, termSize.height - messageLogRow - 1); // Max 10 lines
        if (messageLogHeight > 0 && messageLogRow < termSize.height - 2) {
            log.render_framed(messageLogRow, arenaCol, arenaWidth, messageLogHeight);
        }

        // Draw combat tips box aligned under the combat message log
        {
            std::string combatTip = "Combat: 1-9 keys choose actions, C: cycle targets, Q: retreat if allowed.";
            // Place slightly further below the combat message log on the right side
            int tipRow = messageLogRow + messageLogHeight + 5;
            // Fallback to bottom of screen if there isn't enough space
            if (tipRow + 2 >= termSize.height) {
                tipRow = std::max(0, termSize.height - 3);
            }

            int tipCol = arenaCol;
            int maxBoxWidth = arenaWidth; // Align width with combat message log frame
            int boxWidth = maxBoxWidth;
            if (boxWidth < 10) boxWidth = std::min(10, maxBoxWidth);

            ui::draw_box_single(tipRow, tipCol, boxWidth, 3, constants::color_frame_main);
            ui::move_cursor(tipRow + 1, tipCol + 2);
            int maxTextWidth = boxWidth - 4;
            if (maxTextWidth > 0) {
                std::cout << combatTip.substr(0, static_cast<size_t>(maxTextWidth));
            }
        }
        
        auto draw_bar = [&](int row, const std::string& label, int current, int maxValue, const std::string& color) {
            ui::move_cursor(row, menuCol + 2);
            std::cout << label << " ";
            const int barWidth = 22;
            int filled = (maxValue > 0) ? (current * barWidth) / maxValue : 0;
            std::cout << glyphs::bar_left();
            for (int i = 0; i < barWidth; ++i) {
                if (i < filled) {
                    ui::set_color(color);
                    std::cout << glyphs::bar_full();
                } else {
                    ui::set_color("\033[90m");
                    std::cout << glyphs::bar_quarter();
                }
            }
            ui::reset_color();
            std::cout << glyphs::bar_right() << " " << current << "/" << maxValue;
        };
        
        auto distance_label = [&](CombatDistance dist) -> std::string {
            switch (dist) {
                case CombatDistance::MELEE: return "MELEE (0-1)";
                case CombatDistance::CLOSE: return "CLOSE (2-3)";
                case CombatDistance::MEDIUM: return "MEDIUM (4-6)";
                case CombatDistance::FAR: return "FAR (7-10)";
                case CombatDistance::EXTREME: return "EXTREME (10+)";
            }
            return "UNKNOWN";
        };
        
        // Menu title and info (in bottom section)
        auto distance_badge_row = bottomSectionRow + 1;
        ui::move_cursor(bottomSectionRow, menuCol + 2);
        ui::set_color(constants::color_frame_main);
        std::cout << " " << glyphs::msg_combat() << " COMBAT ACTIONS ";
        ui::reset_color();
        
        auto available = get_available_actions(player, currentDistance);
        if (available.empty()) {
            available.push_back(CombatAction::WAIT);
        }
        std::unordered_set<CombatAction> availableSet(available.begin(), available.end());
        
        int hotkeyIndex = 0;
        std::vector<std::pair<char, CombatAction>> bindings;
        std::map<char, int> consumableKeyToIndex;  // Map consumable keys to inventory indices
        std::map<char, std::string> consumableKeyToName;  // Map consumable keys to item names
        int row = distance_badge_row + 2;  // Start menu items one row earlier (removed distance display)
        
        auto print_category = [&](const std::string& title, const std::vector<CombatAction>& list) {
            bool headerPrinted = false;
            for (auto action : list) {
                if (!availableSet.count(action)) continue;
                
                // Special handling for CONSUMABLE - show available consumables with effects
                if (action == CombatAction::CONSUMABLE) {
                    // Count consumables by type
                    std::map<std::string, std::pair<int, const Item*>> consumableCounts;
                    for (const auto& item : player.inventory()) {
                        if (item.type == ItemType::Consumable || item.isConsumable) {
                            std::string key = item.name;
                            if (consumableCounts.find(key) == consumableCounts.end()) {
                                consumableCounts[key] = {0, &item};
                            }
                            consumableCounts[key].first++;
                        }
                    }
                    
                    // Only show if there are consumables
                    if (consumableCounts.empty()) continue;
                    
                    if (!headerPrinted) {
                        ui::move_cursor(row++, menuCol + 2);
                        ui::set_color("\033[96m");
                        std::cout << title;
                ui::reset_color();
                        headerPrinted = true;
                    }
                    
                    // Display each consumable type with count and effect
                    for (const auto& [itemName, countAndItem] : consumableCounts) {
                        int count = countAndItem.first;
                        const Item* item = countAndItem.second;
                        
                        if (hotkeyIndex >= static_cast<int>(kActionHotkeys.size())) continue;
                        char key = kActionHotkeys[hotkeyIndex++];
                        bindings.push_back({key, action});
                        
                        // Find first inventory index for this consumable type
                        for (size_t i = 0; i < player.inventory().size(); ++i) {
                            const auto& invItem = player.inventory()[i];
                            if ((invItem.type == ItemType::Consumable || invItem.isConsumable) && 
                                invItem.name == itemName) {
                                consumableKeyToIndex[key] = static_cast<int>(i);
                                consumableKeyToName[key] = itemName;  // Store name mapping
                                break;
                            }
                        }
                        
                        ui::move_cursor(row++, menuCol + 4);
                        std::string glyph = kActionGlyphs.count(action) ? kActionGlyphs.at(action) : "";
                        const auto& ctx = get_action_context(action);
                        
                        // Build effect description
                        std::string effectDesc;
                        if (item->healAmount > 0) {
                            effectDesc = "+" + std::to_string(item->healAmount) + " HP";
                        }
                        if (item->onUseStatus != StatusType::None) {
                            if (!effectDesc.empty()) effectDesc += ", ";
                            std::string statusName;
                            switch (item->onUseStatus) {
                                case StatusType::Haste: statusName = "Haste"; break;
                                case StatusType::Fortify: statusName = "Fortify"; break;
                                case StatusType::Bleed: statusName = "Bleed"; break;
                                case StatusType::Poison: statusName = "Poison"; break;
                                case StatusType::Burn: statusName = "Burn"; break;
                                case StatusType::Freeze: statusName = "Freeze"; break;
                                case StatusType::Stun: statusName = "Stun"; break;
                                default: statusName = "Status"; break;
                            }
                            effectDesc += statusName;
                            if (item->onUseDuration > 0) {
                                effectDesc += " " + std::to_string(item->onUseDuration) + "t";
                            }
                        }
                        if (effectDesc.empty()) {
                            effectDesc = "Use item";
                        }
                        
                        // Show: [key] 🧪 ItemName (x2) - Effect
                        std::cout << "[" << key << "] " << glyph << " " << itemName;
                        if (count > 1) {
                            std::cout << " (x" << count << ")";
                        }
                        std::cout << " - " << effectDesc;
                        // Energy system removed - no energy cost display
                    }
                    continue;
                }
                
                // Normal action display
                if (!headerPrinted) {
                    ui::move_cursor(row++, menuCol + 2);
                    ui::set_color("\033[96m");
                    std::cout << title;
                    ui::reset_color();
                    headerPrinted = true;
                }
                if (hotkeyIndex >= static_cast<int>(kActionHotkeys.size())) continue;
                char key = kActionHotkeys[hotkeyIndex++];
                bindings.push_back({key, action});
                
                const auto& ctx = get_action_context(action);
                ui::move_cursor(row++, menuCol + 4);
                std::string glyph = kActionGlyphs.count(action) ? kActionGlyphs.at(action) : "";
                std::string name = kActionNames.count(action) ? kActionNames.at(action) : "Action";
                std::string description = ctx.description;
                
                // Class-specific names and descriptions for SKILL action
                if (action == CombatAction::SKILL) {
                    switch (player.player_class()) {
                        case PlayerClass::Warrior:
                            name = "Shield Bash";
                            description = "Melee attack + stun";
                            break;
                        case PlayerClass::Rogue:
                            name = "Shadowstep";
                            description = "Teleport + damage buff";
                            break;
                        case PlayerClass::Mage:
                            name = "Frost Bolt";
                            description = "Freezing bolt attack";
                            break;
                    }
                }
                
                // For mages with weapons: rename FIREBALL to Magic Sword, FROST_BOLT to Sword Casting
                if (player.player_class() == PlayerClass::Mage && 
                    player.get_equipment().find(EquipmentSlot::Weapon) != player.get_equipment().end()) {
                    if (action == CombatAction::FIREBALL) {
                        name = "Magic Sword";
                        description = "Long-range flying sword";
                    } else if (action == CombatAction::FROST_BOLT) {
                        name = "Sword Casting";
                        description = "Magical sword projectile";
                    }
                }
                
                std::cout << "[" << key << "] " << glyph << " " << name
                          << " (" << description;
                          if (ctx.cooldown > 0) {
                              std::cout << ", CD:" << ctx.cooldown;
                          }
                          std::cout << ")";
            }
            if (headerPrinted) {
                row++; // spacer
            }
        };
        
        print_category("Attack", kCategoryAbilities);  // Class ability - moved to top
        print_category("Melee", kCategoryMelee);
        // For mages with weapons, show Magic category (Magic Sword and Sword Casting)
        bool isMageWithWeapon = (player.player_class() == PlayerClass::Mage && 
            (player.get_equipment().find(EquipmentSlot::Weapon) != player.get_equipment().end() ||
             player.get_equipment().find(EquipmentSlot::Offhand) != player.get_equipment().end()));
        if (isMageWithWeapon) {
            print_category("Magic", kCategoryMagic);
        }
        print_category("Defense", kCategoryDefense);
        print_category("Utility", kCategoryUtility);
        
        ui::move_cursor(bottomSectionRow + bottomSectionHeight - 3, menuCol + 2);
        std::cout << "Space: Wait   ESC: Cancel";
        ui::move_cursor(bottomSectionRow + bottomSectionHeight - 2, menuCol + 2);
        std::cout << "Choose action: ";
        std::cout.flush();
        
        LOG_DEBUG("Combat menu: Waiting for player input...");
        int inputAttempts = 0;
        while (true) {
            inputAttempts++;
            if (inputAttempts > 1) {
                LOG_DEBUG("Combat menu: Input attempt #" + std::to_string(inputAttempts));
            }
            
            LOG_OP_START("read_key_blocking_combat");
            int key = input::read_key_blocking();
            LOG_OP_END("read_key_blocking_combat");
            
            // Safety check: if read_key_blocking returns -1 (timeout/error), log and continue
            if (key == -1) {
                LOG_WARN("Combat menu: read_key_blocking returned -1, retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            LOG_DEBUG("Combat menu: Received key: " + std::to_string(key));
            
            if (key == ' ' || key == 27) {
                LOG_OP_END("show_combat_menu");
                return CombatAction::WAIT;
            }
            
            // Ignore movement keys (WASD and arrow keys) during combat
            // Class ability is now in the numbered menu, not triggered by 'S'
            if (key == 'w' || key == 'W' || key == 'a' || key == 'A' || 
                key == 's' || key == 'S' || key == 'd' || key == 'D' ||
                key == input::KEY_UP || key == input::KEY_DOWN || 
                key == input::KEY_LEFT || key == input::KEY_RIGHT) {
                // Ignore movement keys - they do nothing in combat
                LOG_DEBUG("Combat menu: Movement key ignored");
                continue;
            }
            for (const auto& binding : bindings) {
                if (key == binding.first) {
                    // Store consumable selection for execute_action
                    if (binding.second == CombatAction::CONSUMABLE && consumableKeyToName.count(key)) {
                        g_lastSelectedConsumableName = consumableKeyToName[key];
                    }
                    LOG_OP_END("show_combat_menu");
                    return binding.second;
                }
            }
            
            // Invalid key - log it
            LOG_DEBUG("Combat menu: Invalid key pressed, waiting again...");
        }
    }

    // Execute player's chosen combat action
    void execute_action(Player& player, std::vector<Enemy>& enemies, 
                        CombatContext& ctx, MessageLog& log, const Dungeon& dungeon,
                        const CombatArena* arena) {
        if (enemies.empty()) {
            ctx.wasSuccessful = false;
            return;
        }
        
        // Bounds check for targetIndex
        if (ctx.targetIndex < 0 || static_cast<size_t>(ctx.targetIndex) >= enemies.size()) {
            LOG_ERROR("Invalid targetIndex: " + std::to_string(ctx.targetIndex) + " (enemies.size()=" + std::to_string(enemies.size()) + ")");
            ctx.wasSuccessful = false;
            return;
        }
        
        Enemy& target = enemies[ctx.targetIndex];
        ctx.wasSuccessful = true;
        const auto& actionInfo = get_action_context(ctx.action);
        
        // Check cooldown before executing
        if (player.is_on_cooldown(ctx.action)) {
            int cooldown = player.get_cooldown(ctx.action);
            log.add(MessageType::Warning, "Ability on cooldown (" + std::to_string(cooldown) + " turns remaining)!");
            ctx.wasSuccessful = false;
            return;
        }
        
        if (actionInfo.isTelegraphed) {
            log.add(MessageType::Warning, glyphs::warning() + std::string(" You telegraph your move!"));
        }
        
        // Set cooldown after successful action (if action has cooldown)
        if (actionInfo.cooldown > 0) {
            player.set_cooldown(ctx.action, actionInfo.cooldown);
        }
        auto applyTelegraphModifier = [&](int dmg, Enemy& affected) {
            if (!actionInfo.isTelegraphed) return dmg;
            if (roll_percentage(30)) {
                log.add(MessageType::Warning, affected.name() + " braces for impact!");
                return static_cast<int>(dmg * 0.7f);
            }
            return dmg;
        };
        
        switch (ctx.action) {
            // Legacy actions
            case CombatAction::ATTACK:
            case CombatAction::SLASH: {
                // Player melee attack on enemy
                if (target.height() != HeightLevel::Ground) {
                    log.add(MessageType::Combat, "The " + target.name() + " is out of reach!");
                break;
                }
                int atk = player.get_stats().attack;
                int def = target.stats().defense;
                int baseDamage = std::max(0, atk - def);
                float distanceMod = get_distance_damage_modifier(ctx.currentDistance);
                int damage = static_cast<int>(baseDamage * actionInfo.baseDamage * distanceMod);
                int finalDamage = applyTelegraphModifier(damage, target);
                target.stats().hp -= finalDamage;
                // 1.0x damage = 0 cooldown (basic attack)
                log.add(MessageType::Combat, "SLASH! " + std::to_string(finalDamage) + " damage!");
                // Show damage number in viewport (enemy takes damage)
                auto termSize = input::get_terminal_size();
                int enemySpriteCol = termSize.width * 2 / 3;  // Match viewport positioning
                ui::add_damage_number(finalDamage, 3, enemySpriteCol, false, false);
                break;
            }
                
            case CombatAction::RANGED:
            case CombatAction::SHOOT:
                ranged(player, target, log, ctx.currentDistance);
                break;
                
            case CombatAction::DEFEND:
            case CombatAction::BRACE:
                perform_defensive_stance(player, log);
                break;
                
            case CombatAction::SKILL:
                perform_class_ability(player, target, log);
                break;
                
            case CombatAction::CONSUMABLE:
                if (ctx.consumableUsedIndex >= 0 && ctx.consumableUsedIndex < static_cast<int>(player.inventory().size())) {
                    Item& item = player.inventory()[ctx.consumableUsedIndex];
                    use_consumable_in_combat(player, item, log);
                    // Remove consumable from inventory after use
                    player.inventory().erase(player.inventory().begin() + ctx.consumableUsedIndex);
                } else {
                    log.add(MessageType::Warning, "No consumable available!");
                    ctx.wasSuccessful = false;
                }
                break;
                
            // Movement actions removed - combat simplified
            // These actions are no longer available in the combat menu
            case CombatAction::RETREAT:
            case CombatAction::ADVANCE:
            case CombatAction::CIRCLE:
            case CombatAction::REPOSITION: {
                // Movement actions disabled - should not be reachable
                log.add(MessageType::Warning, "Movement actions are not available!");
                    ctx.wasSuccessful = false;
                break;
            }
                
            case CombatAction::WAIT:
                log.add(MessageType::Info, "You take a defensive stance and wait.");
                break;
                
            // Simplified melee actions - SLASH is already handled above in ATTACK case
            case CombatAction::POWER_STRIKE:
                // Special attack - cooldown set in action database (2 turns)
                perform_power_strike(player, target, ctx, log, applyTelegraphModifier);
                // Cooldown is automatically set by execute_action if actionInfo.cooldown > 0
                break;
            // Removed actions: TACKLE, WHIRLWIND (kept for backward compatibility but not in menu)
            case CombatAction::TACKLE:
                perform_tackle(player, target, ctx, log);
                break;
            case CombatAction::WHIRLWIND:
                perform_whirlwind(player, enemies, ctx, log);
                break;
                
            // New ranged actions
            case CombatAction::SNIPE:
                perform_snipe(player, target, ctx, log, applyTelegraphModifier);
                break;
            case CombatAction::MULTISHOT:
                perform_multishot(player, enemies, ctx, log, applyTelegraphModifier);
                break;
            case CombatAction::FIREBALL:
                perform_fireball(player, target, ctx, log, applyTelegraphModifier);
                break;
            case CombatAction::FROST_BOLT:
                perform_frost_bolt(player, target, ctx, log, applyTelegraphModifier);
                break;
            case CombatAction::TELEPORT:
                perform_teleport(player, ctx, log);
                break;
        }
    }

    // Perform defensive stance
    void perform_defensive_stance(Player& player, MessageLog& log) {
        // Apply temporary fortify status for 1 turn
        StatusEffect fortify;
        fortify.type = StatusType::Fortify;
        fortify.remainingTurns = 1;
        fortify.magnitude = 50;  // 50% damage reduction
        player.apply_status(fortify);
        
        log.add(MessageType::Combat, glyphs::shield() + std::string(" You raise your guard! Damage reduced by 50%."));
        ui::play_hit_sound();
    }

    // Perform class-specific ability (called via SKILL action)
    void perform_class_ability(Player& player, Enemy& enemy, MessageLog& log) {
        PlayerClass playerClass = player.player_class();
        auto& playerStats = player.get_stats();
        
        switch (playerClass) {
            case PlayerClass::Warrior: {
                // SHIELD_BASH: Knockback + stun (60 energy)
                int damage = std::max(0, playerStats.attack - enemy.stats().defense);
                enemy.stats().hp -= damage;
                // Basic attack = 0 cooldown
                // TODO: Apply knockback and stun in Phase 8
                log.add(MessageType::Combat, glyphs::shield() + std::string(" SHIELD BASH! ") + 
                        std::to_string(damage) + " damage! Enemy stunned!");
                break;
            }
            case PlayerClass::Rogue: {
                // SHADOWSTEP: Teleport + next attack 1.2x (60 energy)
                // Basic attack = 0 cooldown (Shadowstep removed - Rogue class hidden)
                // TODO: Implement teleport and damage buff in Phase 4
                log.add(MessageType::Combat, glyphs::dagger() + std::string(" SHADOWSTEP! You teleport behind the enemy! Next attack +20% damage!"));
                break;
            }
            case PlayerClass::Mage: {
                // FROST BOLT: Basic freezing bolt attack (no cooldown)
                int atk = player.get_stats().attack;
                int def = enemy.stats().defense;
                int damage = std::max(0, atk - def);
                enemy.stats().hp -= damage;
                enemy.apply_status({StatusType::Freeze, 1, 0});
                // Show damage number in viewport (enemy takes damage)
                auto termSize = input::get_terminal_size();
                int enemySpriteCol = termSize.width * 2 / 3;
                ui::add_damage_number(damage, 3, enemySpriteCol, false, false);
                log.add(MessageType::Combat, "FROST BOLT! " + std::to_string(damage) + " damage! Enemy frozen!");
                // Basic attack = 0 cooldown
                break;
            }
        }
    }

    // Use consumable item during combat
    void use_consumable_in_combat(Player& player, Item& item, MessageLog& log) {
        if (item.type == ItemType::Consumable) {
            if (item.healAmount > 0) {
                int healAmount = item.healAmount;
                int oldHp = player.get_stats().hp;
                // Use the player's heal method to ensure HP is properly updated
                player.heal(healAmount);
                int actualHeal = player.get_stats().hp - oldHp;
                log.add(MessageType::Heal, glyphs::potion() + std::string(" You drink the potion and recover ") + 
                        std::to_string(actualHeal) + " HP!");
                ui::flash_heal();
            }
            // Remove item from inventory (handled by caller)
        }
    }

    // Attempt to retreat from combat
    bool perform_retreat(Player& player, const Enemy& enemy, MessageLog& log) {
        // Retreat chance based on speed difference
        int speedDiff = player.get_stats().speed - enemy.stats().speed;
        int retreatChance = 50 + speedDiff * 5;  // Base 50% + 5% per speed point
        retreatChance = std::max(20, std::min(90, retreatChance));  // Clamp 20-90%
        
        std::random_device rd;
        std::mt19937 rng(rd());
        int roll = std::uniform_int_distribution<int>(0, 100)(rng);
        
        if (roll < retreatChance) {
            log.add(MessageType::Info, glyphs::arrow_left() + std::string(" You successfully retreat!"));
            return true;
        } else {
            log.add(MessageType::Warning, "You failed to retreat! The " + enemy.name() + " blocks your escape!");
            return false;
        }
    }

    // Enter tactical combat mode - full combat loop with menu
    bool enter_combat_mode(Player& player, Enemy& enemy, Dungeon& dungeon, MessageLog& log) {
        LOG_DEBUG("Entering tactical combat mode with " + enemy.name());
        
        // Initialize 3D positions
        Position3D playerPos;
        playerPos.x = player.get_position().x;
        playerPos.y = player.get_position().y;
        playerPos.depth = 0;  // Start at closest
        
        Position3D enemyPos;
        enemyPos.x = enemy.get_position().x;
        enemyPos.y = enemy.get_position().y;
        enemyPos.depth = 0;  // Start at closest
        
        // Calculate initial combat distance
        CombatDistance currentDistance = calculate_combat_distance(playerPos, enemyPos);
        
        // Create enemy vector for combat menu (update it each turn to reflect current state)
        std::vector<Enemy> enemies;
        
        // Check if player has ranged weapon
        bool hasRangedWeapon = (get_player_attack_type(player) == AttackType::Ranged);
        
        // Generate combat arena (with potential hazards)
        CombatArena arena = CombatArena::generate_random(0, dungeon, combat_rng());  // 0 hazards for now
        
        // Combat loop
        bool combatActive = true;
        bool playerWon = false;
        bool playerRetreated = false;
        (void)playerRetreated;  // Reserved for future use
        
        while (combatActive) {
            // Check if player is dead
            if (player.get_stats().hp <= 0) {
                LOG_DEBUG("Player died in combat");
                combatActive = false;
                playerWon = false;
                break;
            }
            
            // Check if enemy is dead
            if (enemy.stats().hp <= 0) {
                LOG_DEBUG("Enemy " + enemy.name() + " died in combat");
                combatActive = false;
                playerWon = true;
                break;
            }
            
            // Update enemy vector with current enemy state
            enemies.clear();
            enemies.push_back(enemy);
            
            // Remind player to heal if HP is low
            int currentHp = player.get_stats().hp;
            int maxHp = player.get_stats().maxHp;
            float hpPercent = (maxHp > 0) ? (static_cast<float>(currentHp) / static_cast<float>(maxHp)) : 0.0f;
            
            // Check if player has healing potions
            bool hasHealingPotion = false;
            for (const auto& item : player.inventory()) {
                if (item.isConsumable && item.healAmount > 0) {
                    hasHealingPotion = true;
                    break;
                }
            }
            
            // Remind to heal if HP is below 50% and has potions
            if (hpPercent < 0.5f && hasHealingPotion) {
                static int turnCounter = 0;
                turnCounter++;
                // Only remind once per 3 turns to avoid spam
                if (turnCounter % 3 == 0) {
                    log.add(MessageType::Warning, glyphs::warning() + std::string(" Your health is low! Use a healing potion from your inventory (press consumable key in combat menu)."));
                }
            }
            
            // Clear screen and show combat UI
            ui::clear();
            
            // Get terminal size for positioning
            auto termSize = input::get_terminal_size();
            int menuRow = std::max(2, (termSize.height - 24) / 2);
            int menuCol = std::max(2, (termSize.width - 120) / 2);
            
            // Show combat menu with arena visualization
            LOG_OP_START("combat_menu_call");
            CombatAction action = show_combat_menu(player, enemies, menuRow, menuCol, hasRangedWeapon, 
                                                   currentDistance, playerPos, enemyPos, &arena, log);
            LOG_OP_END("combat_menu_call");
            
            // Handle WAIT action (cancel combat)
            if (action == CombatAction::WAIT) {
                // Player wants to cancel - treat as failed retreat
                log.add(MessageType::Warning, "You cannot escape! The " + enemy.name() + " blocks your path!");
                // Continue combat
            }
            
            // Execute player action
            CombatContext ctx;
            ctx.action = action;
            ctx.targetIndex = 0;
            ctx.playerPos = playerPos;
            ctx.enemyPos = enemyPos;
            ctx.currentDistance = currentDistance;
            
            // If consumable action, find the selected consumable by name
            if (action == CombatAction::CONSUMABLE) {
                // Find first consumable with matching name
                bool found = false;
                for (size_t i = 0; i < player.inventory().size(); ++i) {
                    const auto& item = player.inventory()[i];
                    if ((item.type == ItemType::Consumable || item.isConsumable) && 
                        item.name == g_lastSelectedConsumableName) {
                        ctx.consumableUsedIndex = static_cast<int>(i);
                        found = true;
                        break;
                    }
                }
                // If not found, try to find any consumable as fallback
                if (!found && !g_lastSelectedConsumableName.empty()) {
                    LOG_WARN("Consumable '" + g_lastSelectedConsumableName + "' not found, trying fallback");
                    for (size_t i = 0; i < player.inventory().size(); ++i) {
                        const auto& item = player.inventory()[i];
                        if (item.type == ItemType::Consumable || item.isConsumable) {
                            ctx.consumableUsedIndex = static_cast<int>(i);
                            found = true;
                            break;
                        }
                    }
                }
                // Clear the stored name after use to prevent stale data
                if (found) {
                    g_lastSelectedConsumableName.clear();
                }
            }
            
            // Animate player action if it's an attack
            // For mage SKILL, treat it as FROST_BOLT for animation
            bool isMageSkill = (action == CombatAction::SKILL && player.player_class() == PlayerClass::Mage);
            if (action == CombatAction::SLASH || action == CombatAction::POWER_STRIKE || 
                action == CombatAction::TACKLE || action == CombatAction::SHOOT || 
                action == CombatAction::SNIPE || action == CombatAction::FIREBALL || 
                action == CombatAction::FROST_BOLT || action == CombatAction::MULTISHOT ||
                isMageSkill) {
                // Get terminal size and calculate viewport height
                auto termSize = input::get_terminal_size();
                const int topViewportHeight = std::max(15, termSize.height / 2);
                
                // Redraw viewport for animation
                ui::clear();
                ui::draw_combat_viewport(0, 0, termSize.width, topViewportHeight, player, enemy, currentDistance);
                
                // Get player sprite and calculate dimensions
                std::string playerSprite = ui::get_player_sprite(player.player_class());
                auto playerDims = ui::calculate_sprite_dimensions(playerSprite);
                int playerSpriteCol = termSize.width / 4;  // ~25% from left
                int playerSpriteRow = topViewportHeight - playerDims.first - 3; // From bottom
                
                // Get enemy sprite and calculate dimensions
                std::string enemySprite = ui::get_enemy_sprite(enemy);
                auto enemyDims = ui::calculate_sprite_dimensions(enemySprite);
                int enemySpriteCol = termSize.width * 2 / 3;  // ~67% from left
                int enemySpriteRow = 2; // From top
                
                // Calculate center positions for projectiles
                int playerCenterRow = playerSpriteRow + playerDims.first / 2;
                int playerCenterCol = playerSpriteCol + playerDims.second / 2;
                int enemyCenterRow = enemySpriteRow + enemyDims.first / 2;
                int enemyCenterCol = enemySpriteCol + enemyDims.second / 2;
                
                std::string playerColor = constants::color_player;
                std::string enemyColor = enemy.color().empty() ? "\033[91m" : enemy.color();
                
                // Animate based on player class and attack type
                PlayerClass pclass = player.player_class();
                AttackType attackType = get_player_attack_type(player);
                
                // Check if this is a mage skill (frost bolt) or mage spell
                bool isMageSkill = (action == CombatAction::SKILL && pclass == PlayerClass::Mage);
                bool isMageSpell = (pclass == PlayerClass::Mage || action == CombatAction::FIREBALL || action == CombatAction::FROST_BOLT || isMageSkill);
                
                if (isMageSpell) {
                    // Mage: Projectile animation
                    // For mage SKILL, use frost bolt animation
                    bool isFireball = (action == CombatAction::FIREBALL);
                    std::string projectile = isFireball ? "🔥" : "❄";
                    std::string projColor = isFireball ? "\033[91m" : "\033[96m";
                    ui::animate_projectile(playerCenterRow, playerCenterCol, enemyCenterRow, enemyCenterCol, 
                                          projectile, projColor);
                    ui::animate_explosion(enemyCenterRow, enemyCenterCol, projColor);
                } else if (pclass == PlayerClass::Rogue || attackType == AttackType::Ranged) {
                    // Rogue: Slide animation
                    ui::animate_rogue_slide(playerSpriteRow, playerSpriteCol, enemySpriteCol - 5, 
                                          playerSprite, playerColor);
                } else if (pclass == PlayerClass::Warrior) {
                    // Warrior: Charge animation
                    ui::animate_warrior_charge(playerSpriteRow, playerSpriteCol, enemySpriteCol - 5, 
                                             playerSprite, playerColor);
                } else {
                    // Default: Simple slide
                    ui::animate_sprite_attack(playerSpriteRow, playerSpriteCol, playerSprite, playerColor, true);
                }
                
                // Animate enemy shake after attack
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                ui::animate_sprite_shake(enemySpriteRow, enemySpriteCol, enemySprite, enemyColor, 2, 300);
                
                // Don't clear screen here - wait until after damage is applied
            }
            
            LOG_OP_START("execute_action");
            combat::execute_action(player, enemies, ctx, log, dungeon, &arena);
            LOG_OP_END("execute_action");
            
            // Update enemy reference from vector (in case execute_action modified it)
            if (!enemies.empty()) {
                enemy = enemies[0];
            }
            
            // Update positions from context
            playerPos = ctx.playerPos;
            enemyPos = ctx.enemyPos;
            currentDistance = ctx.currentDistance;
            
            // Movement actions removed - retreat check no longer needed
            // if (action == CombatAction::RETREAT && ctx.wasSuccessful) {
            //     combatActive = false;
            //     playerRetreated = true;
            //     playerWon = true;  // Retreat is a "win" for the player
            //     break;
            // }
            
            // Don't redraw here - let the next loop iteration handle it via show_combat_menu
            // This prevents any potential blocking or visual glitches
            
            // Check if enemy died from player action
            if (enemy.stats().hp <= 0) {
                // Enemy died - play death animation before exiting
                LOG_DEBUG("Enemy " + enemy.name() + " died - playing death animation");
                
                // Redraw viewport to show current state (enemy at 0 HP) before death animation
                auto termSizeForDeath = input::get_terminal_size();
                const int topViewportHeightForDeath = std::max(15, termSizeForDeath.height / 2);
                ui::clear();
                ui::draw_combat_viewport(0, 0, termSizeForDeath.width, topViewportHeightForDeath, player, enemy, currentDistance);
                
                // Get enemy sprite position for explosion (must match viewport positioning)
                std::string enemySpriteForDeath = ui::get_enemy_sprite(enemy);
                auto enemyDimsForDeath = ui::calculate_sprite_dimensions(enemySpriteForDeath);
                
                // Match the positioning used in draw_combat_viewport exactly
                // In draw_combat_viewport:
                // - startRow = 0, startCol = 0 (viewport starts at top-left)
                // - enemySpriteRow = startRow + 2 = 2
                // - enemySpriteCol = startCol + (width * 2 / 3) = width * 2 / 3 (NOT centered)
                int enemySpriteRowForDeath = 2;  // startRow + 2, where startRow=0
                int enemySpriteColForDeath = termSizeForDeath.width * 2 / 3;  // Match draw_combat_viewport
                
                // Calculate center of enemy sprite for explosion
                // enemyDims.first = height (rows), enemyDims.second = width (columns)
                int explosionRow = enemySpriteRowForDeath + enemyDimsForDeath.first / 2;
                int explosionCol = enemySpriteColForDeath + enemyDimsForDeath.second / 2;
                
                // Brief pause before explosion
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                
                // Play explosion animation at enemy center
                ui::animate_explosion(explosionRow, explosionCol, "\033[91m");  // Red explosion
                
                // Brief pause to show death
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                
                // Add victory message
                log.add(MessageType::Combat, enemy.name() + " defeated!");
                
                combatActive = false;
                playerWon = true;
                break;
            }
            
            // Check if player died
            if (player.get_stats().hp <= 0) {
                combatActive = false;
                playerWon = false;
                break;
            }
            
            // Enemy turn (if still alive)
            if (combatActive && enemy.stats().hp > 0) {
                // Check if enemy is adjacent (for melee attack)
                Position ep = enemy.get_position();
                Position pp = player.get_position();
                int manhattanDist = std::abs(ep.x - pp.x) + std::abs(ep.y - pp.y);
                
                if (manhattanDist == 1) {
                    // Enemy is adjacent - can attack
                    LOG_DEBUG("Enemy " + enemy.name() + " attacking player");
                    
                    // Check if this is a "heavy" attack that should be telegraphed
                    bool isHeavyAttack = (enemy.stats().attack >= 8 || 
                                         enemy.enemy_type() == EnemyType::Ogre ||
                                         enemy.enemy_type() == EnemyType::Troll ||
                                         enemy.enemy_type() == EnemyType::Dragon ||
                                         enemy.enemy_type() == EnemyType::StoneGolem ||
                                         enemy.enemy_type() == EnemyType::ShadowLord);
                    
                    if (isHeavyAttack) {
                        // Show telegraph warning
                        log.add(MessageType::Warning, glyphs::warning() + std::string(" ") + enemy.name() + " is preparing a heavy attack...");
                        ui::flash_warning();
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Pause for visibility
                    }
                    
                    // Animate enemy attack
                    auto termSizeForEnemyAnim = input::get_terminal_size();
                    const int topViewportHeightForEnemyAnim = std::max(15, termSizeForEnemyAnim.height / 2);
                    ui::clear();
                    ui::draw_combat_viewport(0, 0, termSizeForEnemyAnim.width, topViewportHeightForEnemyAnim, player, enemy, currentDistance);
                    
                    // Get enemy sprite and calculate dimensions
                    std::string enemySprite = ui::get_enemy_sprite(enemy);
                    auto enemyDims = ui::calculate_sprite_dimensions(enemySprite);
                    std::string enemyColor = enemy.color().empty() ? "\033[91m" : enemy.color();
                    int enemySpriteCol = termSizeForEnemyAnim.width * 2 / 3;  // Match viewport positioning
                    int enemySpriteRow = 2; // From top
                    
                    // Get player sprite and calculate dimensions for shake animation
                    std::string playerSprite = ui::get_player_sprite(player.player_class());
                    auto playerDims = ui::calculate_sprite_dimensions(playerSprite);
                    int playerSpriteCol = termSizeForEnemyAnim.width / 4;  // Match viewport positioning
                    int playerSpriteRow = topViewportHeightForEnemyAnim - playerDims.first - 3; // From bottom
                    std::string playerColor = constants::color_player;
                    
                    // Check if enemy is ranged or melee
                    // For enemies, check if they have ranged attacks (archers, etc.)
                    bool isRangedEnemy = (enemy.enemy_type() == EnemyType::Archer || 
                                         enemy.enemy_type() == EnemyType::Dragon);
                    
                    if (isRangedEnemy) {
                        // Ranged enemy: projectile animation
                        int enemyCenterRow = enemySpriteRow + enemyDims.first / 2;
                        int enemyCenterCol = enemySpriteCol + enemyDims.second / 2;
                        int playerCenterRow = playerSpriteRow + playerDims.first / 2;
                        int playerCenterCol = playerSpriteCol + playerDims.second / 2;
                        ui::animate_projectile(enemyCenterRow, enemyCenterCol, playerCenterRow, playerCenterCol,
                                              "→", enemyColor);
                        ui::animate_explosion(playerCenterRow, playerCenterCol, enemyColor);
                    } else {
                        // Melee enemy: slide animation
                        ui::animate_sprite_attack(enemySpriteRow, enemySpriteCol, enemySprite, enemyColor, false);
                    }
                    
                    // Animate player shake
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    ui::animate_sprite_shake(playerSpriteRow, playerSpriteCol, playerSprite, playerColor, 2, 300);
                    
                    // Redraw viewport after animation
                    ui::clear();
                    
                    // Enemy attacks player
                    combat::melee(player, enemy, log, currentDistance);
                } else {
                    // Enemy is not adjacent - move towards player
                    ai::take_turn(enemy, player, dungeon, log);
                    
                    // Update enemy position in 3D space (enemy may have moved)
                    enemyPos.x = enemy.get_position().x;
                    enemyPos.y = enemy.get_position().y;
                    
                    // Recalculate distance after enemy movement
                    currentDistance = calculate_combat_distance(playerPos, enemyPos);
                }
            }
            
            // Restore energy and tick cooldowns at end of turn
            player.tick_cooldowns();
            player.tick_statuses();
            
            // Small delay for readability
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Return true if player won or retreated, false if player died
        return playerWon;
    }

    // Apply weapon affixes during combat
    void apply_weapon_affixes(const Item& weapon, Enemy& target, Player& attacker, MessageLog& log) {
        if (weapon.affix == ItemAffix::NONE) return;
        
        std::random_device rd;
        std::mt19937 rng(rd());
        
        switch (weapon.affix) {
            case ItemAffix::LIFESTEAL: {
                int healAmount = static_cast<int>(5 * weapon.affixStrength);
                attacker.get_stats().hp = std::min(
                    attacker.get_stats().hp + healAmount,
                    attacker.get_stats().maxHp
                );
                log.add(MessageType::Heal, glyphs::corpse() + std::string(" Life stolen! +") + 
                        std::to_string(healAmount) + " HP");
                ui::flash_heal();
                break;
            }
            
            case ItemAffix::BURNING: {
                log.add(MessageType::Combat, glyphs::fire() + std::string(" Enemy is set ablaze!"));
                // Would apply burn status to enemy
                break;
            }
            
            case ItemAffix::FROST: {
                target.stats().speed = std::max(1, target.stats().speed - 3);
                log.add(MessageType::Combat, glyphs::ice() + std::string(" Enemy is frozen! Speed reduced."));
                break;
            }
            
            case ItemAffix::POISON_COAT: {
                log.add(MessageType::Combat, glyphs::status_poison() + std::string(" Enemy is poisoned!"));
                // Would apply poison status to enemy
                break;
            }
            
            case ItemAffix::SLOW_TARGET: {
                target.stats().speed = std::max(1, target.stats().speed - 2);
                log.add(MessageType::Combat, "Enemy slowed!");
                break;
            }
            
            case ItemAffix::VORPAL: {
                // 10% instant kill (non-boss)
                if (std::uniform_int_distribution<int>(0, 100)(rng) < 10) {
                    // Check if not a boss (Dragon, Lich)
                    EnemyType type = target.enemy_type();
                    if (type != EnemyType::Dragon && type != EnemyType::Lich) {
                        target.stats().hp = 0;
                        log.add(MessageType::Combat, glyphs::weapon() + std::string(" VORPAL! Head severed!"));
                        ui::flash_critical();
                        ui::play_critical_sound();
                    }
                }
                break;
            }
            
            case ItemAffix::VAMPIRIC: {
                int healAmount = static_cast<int>(10 * weapon.affixStrength);
                attacker.get_stats().hp = std::min(
                    attacker.get_stats().hp + healAmount,
                    attacker.get_stats().maxHp
                );
                log.add(MessageType::Heal, glyphs::corpse() + std::string(" VAMPIRIC! Massive life drain! +") + 
                        std::to_string(healAmount) + " HP");
                ui::flash_heal();
                break;
            }
            
            default:
                break;
        }
    }

    // Apply armor affixes when taking damage
    int apply_armor_affixes(const Item& armor, int incomingDamage, Player& wearer, MessageLog& log) {
        int finalDamage = incomingDamage;
        
        switch (armor.affix) {
            case ItemAffix::FIRE_RESIST:
                // Assume fire damage for now
                finalDamage = static_cast<int>(finalDamage * 0.5f);
                log.add(MessageType::Combat, glyphs::fire() + std::string(" Armor resists fire!"));
                break;
                
            case ItemAffix::COLD_RESIST:
                finalDamage = static_cast<int>(finalDamage * 0.5f);
                log.add(MessageType::Combat, glyphs::ice() + std::string(" Armor resists cold!"));
                break;
                
            case ItemAffix::THORNS: {
                int returnDamage = static_cast<int>(incomingDamage * 0.25f);
                log.add(MessageType::Combat, glyphs::trap() + std::string(" Thorns reflect ") + 
                        std::to_string(returnDamage) + " damage!");
                // Would need to apply to attacker
                break;
            }
            
            case ItemAffix::REFLECTIVE: {
                finalDamage = static_cast<int>(finalDamage * 0.5f);
                log.add(MessageType::Combat, glyphs::shield() + std::string(" Armor reflects half damage!"));
                break;
            }
            
            case ItemAffix::EVASION: {
                // 20% chance to completely dodge
                std::random_device rd;
                std::mt19937 rng(rd());
                if (std::uniform_int_distribution<int>(0, 100)(rng) < 20) {
                    finalDamage = 0;
                    log.add(MessageType::Combat, glyphs::arrow_right() + std::string(" Dodged!"));
                }
                break;
            }
            
            case ItemAffix::HEALTH_REGEN:
                // This is applied per turn, not on damage
                break;
                
            default:
                break;
        }
        
        // Suppress unused parameter warning
        (void)wearer;
        
        return finalDamage;
    }
}

// Combat arena hazard system implementation (outside combat namespace)
bool CombatArena::apply_hazard(const Position3D& pos, Player& player, MessageLog& log) const {
    // Safety check: ensure vectors are in sync
    if (hazards.size() != hazardPositions.size()) {
        LOG_ERROR("CombatArena::apply_hazard - hazards and hazardPositions size mismatch!");
        return false;
    }
    
    for (size_t i = 0; i < hazards.size(); ++i) {
        if (hazardPositions[i].x == pos.x && hazardPositions[i].y == pos.y && 
            hazardPositions[i].depth == pos.depth) {
            switch (hazards[i]) {
                case CombatHazard::SPIKE_FLOOR:
                    player.take_damage(5);
                    log.add(MessageType::Damage, glyphs::trap() + std::string(" Spikes damage you for 5 HP!"));
                    ui::flash_damage();
                    return true;
                case CombatHazard::FIRE_PILLAR:
                    player.apply_status({StatusType::Burn, 3, 1});
                    log.add(MessageType::Damage, glyphs::fire() + std::string(" Fire burns you!"));
                    ui::flash_damage();
                    return true;
                case CombatHazard::ICE_PATCH:
                    player.apply_status({StatusType::Freeze, 2, 1});
                    log.add(MessageType::Warning, glyphs::ice() + std::string(" Ice freezes you!"));
                    return true;
                case CombatHazard::POISON_CLOUD:
                    player.apply_status({StatusType::Poison, 5, 2});
                    log.add(MessageType::Damage, std::string(glyphs::status_poison()) + " Poison cloud engulfs you!");
                    return true;
                case CombatHazard::HEALING_SPRING:
                    player.heal(5);
                    log.add(MessageType::Heal, glyphs::potion() + std::string(" Healing spring restores 5 HP!"));
                    ui::flash_heal();
                    return true;
                default:
                    break;
            }
        }
    }
    return false;
}

CombatArena CombatArena::generate_random(int hazardCount, const Dungeon& dungeon, std::mt19937& rng) {
    CombatArena arena;
    std::uniform_int_distribution<int> hazardDist(0, 4);  // 5 hazard types (excluding NONE)
    std::uniform_int_distribution<int> xDist(1, dungeon.width() - 2);
    std::uniform_int_distribution<int> yDist(1, dungeon.height() - 2);
    std::uniform_int_distribution<int> depthDist(0, 10);
    
    for (int i = 0; i < hazardCount; ++i) {
        CombatHazard hazard = static_cast<CombatHazard>(hazardDist(rng));
        Position3D pos;
        pos.x = xDist(rng);
        pos.y = yDist(rng);
        pos.depth = depthDist(rng);
        
        // Only place on walkable tiles
        if (dungeon.is_walkable(pos.x, pos.y)) {
            arena.hazards.push_back(hazard);
            arena.hazardPositions.push_back(pos);
        }
    }
    
    return arena;
}




