#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "types.h"
#include "player.h"
#include "enemy.h"
#include "dungeon.h"

// Forward declaration
struct sqlite3;

// Database wrapper for game persistence
class Database {
public:
    Database();
    ~Database();
    
    // Open database file (creates if doesn't exist)
    bool open(const std::string& path);
    
    // Close database
    void close();
    
    // Check if database is open
    bool is_open() const { return db_ != nullptr; }
    
    // Initialize schema (creates tables if needed)
    bool init_schema();
    
    // === PLAYER OPERATIONS ===
    bool save_player(int saveSlot, const Player& player, int floor, unsigned int seed);
    bool load_player(int saveSlot, Player& player, int& floor, unsigned int& seed);
    bool delete_save(int saveSlot);
    bool has_save(int saveSlot);
    
    // === FLOOR OPERATIONS ===
    bool save_floor(int saveSlot, int floorNum, const Dungeon& dungeon, 
                    const std::vector<Enemy>& enemies);
    bool load_floor(int saveSlot, int floorNum, Dungeon& dungeon, 
                    std::vector<Enemy>& enemies);
    bool delete_floor(int saveSlot, int floorNum);
    
    // === CORPSE OPERATIONS ===
    bool save_corpse(const CorpseData& corpse);
    std::vector<CorpseData> load_corpses();
    bool age_corpses();  // Increment runsSinceDeath
    bool delete_old_corpses(int maxAge = 10);
    
    // === CONFIG OPERATIONS ===
    bool save_config(const std::string& key, const std::string& value);
    std::string load_config(const std::string& key, const std::string& defaultValue = "");
    
    // === STATISTICS ===
    bool save_stat(const std::string& key, int value);
    int load_stat(const std::string& key, int defaultValue = 0);
    
    // Get last error message
    const std::string& last_error() const { return lastError_; }
    
private:
    // Execute SQL statement
    bool execute(const std::string& sql);
    
    // Execute SQL with callback for results
    bool query(const std::string& sql, 
               std::function<void(int argc, char** argv, char** colNames)> callback);
    
    // Serialize dungeon to blob
    std::vector<uint8_t> serialize_dungeon(const Dungeon& dungeon);
    bool deserialize_dungeon(const std::vector<uint8_t>& data, Dungeon& dungeon);
    
    // Serialize enemies to blob
    std::vector<uint8_t> serialize_enemies(const std::vector<Enemy>& enemies);
    bool deserialize_enemies(const std::vector<uint8_t>& data, std::vector<Enemy>& enemies);
    
    sqlite3* db_ = nullptr;
    std::string lastError_;
};

// Global database instance
namespace game {
    Database& db();
}


