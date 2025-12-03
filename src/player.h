#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "types.h"
#include "entity.h"

/**
 * @class Player
 * @brief Represents the player character, including stats, inventory, equipment, and status effects.
 */
class Player {
public:
    Player();
    /**
     * @brief Constructs a Player with the given class.
     * @param playerClass The class of the player (Warrior, Rogue, Mage, etc.).
     */
    explicit Player(PlayerClass playerClass);
    
    /**
     * @brief Sets the player's position on the map.
     * @param x The x coordinate.
     * @param y The y coordinate.
     */
    void set_position(int x, int y);
    const Position& get_position() const;
    /**
     * @brief Moves the player by the given delta.
     * @param dx Change in x.
     * @param dy Change in y.
     */
    void move_by(int dx, int dy);
    
    // Facing direction for first-person view
    Direction facing() const { return facing_; }
    /**
     * @brief Sets the player's facing direction (for first-person view).
     * @param d The new direction.
     */
    void set_facing(Direction d) { facing_ = d; }
    /**
     * @brief Turns the player left (counterclockwise).
     */
    void turn_left();
    /**
     * @brief Turns the player right (clockwise).
     */
    void turn_right();

    Stats& get_stats();
    const Stats& get_stats() const;

    /**
     * @brief Gets the player's class.
     * @return The player's class.
     */
    PlayerClass player_class() const;
    /**
     * @brief Gets the display name for a player class.
     * @param c The player class.
     * @return The class name as a string.
     */
    static std::string class_name(PlayerClass c);

    std::vector<Item>& inventory();
    const std::vector<Item>& inventory() const;

    // Equipment and consumables
    /**
     * @brief Equips an item from the inventory.
     * @param inventoryIndex The index in the inventory.
     * @return True if successful, false otherwise.
     */
    bool equip_item(size_t inventoryIndex);
    /**
     * @brief Unequips an item from a given equipment slot.
     * @param slot The equipment slot.
     * @return True if successful, false otherwise.
     */
    bool unequip(EquipmentSlot slot);
    /**
     * @brief Uses a consumable item from the inventory.
     * @param inventoryIndex The index in the inventory.
     * @return True if successful, false otherwise.
     */
    bool use_consumable(size_t inventoryIndex);

    // Status effects
    /**
     * @brief Applies a status effect to the player.
     * @param effect The status effect to apply.
     */
    void apply_status(const StatusEffect& effect);
    /**
     * @brief Updates all status effects (decrement turns, remove expired).
     */
    void tick_statuses();
    const std::vector<StatusEffect>& statuses() const;
    /**
     * @brief Checks if the player has a specific status effect.
     * @param type The status type to check.
     * @return True if the player has the status, false otherwise.
     */
    bool has_status(StatusType type) const;
    
    // Mana system (for Mage class)
    int get_mana() const { return mana_; }
    int get_max_mana() const { return maxMana_; }
    void use_mana(int amount) { mana_ = std::max(0, mana_ - amount); }
    void restore_mana(int amount) { mana_ = std::min(maxMana_, mana_ + amount); }
    void tick_mana_regen() { restore_mana(1); }  // Regenerate 1 mana per turn
    
    // Cooldown system for abilities
    /**
     * @brief Gets the cooldown for a given combat action.
     * @param action The combat action.
     * @return The number of turns remaining on cooldown.
     */
    int get_cooldown(CombatAction action) const;
    /**
     * @brief Sets the cooldown for a given combat action.
     * @param action The combat action.
     * @param turns The number of turns for the cooldown.
     */
    void set_cooldown(CombatAction action, int turns);
    /**
     * @brief Decrements all ability cooldowns by 1 turn.
     */
    void tick_cooldowns();  // Decrement all cooldowns by 1
    /**
     * @brief Checks if a combat action is on cooldown.
     * @param action The combat action.
     * @return True if on cooldown, false otherwise.
     */
    bool is_on_cooldown(CombatAction action) const;
    
    // Shrine blessing system
    int blessing_health_boost() const { return blessingHealthBoost_; }
    int blessing_damage_boost() const { return blessingDamageBoost_; }
    int blessing_protection() const { return blessingProtection_; }
    bool has_resurrection() const { return hasResurrection_; }
    
    void set_blessing_health_boost(int floors) { blessingHealthBoost_ = floors; }
    void set_blessing_damage_boost(int floors) { blessingDamageBoost_ = floors; }
    void set_blessing_protection(int floors) { blessingProtection_ = floors; }
    void set_has_resurrection(bool val) { hasResurrection_ = val; }
    
    // Convenience stat modifiers
    void add_atk(int val) { baseStats_.attack += val; recompute_effective_stats(); }
    void add_def(int val) { baseStats_.defense += val; recompute_effective_stats(); }
    void add_spd(int val) { baseStats_.speed += val; recompute_effective_stats(); }
    void set_max_hp(int val) { baseStats_.maxHp = val; stats_.maxHp = val; }
    void set_hp(int val) { stats_.hp = val; }
    int hp() const { return stats_.hp; }
    int max_hp() const { return stats_.maxHp; }
    int atk() const { return stats_.attack; }
    int def() const { return stats_.defense; }
    int spd() const { return stats_.speed; }
    
    // Damage and healing
    /**
     * @brief Applies damage to the player.
     * @param dmg The amount of damage to apply.
     */
    void take_damage(int dmg);
    /**
     * @brief Heals the player by a specified amount.
     * @param amount The amount to heal.
     */
    void heal(int amount);
    
    // Clear all status effects
    void clear_statuses() { statuses_.clear(); recompute_effective_stats(); }
    
    // Depth-based bonuses
    /**
     * @brief Sets the current depth for depth-based stat bonuses.
     * @param depth The current depth level.
     */
    void set_depth(int depth) { depth_ = depth; recompute_effective_stats(); }
    int depth() const { return depth_; }

    // Persistence helpers
    const std::unordered_map<EquipmentSlot, Item>& equipment() const;
    const std::unordered_map<EquipmentSlot, Item>& get_equipment() const { return equipment(); }
    /**
     * @brief Sets the player's inventory to the given items.
     * @param items The new inventory items.
     */
    void set_inventory(const std::vector<Item>& items);
    /**
     * @brief Loads player data from persisted stats and inventory.
     * @param effectiveStats The effective stats to load.
     * @param inventoryItems The inventory to load.
     * @param equipmentItems The equipment to load.
     * @param statusList The status effects to load.
     * @param playerClass The class to load.
     */
    void load_from_persisted(const Stats& effectiveStats,
                             const std::vector<Item>& inventoryItems,
                             const std::unordered_map<EquipmentSlot, Item>& equipmentItems,
                             const std::vector<StatusEffect>& statusList,
                             PlayerClass playerClass = PlayerClass::Warrior);

    char glyph() const;
    const std::string& color() const;

private:
    /**
     * @brief Recomputes the player's effective stats from base, equipment, and statuses.
     */
    void recompute_effective_stats();
    /**
     * @brief Applies class-specific bonuses to the player.
     */
    void apply_class_bonuses();

    Position position_{};
    Stats baseStats_{};
    Stats stats_{}; // effective stats (base + equipment + statuses)
    PlayerClass class_ = PlayerClass::Warrior;
    Direction facing_ = Direction::North;
    std::vector<Item> inventory_{};
    std::unordered_map<EquipmentSlot, Item> equipment_{};
    std::vector<StatusEffect> statuses_{};
    char glyph_ = '@';
    std::string color_ = "\033[38;5;208m";
    
    // Mana for spellcasting
    int mana_ = 30;
    int maxMana_ = 30;
    
    // Shrine blessings (floor duration)
    int blessingHealthBoost_ = 0;
    int blessingDamageBoost_ = 0;
    int blessingProtection_ = 0;
    bool hasResurrection_ = false;
    
    // Ability cooldowns (action -> turns remaining)
    std::unordered_map<CombatAction, int> cooldownTurns_{};
    
    // Current depth for depth-based bonuses
    int depth_ = 1;
};


