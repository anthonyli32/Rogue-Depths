#include "player.h"

Player::Player() {
    baseStats_ = stats_;
    apply_class_bonuses();
    recompute_effective_stats();
}

Player::Player(PlayerClass playerClass) : class_(playerClass) {
    baseStats_ = stats_;
    apply_class_bonuses();
    recompute_effective_stats();
}

void Player::apply_class_bonuses() {
    switch (class_) {
        case PlayerClass::Warrior:
            // +3 HP, +1 ATK
            baseStats_.maxHp += 3;
            baseStats_.hp += 3;
            baseStats_.attack += 1;
            break;
        case PlayerClass::Rogue:
            // +2 ATK, +1 SPD
            baseStats_.attack += 2;
            baseStats_.speed += 1;
            break;
        case PlayerClass::Mage:
            // -1 HP, +2 DEF
            baseStats_.maxHp -= 1;
            baseStats_.hp -= 1;
            baseStats_.defense += 2;
            break;
    }
}

PlayerClass Player::player_class() const {
    return class_;
}

std::string Player::class_name(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior: return "Warrior";
        case PlayerClass::Rogue:   return "Rogue";
        case PlayerClass::Mage:    return "Mage";
        default:                   return "Unknown";
    }
}

void Player::turn_left() {
    switch (facing_) {
        case Direction::North: facing_ = Direction::West; break;
        case Direction::West:  facing_ = Direction::South; break;
        case Direction::South: facing_ = Direction::East; break;
        case Direction::East:  facing_ = Direction::North; break;
    }
}

void Player::turn_right() {
    switch (facing_) {
        case Direction::North: facing_ = Direction::East; break;
        case Direction::East:  facing_ = Direction::South; break;
        case Direction::South: facing_ = Direction::West; break;
        case Direction::West:  facing_ = Direction::North; break;
    }
}

void Player::set_position(int x, int y) {
    position_.x = x;
    position_.y = y;
}

const Position& Player::get_position() const {
    return position_;
}

void Player::move_by(int dx, int dy) {
    position_.x += dx;
    position_.y += dy;
}

Stats& Player::get_stats() {
    return stats_;
}

const Stats& Player::get_stats() const {
    return stats_;
}

std::vector<Item>& Player::inventory() {
    return inventory_;
}

const std::vector<Item>& Player::inventory() const {
    return inventory_;
}

bool Player::equip_item(size_t inventoryIndex) {
    if (inventoryIndex >= inventory_.size()) return false;
    Item& it = inventory_[inventoryIndex];
    if (!it.isEquippable) return false;
    
    // Dual wielding: Weapons can be equipped to either Weapon or Offhand slot
    EquipmentSlot targetSlot = it.slot;
    if (it.type == ItemType::Weapon) {
        // If Weapon slot is empty, use it; otherwise use Offhand
        if (equipment_.find(EquipmentSlot::Weapon) == equipment_.end()) {
            targetSlot = EquipmentSlot::Weapon;
        } else if (equipment_.find(EquipmentSlot::Offhand) == equipment_.end()) {
            targetSlot = EquipmentSlot::Offhand;
        } else {
            // Both slots full - replace Weapon slot (main hand takes priority)
            targetSlot = EquipmentSlot::Weapon;
        }
    }
    
    // Unequip existing in that slot (moves it back to inventory)
    auto found = equipment_.find(targetSlot);
    if (found != equipment_.end()) {
        inventory_.push_back(found->second);
        equipment_.erase(found);
    }
    equipment_[targetSlot] = it;
    // Remove from inventory by swapping with back
    inventory_[inventoryIndex] = inventory_.back();
    inventory_.pop_back();
    recompute_effective_stats();
    return true;
}

bool Player::unequip(EquipmentSlot slot) {
    auto it = equipment_.find(slot);
    if (it == equipment_.end()) return false;
    inventory_.push_back(it->second);
    equipment_.erase(it);
    recompute_effective_stats();
    return true;
}

bool Player::use_consumable(size_t inventoryIndex) {
    if (inventoryIndex >= inventory_.size()) return false;
    Item& it = inventory_[inventoryIndex];
    if (!it.isConsumable) return false;
    if (it.healAmount > 0) {
        // Use heal() method to ensure baseStats_.hp is also updated
        heal(it.healAmount);
    }
    if (it.onUseStatus != StatusType::None && it.onUseDuration > 0) {
        apply_status(StatusEffect{it.onUseStatus, it.onUseDuration, it.onUseMagnitude});
    }
    // consume
    inventory_[inventoryIndex] = inventory_.back();
    inventory_.pop_back();
    return true;
}

void Player::apply_status(const StatusEffect& effect) {
    // FIXED: Prevent duplicate stacking, always refresh duration and use max magnitude
    for (auto& s : statuses_) {
        if (s.type == effect.type) {
            s.remainingTurns = std::max(s.remainingTurns, effect.remainingTurns); // IMPROVED: Use max duration
            s.magnitude = std::max(s.magnitude, effect.magnitude);
            recompute_effective_stats();
            return;
        }
    }
    statuses_.push_back(effect);
    recompute_effective_stats();
}

void Player::tick_statuses() {
    // Damage-over-time effects
    for (auto& s : statuses_) {
        if (s.type == StatusType::Bleed || s.type == StatusType::Poison || s.type == StatusType::Burn) {
            int dmg = std::max(1, s.magnitude);
            stats_.hp -= dmg;
            // Clamp HP to 0 minimum (player dies if HP <= 0, checked elsewhere)
            if (stats_.hp < 0) {
                stats_.hp = 0;
            }
        }
        s.remainingTurns -= 1;
    }
    // purge expired
    std::vector<StatusEffect> next;
    next.reserve(statuses_.size());
    for (auto& s : statuses_) {
        if (s.remainingTurns > 0) next.push_back(s);
    }
    statuses_.swap(next);
    recompute_effective_stats();
}

const std::vector<StatusEffect>& Player::statuses() const {
    return statuses_;
}

bool Player::has_status(StatusType type) const {
    for (const auto& s : statuses_) {
        if (s.type == type && s.remainingTurns > 0) {
            return true;
        }
    }
    return false;
}

const std::unordered_map<EquipmentSlot, Item>& Player::equipment() const {
    return equipment_;
}

void Player::set_inventory(const std::vector<Item>& items) {
    inventory_ = items;
}

void Player::load_from_persisted(const Stats& effectiveStats,
                                 const std::vector<Item>& inventoryItems,
                                 const std::unordered_map<EquipmentSlot, Item>& equipmentItems,
                                 const std::vector<StatusEffect>& statusList,
                                 PlayerClass playerClass) {
    // Set player class
    class_ = playerClass;
    // Set current/effective stats
    stats_ = effectiveStats;
    // Store inventory/equipment/statuses
    inventory_ = inventoryItems;
    equipment_ = equipmentItems;
    statuses_ = statusList;
    // Compute a plausible base by reversing contributions
    baseStats_ = stats_;
    for (const auto& kv : equipment_) {
        baseStats_.attack -= kv.second.attackBonus;
        baseStats_.defense -= kv.second.defenseBonus;
        baseStats_.maxHp -= kv.second.hpBonus;
    }
    for (const auto& s : statuses_) {
        if (s.type == StatusType::Fortify) {
            baseStats_.defense -= s.magnitude;
        } else if (s.type == StatusType::Haste) {
            baseStats_.speed -= s.magnitude;
        }
    }
    // Ensure base HP isn't higher than effective max; keep current hp as loaded
    if (baseStats_.hp > baseStats_.maxHp) baseStats_.hp = baseStats_.maxHp;
}

void Player::recompute_effective_stats() {
    // IMPORTANT: Preserve current HP before resetting stats
    int currentHp = stats_.hp;
    
    stats_ = baseStats_;
    // Equipment bonuses
    for (const auto& kv : equipment_) {
        const Item& it = kv.second;
        stats_.attack += it.attackBonus;
        stats_.defense += it.defenseBonus;
        stats_.maxHp += it.hpBonus;
    }
    
    // Depth-based bonuses: +3 attack and +5 max HP per depth level
    stats_.attack += depth_ * 3;
    stats_.maxHp += depth_ * 5;
    
    // Status effects
    for (const auto& s : statuses_) {
        if (s.type == StatusType::Fortify) {
            stats_.defense += s.magnitude;
        } else if (s.type == StatusType::Haste) {
            stats_.speed += s.magnitude;
        }
    }
    
    // Restore current HP (don't reset to max!)
    // Only use baseStats_.hp on first initialization (when currentHp == 0 or matches base)
    if (currentHp > 0) {
        stats_.hp = currentHp;
    }
    
    // DO NOT update baseStats_.maxHp here - it should remain as the base value
    // Depth and equipment bonuses are applied to stats_.maxHp but should NOT be stored in baseStats_.maxHp
    // This prevents bonuses from accumulating when recompute_effective_stats() is called multiple times
    
    // Clamp current HP to new max (in case max HP decreased)
    if (stats_.hp > stats_.maxHp) stats_.hp = stats_.maxHp;
}
char Player::glyph() const {
    return glyph_;
}

const std::string& Player::color() const {
    return color_;
}

void Player::take_damage(int dmg) {
    // Apply protection blessing (50% damage reduction)
    if (blessingProtection_ > 0) {
        dmg = dmg / 2;
    }
    
    stats_.hp -= dmg;
    if (stats_.hp < 0) stats_.hp = 0;
}

void Player::heal(int amount) {
    stats_.hp = std::min(stats_.hp + amount, stats_.maxHp);
    
    // Always update baseStats_.hp to match stats_.hp after healing
    // This ensures recompute_effective_stats() preserves the healed HP
    // NOTE: We do NOT update baseStats_.maxHp - maxHp should only change from equipment/depth, not healing
    baseStats_.hp = stats_.hp;
}

// Cooldown system implementation
int Player::get_cooldown(CombatAction action) const {
    auto it = cooldownTurns_.find(action);
    return (it != cooldownTurns_.end()) ? it->second : 0;
}

void Player::set_cooldown(CombatAction action, int turns) {
    if (turns > 0) {
        cooldownTurns_[action] = turns;
    } else {
        cooldownTurns_.erase(action);
    }
}

void Player::tick_cooldowns() {
    for (auto it = cooldownTurns_.begin(); it != cooldownTurns_.end();) {
        it->second--;
        if (it->second <= 0) {
            it = cooldownTurns_.erase(it);
        } else {
            ++it;
        }
    }
}

bool Player::is_on_cooldown(CombatAction action) const {
    return get_cooldown(action) > 0;
}


