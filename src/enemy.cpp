#include "enemy.h"
#include "constants.h"
#include "glyphs.h"
#include "ui.h"

#include <algorithm>

// Legacy constructor
Enemy::Enemy(EnemyArchetype archetype, char glyph, const std::string& color)
    : archetype_(archetype), glyph_(glyph), color_(color), name_("Enemy") {}

// New constructor using EnemyType
Enemy::Enemy(EnemyType type, EnemyArchetype archetype)
    : archetype_(archetype), enemyType_(type) {
    glyph_ = glyph_for_type(type);
    color_ = color_for_type(type);
    name_ = name_for_type(type);
    stats_ = base_stats_for_type(type);
    height_ = default_height_for_type(type);
}

const Position& Enemy::get_position() const {
    return position_;
}

void Enemy::set_position(int x, int y) {
    position_.x = x;
    position_.y = y;
}

void Enemy::move_by(int dx, int dy) {
    position_.x += dx;
    position_.y += dy;
}

EnemyArchetype Enemy::archetype() const {
    return archetype_;
}

EnemyType Enemy::enemy_type() const {
    return enemyType_;
}

Stats& Enemy::stats() {
    return stats_;
}

const Stats& Enemy::stats() const {
    return stats_;
}

EnemyKnowledge& Enemy::knowledge() {
    return knowledge_;
}

const EnemyKnowledge& Enemy::knowledge() const {
    return knowledge_;
}

char Enemy::glyph() const {
    return glyph_;
}

const std::string& Enemy::color() const {
    return color_;
}

const std::string& Enemy::name() const {
    return name_;
}

const std::vector<StatusEffect>& Enemy::statuses() const {
    return statuses_;
}

bool Enemy::has_status(StatusType type) const {
    for (const auto& s : statuses_) {
        if (s.type == type && s.remainingTurns > 0) {
            return true;
        }
    }
    return false;
}

void Enemy::apply_status(const StatusEffect& effect) {
    // FIXED: Prevent duplicate stacking, always refresh duration and use max magnitude
    for (auto& s : statuses_) {
        if (s.type == effect.type) {
            s.remainingTurns = std::max(s.remainingTurns, effect.remainingTurns); // IMPROVED: Use max duration
            s.magnitude = std::max(s.magnitude, effect.magnitude);
            return;
        }
    }
    statuses_.push_back(effect);
}

void Enemy::tick_statuses(MessageLog& log) {
    if (statuses_.empty()) return;
    for (auto& s : statuses_) {
        if (s.type == StatusType::Bleed || s.type == StatusType::Poison || s.type == StatusType::Burn) {
            int dmg = std::max(1, s.magnitude);
            stats_.hp -= dmg;
            std::string source = "poison";
            if (s.type == StatusType::Bleed) source = "bleeding";
            else if (s.type == StatusType::Burn) source = "burn";
            log.add(MessageType::Damage, name_ + " suffers " + std::to_string(dmg) + " damage from " + source + "!");
        }
        s.remainingTurns -= 1;
    }
    statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(),
                                   [](const StatusEffect& s){ return s.remainingTurns <= 0; }),
                    statuses_.end());
}

HeightLevel Enemy::height() const {
    return height_;
}

void Enemy::set_height(HeightLevel h) {
    height_ = h;
}

bool Enemy::is_grounded() const {
    return height_ == HeightLevel::Ground;
}

void Enemy::descend() {
    // Flying enemies can descend one level
    if (height_ == HeightLevel::Flying) {
        height_ = HeightLevel::LowAir;
    } else if (height_ == HeightLevel::LowAir) {
        height_ = HeightLevel::Ground;
    }
}

// Static: get default height for enemy type
HeightLevel Enemy::default_height_for_type(EnemyType type) {
    switch (type) {
        // Flying enemies
        case EnemyType::Dragon:
            return HeightLevel::Flying;
        // Hovering enemies (ghosts, magical creatures)
        case EnemyType::Lich:
        case EnemyType::CorpseEnemy:
            return HeightLevel::LowAir;
        // Ground enemies (default)
        default:
            return HeightLevel::Ground;
    }
}

// Static: get glyph for enemy type
// Returns first character of glyph string (enemy glyphs are single chars)
char Enemy::glyph_for_type(EnemyType type) {
    switch (type) {
        case EnemyType::Rat:         return glyphs::rat()[0];
        case EnemyType::Spider:      return glyphs::spider()[0];
        case EnemyType::Goblin:      return glyphs::goblin()[0];
        case EnemyType::Kobold:      return glyphs::kobold()[0];
        case EnemyType::Orc:         return glyphs::orc()[0];
        case EnemyType::Zombie:      return glyphs::zombie()[0];
        case EnemyType::Archer:      return 'a';  // Goblin Archer
        case EnemyType::Gnome:       return glyphs::gnome()[0];
        case EnemyType::Ogre:        return glyphs::ogre()[0];
        case EnemyType::Troll:       return glyphs::troll()[0];
        case EnemyType::Dragon:      return glyphs::dragon()[0];
        case EnemyType::Lich:        return glyphs::lich()[0];
        case EnemyType::StoneGolem:  return glyphs::golem()[0];
        case EnemyType::ShadowLord:  return glyphs::demon()[0];
        case EnemyType::CorpseEnemy: return glyphs::vengeful_spirit()[0];
        default:                     return 'e';
    }
}

// Static: get color for enemy type
std::string Enemy::color_for_type(EnemyType type) {
    switch (type) {
        case EnemyType::Rat:
        case EnemyType::Spider:
            return constants::color_monster_weak;
        case EnemyType::Goblin:
        case EnemyType::Kobold:
        case EnemyType::Archer:
            return constants::color_monster_common;
        case EnemyType::Orc:
        case EnemyType::Zombie:
            return constants::color_monster_strong;
        case EnemyType::Gnome:
        case EnemyType::Ogre:
        case EnemyType::Troll:
            return constants::color_monster_elite;
        case EnemyType::Dragon:
        case EnemyType::Lich:
        case EnemyType::StoneGolem:
        case EnemyType::ShadowLord:
            return constants::color_monster_boss;
        case EnemyType::CorpseEnemy:
            return constants::color_corpse;
        default:
            return constants::color_monster_common;
    }
}

// Static: get name for enemy type
std::string Enemy::name_for_type(EnemyType type) {
    switch (type) {
        case EnemyType::Rat:         return "Rat";
        case EnemyType::Spider:      return "Spider";
        case EnemyType::Goblin:      return "Goblin";
        case EnemyType::Kobold:      return "Kobold";
        case EnemyType::Orc:         return "Orc";
        case EnemyType::Zombie:      return "Zombie";
        case EnemyType::Archer:      return "Goblin Archer";
        case EnemyType::Gnome:       return "Gnome";
        case EnemyType::Ogre:        return "Ogre";
        case EnemyType::Troll:       return "Troll";
        case EnemyType::Dragon:      return "Dragon";
        case EnemyType::Lich:        return "Lich";
        case EnemyType::StoneGolem:  return "Stone Golem";
        case EnemyType::ShadowLord:  return "Shadow Lord";
        case EnemyType::CorpseEnemy: return "Vengeful Spirit";
        default:                     return "Unknown";
    }
}

// Static: get base stats for enemy type (scales with depth later)
Stats Enemy::base_stats_for_type(EnemyType type) {
    Stats s;
    switch (type) {
        case EnemyType::Rat:
            s.maxHp = 3; s.hp = 3; s.attack = 1; s.defense = 0; s.speed = 15;
            break;
        case EnemyType::Spider:
            s.maxHp = 4; s.hp = 4; s.attack = 2; s.defense = 0; s.speed = 12;
            break;
        case EnemyType::Goblin:
            s.maxHp = 6; s.hp = 6; s.attack = 2; s.defense = 1; s.speed = 10;
            break;
        case EnemyType::Kobold:
            s.maxHp = 5; s.hp = 5; s.attack = 3; s.defense = 0; s.speed = 11;
            break;
        case EnemyType::Orc:
            s.maxHp = 10; s.hp = 10; s.attack = 4; s.defense = 2; s.speed = 8;
            break;
        case EnemyType::Zombie:
            s.maxHp = 12; s.hp = 12; s.attack = 3; s.defense = 3; s.speed = 5;
            break;
        case EnemyType::Archer:
            // Goblin Archer: Lower HP, ranged damage, keeps distance
            s.maxHp = 5; s.hp = 5; s.attack = 4; s.defense = 0; s.speed = 11;
            break;
        case EnemyType::Gnome:
            s.maxHp = 8; s.hp = 8; s.attack = 5; s.defense = 1; s.speed = 10;
            break;
        case EnemyType::Ogre:
            s.maxHp = 20; s.hp = 20; s.attack = 6; s.defense = 3; s.speed = 6;
            break;
        case EnemyType::Troll:
            s.maxHp = 25; s.hp = 25; s.attack = 5; s.defense = 4; s.speed = 7;
            break;
        case EnemyType::Dragon:
            // Final boss: Heavily nerfed - much weaker now
            s.maxHp = 20; s.hp = 20; s.attack = 4; s.defense = 2; s.speed = 9;
            break;
        case EnemyType::Lich:
            s.maxHp = 40; s.hp = 40; s.attack = 12; s.defense = 3; s.speed = 10;
            break;
        case EnemyType::StoneGolem:
            // Floor 4 boss: Heavily nerfed - much weaker now
            s.maxHp = 30; s.hp = 30; s.attack = 3; s.defense = 3; s.speed = 4;
            break;
        case EnemyType::ShadowLord:
            // Floor 8 boss: Heavily nerfed - much weaker now
            s.maxHp = 25; s.hp = 25; s.attack = 4; s.defense = 2; s.speed = 14;
            break;
        case EnemyType::CorpseEnemy:
            s.maxHp = 15; s.hp = 15; s.attack = 5; s.defense = 2; s.speed = 10;
            break;
        default:
            s.maxHp = 6; s.hp = 6; s.attack = 2; s.defense = 1; s.speed = 10;
            break;
    }
    return s;
}


