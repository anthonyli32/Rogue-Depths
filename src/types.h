#pragma once

#include <cstdint>
#include <string>

enum class Difficulty {
    Explorer,
    Adventurer,
    Nightmare
};

enum class TileType {
    Wall,
    Floor,
    Door,
    StairsUp,
    StairsDown,
    Trap,
    Shrine,
    // Environmental hazards
    Water,      // Slows movement
    Lava,       // Instant death / heavy damage
    Chasm,      // Impassable, can push enemies into
    DeepWater,  // Impassable without swimming
    Unknown
};

// Monster types with standard roguelike glyphs
enum class EnemyType {
    // Weak monsters (lowercase glyphs)
    Rat,        // 'r' - very weak, fast
    Spider,     // 's' - weak, can poison
    Goblin,     // 'g' - common early enemy
    Kobold,     // 'k' - weak but cunning
    Orc,        // 'o' - stronger melee
    Zombie,     // 'z' - slow but tough
    // Ranged monsters
    Archer,     // 'a' - goblin archer, ranged attacks
    // Strong monsters (uppercase glyphs)
    Gnome,      // 'G' - magic user
    Ogre,       // 'O' - heavy hitter
    Troll,      // 'T' - regenerates
    Dragon,     // 'D' - boss tier
    Lich,       // 'L' - undead boss
    // Boss monsters
    StoneGolem, // 'Ω' - Floor 4 boss
    ShadowLord, // 'Σ' - Floor 8 boss
    // Special
    CorpseEnemy // 'C' - from corpse run
};

// Player classes
enum class PlayerClass {
    Warrior,    // +3 HP, +1 ATK
    Rogue,      // +2 ATK, +1 SPD
    Mage        // -1 HP, +2 DEF
};

enum class ItemType {
    Weapon,
    Armor,
    Consumable,
    Quest,
    Misc
};

enum class Rarity {
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary
};

enum class EquipmentSlot {
    Head,
    Chest,
    Weapon,
    Offhand,
    Accessory
};

enum class StatusType {
    None,
    Bleed,
    Poison,
    Fortify,
    Haste,
    Burn,
    Freeze,
    Stun
};

// Height levels for flying enemies
enum class HeightLevel {
    Ground,     // Normal ground-level enemies (melee can hit)
    LowAir,     // Hovering enemies (melee with jump attack can hit)
    Flying      // High-flying enemies (only ranged/magic can hit)
};

// Attack type for determining what can hit flying enemies
enum class AttackType {
    Melee,      // Can only hit Ground enemies
    Ranged,     // Can hit any height
    Magic       // Can hit any height
};

// Combat action choices for player during combat
enum class CombatAction {
    // Legacy actions (kept for backward compatibility)
    ATTACK,      // Basic melee attack (maps to SLASH)
    RANGED,      // Ranged attack (maps to SHOOT)
    DEFEND,      // Defensive stance
    SKILL,       // Class-specific ability
    CONSUMABLE,  // Use potion/scroll during combat
    RETREAT,     // Attempt to move away from enemy
    WAIT,        // Pass turn / do nothing
    
    // Expanded melee actions
    SLASH,       // Quick attack (20 energy), 1x damage, no telegraph
    POWER_STRIKE,// Heavy attack (70 energy), 1.5x damage, telegraphed
    TACKLE,      // Knockdown attack (50 energy), 0.8x damage + stun
    WHIRLWIND,   // AOE melee (70 energy), hits all adjacent, 0.7x damage
    
    // Expanded ranged actions
    SHOOT,       // Standard arrow (50 energy), 1x damage, telegraphed
    SNIPE,       // Aimed shot (70 energy), 1.5x damage + accuracy boost
    MULTISHOT,   // AOE arrows (80 energy), hits 3 targets, 0.8x damage each
    
    // Magic actions
    FIREBALL,    // AOE damage + burn (70 energy), 1.2x damage
    FROST_BOLT,  // Single target + freeze (60 energy), 1x damage
    TELEPORT,    // Reposition (50 energy), no damage, change distance
    
    // Movement actions
    ADVANCE,     // Move closer (0 energy), reduce distance by 1-2
    // RETREAT already exists above
    CIRCLE,      // Strafe left/right (30 energy), changes X position
    REPOSITION,  // Move to specific adjacent tile (40 energy)
    
    // Defensive actions
    BRACE        // Prepare impact (30 energy), -30% damage next turn
};

// Item affixes for enhanced loot system
enum class ItemAffix {
    NONE,
    // Weapon affixes
    LIFESTEAL,      // Heal 25% of damage dealt
    BURNING,        // Apply burn status effect
    FROST,          // Apply freeze/slow status effect
    POISON_COAT,    // Apply poison status effect
    SLOW_TARGET,    // Reduce enemy speed on hit
    VORPAL,         // 10% chance instant kill (non-boss)
    VAMPIRIC,       // Heal 50% of damage dealt (rare)
    // Armor affixes
    THORNS,         // Return 25% of damage taken
    FIRE_RESIST,    // Reduce fire damage by 50%
    COLD_RESIST,    // Reduce cold damage by 50%
    EVASION,        // +20% dodge chance
    HEALTH_REGEN,   // +1 HP per turn
    REFLECTIVE      // Return 50% of damage taken (rare)
};

// Room types for dungeon variety
enum class RoomType {
    GENERIC,        // Normal exploration room
    TREASURE,       // Extra loot concentration (3x items)
    SHRINE,         // Stat boost or curse
    SHOP,           // Buy/sell items (future feature)
    BOSS_CHAMBER,   // Boss fight room
    TRAP_CHAMBER,   // Dangerous but high reward
    SECRET,         // Hidden room (20% chance)
    SANCTUARY       // Safe resting point (no enemies)
};

// Shrine blessing types
enum class ShrineBlessing {
    STAT_BOOST,     // +1 to all stats
    HEALTH_BOOST,   // Double max HP for 3 floors
    DAMAGE_BOOST,   // Double ATK for 3 floors
    PROTECTION,     // -50% damage for 3 floors
    RESURRECTION,   // Extra life (revive once if killed)
    CURSE_REMOVAL,  // Remove all negative status effects
    CURSE,          // Random penalty
    NOTHING         // No effect
};

// Trap types for dungeon hazards
enum class TrapType {
    SPIKE_PIT,      // 5-10 fall damage
    POISON_CLOUD,   // Apply poison status
    TELEPORT,       // Random teleport
    EXPLOSIVE,      // 10-20 area damage
    SLOW_FIELD,     // Apply slow status
    NONE
};

// Combat arena hazards (affect combat positioning)
enum class CombatHazard {
    SPIKE_FLOOR,    // Damage on step (5 damage)
    FIRE_PILLAR,    // Apply burn status
    ICE_PATCH,      // Apply freeze status (slow)
    POISON_CLOUD,   // Apply poison status
    HEALING_SPRING, // Restore health (5 HP)
    NONE
};

// Forward declarations
class Dungeon;
class Player;
class MessageLog;
#include <random>  // For std::mt19937

struct Position {
    int x = 0;
    int y = 0;
};

// 3D positioning for tactical combat (simulated depth)
struct Position3D {
    int x = 0;
    int y = 0;
    int depth = 0;  // 0=closest, 10=farthest (simulated Z-axis)
    
    // Calculate raw distance to another Position3D
    int calculate_distance(const Position3D& other) const {
        int dx = abs(other.x - x);
        int dy = abs(other.y - y);
        // Depth weighted by DEPTH_WEIGHT (1.5x) - using integer math: (depth_diff * 3) / 2
        int dz = (abs(other.depth - depth) * 3) / 2;
        return dx + dy + dz;
    }
    
    // Get distance category
    // Implemented in combat.cpp to avoid circular dependency
    // CombatDistance get_distance_to(const Position3D& other) const;
};

// Combat arena with hazards (defined after Position3D)
struct CombatArena {
    std::vector<CombatHazard> hazards;
    std::vector<Position3D> hazardPositions;
    
    // Check if position has a hazard and apply effect
    bool apply_hazard(const Position3D& pos, Player& player, MessageLog& log) const;
    
    // Generate random hazards for an arena
    static CombatArena generate_random(int hazardCount, const Dungeon& dungeon, std::mt19937& rng);
};

// Combat distance zones for tactical positioning
enum class CombatDistance {
    MELEE,      // 0-1: Direct contact, touching
    CLOSE,      // 2-3: Adjacent squares, near
    MEDIUM,     // 4-6: Across room, tactical
    FAR,        // 7-10: Long range, safe
    EXTREME     // 10+: Almost unreachable
};

// Direction for facing
enum class Direction {
    North,  // Up
    South,  // Down
    East,   // Right
    West    // Left
};

// UI View modes for tab-based switching
enum class UIView {
    MAP,            // Main gameplay view (default)
    INVENTORY,      // Full inventory list with details
    STATS,          // Character stats, class bonuses, kill count
    EQUIPMENT,      // Equipped items with slot visualization
    MESSAGE_LOG     // Scrollable message history
};

// Death cause for corpse display
enum class DeathCause {
    Enemy,      // Killed by enemy (skull)
    Trap,       // Killed by trap (lightning)
    Environment,// Killed by lava/chasm (skull with fire)
    Unknown
};

// Corpse data for corpse run mechanic
struct CorpseData {
    Position position;
    int floor = 1;
    int runsSinceDeath = 0;  // For decay calculation
    DeathCause cause = DeathCause::Unknown;
    bool hasLoot = true;     // False after looted
    
    // Get decay level (0 = fresh, 1 = decayed, 2 = ancient)
    int get_decay_level() const {
        if (runsSinceDeath <= 2) return 0;  // Fresh
        if (runsSinceDeath <= 5) return 1;  // Decayed
        return 2;                            // Ancient
    }
    
    // Get glyph based on death cause
    char get_glyph() const {
        switch (cause) {
            case DeathCause::Enemy:      return 'X';  // Skull
            case DeathCause::Trap:       return '!';  // Lightning
            case DeathCause::Environment:return '*';  // Fire
            default:                     return 'X';
        }
    }
};


