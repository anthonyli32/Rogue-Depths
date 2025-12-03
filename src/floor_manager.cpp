#include "floor_manager.h"
#include "logger.h"
#include <random>
#include <algorithm>

// Global instance
static FloorManager g_floorManager;

namespace game {
    FloorManager& floors() {
        return g_floorManager;
    }
}

FloorManager::FloorManager() {}

void FloorManager::init(unsigned int baseSeed) {
    baseSeed_ = baseSeed;
    currentFloor_ = 1;
    floors_.clear();
    LOG_INFO("FloorManager initialized with seed: " + std::to_string(baseSeed));
}

bool FloorManager::has_floor(int floorNum) const {
    return floors_.find(floorNum) != floors_.end();
}

FloorData& FloorManager::get_floor(int floorNum) {
    if (!has_floor(floorNum)) {
        generate_floor(floorNum);
    }
    return floors_[floorNum];
}

void FloorManager::set_current_floor(int floorNum) {
    currentFloor_ = floorNum;
    // Ensure floor exists
    get_floor(floorNum);
}

bool FloorManager::descend() {
    if (currentFloor_ >= maxFloor_) {
        return false;  // Already at max floor (victory condition)
    }
    currentFloor_++;
    get_floor(currentFloor_);  // Generate if needed
    LOG_INFO("Descended to floor " + std::to_string(currentFloor_));
    return true;
}

bool FloorManager::ascend() {
    if (currentFloor_ <= 1) {
        return false;  // Can't go above floor 1
    }
    currentFloor_--;
    LOG_INFO("Ascended to floor " + std::to_string(currentFloor_));
    return true;
}

Position FloorManager::get_start_position() const {
    if (floors_.find(currentFloor_) != floors_.end()) {
        const auto& floor = floors_.at(currentFloor_);
        // If descending, start at stairs up position
        // If ascending, start at stairs down position
        // For now, use stairs up as default start
        return floor.stairsUp;
    }
    return {5, 5};  // Fallback
}

void FloorManager::generate_floor(int floorNum) {
    LOG_INFO("Generating floor " + std::to_string(floorNum));
    
    FloorData floor;
    floor.seed = baseSeed_ + static_cast<unsigned int>(floorNum * 1000);
    floor.visited = true;
    
    // Generate dungeon layout
    Position start, stairsDown;
    floor.dungeon.generate(floor.seed, start, stairsDown);
    
    // Set stairs positions
    floor.stairsUp = start;  // Entry point (stairs up to previous floor)
    floor.stairsDown = stairsDown;  // Exit point (stairs down to next floor)
    
    // Populate with enemies
    populate_enemies(floor, floorNum);
    
    // Store in cache
    floors_[floorNum] = std::move(floor);
    
    LOG_INFO("Floor " + std::to_string(floorNum) + " generated with " + 
             std::to_string(floors_[floorNum].enemies.size()) + " enemies");
}

void FloorManager::populate_enemies(FloorData& floor, int depth) {
    std::mt19937 rng(floor.seed + 12345);
    
    // Number of enemies scales with depth
    int baseEnemies = 3;
    int enemyCount = baseEnemies + depth;
    
    std::uniform_int_distribution<int> xDist(1, floor.dungeon.width() - 2);
    std::uniform_int_distribution<int> yDist(1, floor.dungeon.height() - 2);
    std::uniform_int_distribution<int> typeDist(0, 100);
    
    for (int i = 0; i < enemyCount; i++) {
        // Find valid spawn position
        int x, y;
        int attempts = 0;
        do {
            x = xDist(rng);
            y = yDist(rng);
            attempts++;
        } while (!floor.dungeon.is_walkable(x, y) && attempts < 100);
        
        if (attempts >= 100) continue;  // Couldn't find valid position
        
        // Determine enemy type based on depth
        EnemyType type;
        int roll = typeDist(rng);
        
        if (depth <= 2) {
            // Early floors: rats, spiders, goblins
            if (roll < 40) type = EnemyType::Rat;
            else if (roll < 70) type = EnemyType::Spider;
            else type = EnemyType::Goblin;
        } else if (depth <= 4) {
            // Mid-early: goblins, kobolds, orcs
            if (roll < 30) type = EnemyType::Goblin;
            else if (roll < 60) type = EnemyType::Kobold;
            else if (roll < 85) type = EnemyType::Orc;
            else type = EnemyType::Zombie;
        } else if (depth <= 6) {
            // Mid: orcs, zombies, gnomes
            if (roll < 25) type = EnemyType::Orc;
            else if (roll < 50) type = EnemyType::Zombie;
            else if (roll < 75) type = EnemyType::Gnome;
            else type = EnemyType::Ogre;
        } else if (depth <= 8) {
            // Late-mid: gnomes, ogres, trolls
            if (roll < 30) type = EnemyType::Gnome;
            else if (roll < 60) type = EnemyType::Ogre;
            else if (roll < 90) type = EnemyType::Troll;
            else type = EnemyType::Dragon;
        } else {
            // Deep floors: trolls, dragons, liches
            if (roll < 30) type = EnemyType::Troll;
            else if (roll < 60) type = EnemyType::Dragon;
            else type = EnemyType::Lich;
        }
        
        Enemy enemy(type);
        enemy.set_position(x, y);
        
        // Scale stats with depth
        enemy.stats().hp += depth / 2;
        enemy.stats().maxHp = enemy.stats().hp;
        enemy.stats().attack += depth / 3;
        
        floor.enemies.push_back(enemy);
    }
}

void FloorManager::trim_cache(int maxFloors) {
    if (static_cast<int>(floors_.size()) <= maxFloors) return;
    
    // Find floors furthest from current
    std::vector<int> floorNums;
    for (const auto& [num, _] : floors_) {
        floorNums.push_back(num);
    }
    
    // Sort by distance from current floor
    std::sort(floorNums.begin(), floorNums.end(), [this](int a, int b) {
        return std::abs(a - currentFloor_) > std::abs(b - currentFloor_);
    });
    
    // Remove furthest floors until under limit
    while (static_cast<int>(floors_.size()) > maxFloors && !floorNums.empty()) {
        int toRemove = floorNums.front();
        floorNums.erase(floorNums.begin());
        
        if (toRemove != currentFloor_) {
            floors_.erase(toRemove);
            LOG_INFO("Trimmed floor " + std::to_string(toRemove) + " from cache");
        }
    }
}

int FloorManager::floors_visited() const {
    int count = 0;
    for (const auto& [_, floor] : floors_) {
        if (floor.visited) count++;
    }
    return count;
}

void FloorManager::save_floor(int floorNum, std::ostream& out) const {
    // Placeholder for serialization
    // Would serialize dungeon, enemies, items, etc.
    (void)floorNum;
    (void)out;
}

bool FloorManager::load_floor(int floorNum, std::istream& in) {
    // Placeholder for deserialization
    (void)floorNum;
    (void)in;
    return false;
}


