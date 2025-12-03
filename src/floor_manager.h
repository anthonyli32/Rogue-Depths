#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include "dungeon.h"
#include "enemy.h"
#include "types.h"

// Data for a single floor
struct FloorData {
    Dungeon dungeon;
    std::vector<Enemy> enemies;
    std::vector<Item> items;  // Items on ground
    Position stairsUp;
    Position stairsDown;
    bool visited = false;
    bool cleared = false;  // All enemies defeated
    unsigned int seed = 0;
};

// Manages multiple floors with on-demand generation and caching
class FloorManager {
public:
    FloorManager();
    
    // Initialize with base seed
    void init(unsigned int baseSeed);
    
    // Get or generate floor (returns reference to cached floor)
    FloorData& get_floor(int floorNum);
    
    // Check if floor exists in cache
    bool has_floor(int floorNum) const;
    
    // Get current floor number
    int current_floor() const { return currentFloor_; }
    
    // Set current floor (for navigation)
    void set_current_floor(int floorNum);
    
    // Get current floor data (convenience)
    FloorData& current() { return get_floor(currentFloor_); }
    const FloorData& current() const { return floors_.at(currentFloor_); }
    
    // Move to next floor (descend)
    bool descend();
    
    // Move to previous floor (ascend)
    bool ascend();
    
    // Get player start position for current floor
    Position get_start_position() const;
    
    // Clear cache (keep only recent floors)
    void trim_cache(int maxFloors = 5);
    
    // Get total floors visited
    int floors_visited() const;
    
    // Serialize floor data for saving
    void save_floor(int floorNum, std::ostream& out) const;
    
    // Load floor data
    bool load_floor(int floorNum, std::istream& in);
    
private:
    // Generate a new floor
    void generate_floor(int floorNum);
    
    // Generate enemies for a floor based on depth
    void populate_enemies(FloorData& floor, int depth);
    
    std::unordered_map<int, FloorData> floors_;
    unsigned int baseSeed_ = 0;
    int currentFloor_ = 1;
    int maxFloor_ = 10;  // Victory at floor 10
};

// Global floor manager instance
namespace game {
    FloorManager& floors();
}


