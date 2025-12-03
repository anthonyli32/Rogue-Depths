#include "database.h"
#include "logger.h"
#include "../lib/sqlite3.h"
#include <cstring>
#include <sstream>

// Global instance
static Database g_database;

namespace game {
    Database& db() {
        return g_database;
    }
}

Database::Database() {}

Database::~Database() {
    close();
}

bool Database::open(const std::string& path) {
    if (db_) {
        close();
    }
    
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        LOG_ERROR("Failed to open database: " + lastError_);
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    
    LOG_INFO("Database opened: " + path);
    return init_schema();
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        LOG_INFO("Database closed");
    }
}

bool Database::execute(const std::string& sql) {
    if (!db_) return false;
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        lastError_ = errMsg ? errMsg : "Unknown error";
        LOG_ERROR("SQL error: " + lastError_);
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool Database::query(const std::string& sql, 
                     std::function<void(int argc, char** argv, char** colNames)> callback) {
    if (!db_) return false;
    
    struct CallbackData {
        std::function<void(int, char**, char**)>* fn;
    };
    
    CallbackData data{&callback};
    
    auto sqliteCallback = [](void* userData, int argc, char** argv, char** colNames) -> int {
        auto* data = static_cast<CallbackData*>(userData);
        (*data->fn)(argc, argv, colNames);
        return 0;
    };
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), sqliteCallback, &data, &errMsg);
    
    if (rc != SQLITE_OK) {
        lastError_ = errMsg ? errMsg : "Unknown error";
        LOG_ERROR("SQL query error: " + lastError_);
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool Database::init_schema() {
    // Players table
    if (!execute(R"(
        CREATE TABLE IF NOT EXISTS players (
            save_slot INTEGER PRIMARY KEY,
            name TEXT,
            player_class INTEGER,
            hp INTEGER,
            max_hp INTEGER,
            attack INTEGER,
            defense INTEGER,
            speed INTEGER,
            floor INTEGER,
            seed INTEGER,
            gold INTEGER DEFAULT 0,
            inventory BLOB,
            equipment BLOB,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )")) return false;
    
    // Floors table
    if (!execute(R"(
        CREATE TABLE IF NOT EXISTS floors (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            save_slot INTEGER,
            floor_num INTEGER,
            dungeon_data BLOB,
            enemies_data BLOB,
            items_data BLOB,
            stairs_up_x INTEGER,
            stairs_up_y INTEGER,
            stairs_down_x INTEGER,
            stairs_down_y INTEGER,
            visited INTEGER DEFAULT 1,
            UNIQUE(save_slot, floor_num)
        )
    )")) return false;
    
    // Corpses table
    if (!execute(R"(
        CREATE TABLE IF NOT EXISTS corpses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            floor INTEGER,
            x INTEGER,
            y INTEGER,
            death_cause INTEGER,
            runs_since_death INTEGER DEFAULT 0,
            has_loot INTEGER DEFAULT 1,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )")) return false;
    
    // Config table
    if (!execute(R"(
        CREATE TABLE IF NOT EXISTS config (
            key TEXT PRIMARY KEY,
            value TEXT
        )
    )")) return false;
    
    // Stats table
    if (!execute(R"(
        CREATE TABLE IF NOT EXISTS stats (
            key TEXT PRIMARY KEY,
            value INTEGER
        )
    )")) return false;
    
    LOG_INFO("Database schema initialized");
    return true;
}

bool Database::save_player(int saveSlot, const Player& player, int floor, unsigned int seed) {
    std::stringstream ss;
    ss << "INSERT OR REPLACE INTO players "
       << "(save_slot, name, player_class, hp, max_hp, attack, defense, speed, floor, seed, updated_at) "
       << "VALUES ("
       << saveSlot << ", "
       << "'Player', "
       << static_cast<int>(player.player_class()) << ", "
       << player.get_stats().hp << ", "
       << player.get_stats().maxHp << ", "
       << player.get_stats().attack << ", "
       << player.get_stats().defense << ", "
       << player.get_stats().speed << ", "
       << floor << ", "
       << seed << ", "
       << "CURRENT_TIMESTAMP)";
    
    if (execute(ss.str())) {
        LOG_INFO("Saved player to slot " + std::to_string(saveSlot));
        return true;
    }
    return false;
}

bool Database::load_player(int saveSlot, Player& player, int& floor, unsigned int& seed) {
    bool found = false;
    
    std::stringstream ss;
    ss << "SELECT player_class, hp, max_hp, attack, defense, speed, floor, seed "
       << "FROM players WHERE save_slot = " << saveSlot;
    
    query(ss.str(), [&](int argc, char** argv, char**) {
        if (argc >= 8 && argv[0]) {
            PlayerClass pclass = static_cast<PlayerClass>(std::stoi(argv[0]));
            player = Player(pclass);
            
            auto& stats = player.get_stats();
            stats.hp = std::stoi(argv[1]);
            stats.maxHp = std::stoi(argv[2]);
            stats.attack = std::stoi(argv[3]);
            stats.defense = std::stoi(argv[4]);
            stats.speed = std::stoi(argv[5]);
            
            floor = std::stoi(argv[6]);
            seed = static_cast<unsigned int>(std::stoul(argv[7]));
            
            found = true;
        }
    });
    
    if (found) {
        LOG_INFO("Loaded player from slot " + std::to_string(saveSlot));
    }
    return found;
}

bool Database::delete_save(int saveSlot) {
    std::stringstream ss;
    ss << "DELETE FROM players WHERE save_slot = " << saveSlot;
    if (!execute(ss.str())) return false;
    
    ss.str("");
    ss << "DELETE FROM floors WHERE save_slot = " << saveSlot;
    execute(ss.str());
    
    LOG_INFO("Deleted save slot " + std::to_string(saveSlot));
    return true;
}

bool Database::has_save(int saveSlot) {
    bool exists = false;
    
    std::stringstream ss;
    ss << "SELECT 1 FROM players WHERE save_slot = " << saveSlot << " LIMIT 1";
    
    query(ss.str(), [&](int argc, char** argv, char**) {
        if (argc > 0 && argv[0]) {
            exists = true;
        }
    });
    
    return exists;
}

bool Database::save_floor(int saveSlot, int floorNum, const Dungeon& dungeon,
                          const std::vector<Enemy>& enemies) {
    auto dungeonData = serialize_dungeon(dungeon);
    auto enemiesData = serialize_enemies(enemies);
    
    // For simplicity, we'll store as hex strings (proper implementation would use prepared statements)
    std::stringstream dungeonHex, enemiesHex;
    for (uint8_t b : dungeonData) {
        dungeonHex << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    for (uint8_t b : enemiesData) {
        enemiesHex << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    
    std::stringstream ss;
    ss << "INSERT OR REPLACE INTO floors "
       << "(save_slot, floor_num, dungeon_data, enemies_data) "
       << "VALUES ("
       << saveSlot << ", "
       << floorNum << ", "
       << "X'" << dungeonHex.str() << "', "
       << "X'" << enemiesHex.str() << "')";
    
    return execute(ss.str());
}

bool Database::load_floor(int saveSlot, int floorNum, Dungeon& dungeon,
                          std::vector<Enemy>& enemies) {
    // Placeholder - would need proper blob handling
    (void)saveSlot;
    (void)floorNum;
    (void)dungeon;
    (void)enemies;
    return false;
}

bool Database::delete_floor(int saveSlot, int floorNum) {
    std::stringstream ss;
    ss << "DELETE FROM floors WHERE save_slot = " << saveSlot 
       << " AND floor_num = " << floorNum;
    return execute(ss.str());
}

bool Database::save_corpse(const CorpseData& corpse) {
    std::stringstream ss;
    ss << "INSERT INTO corpses (floor, x, y, death_cause, has_loot) VALUES ("
       << corpse.floor << ", "
       << corpse.position.x << ", "
       << corpse.position.y << ", "
       << static_cast<int>(corpse.cause) << ", "
       << (corpse.hasLoot ? 1 : 0) << ")";
    return execute(ss.str());
}

std::vector<CorpseData> Database::load_corpses() {
    std::vector<CorpseData> corpses;
    
    query("SELECT floor, x, y, death_cause, runs_since_death, has_loot FROM corpses ORDER BY id DESC LIMIT 10",
          [&](int argc, char** argv, char**) {
              if (argc >= 6 && argv[0]) {
                  CorpseData corpse;
                  corpse.floor = std::stoi(argv[0]);
                  corpse.position.x = std::stoi(argv[1]);
                  corpse.position.y = std::stoi(argv[2]);
                  corpse.cause = static_cast<DeathCause>(std::stoi(argv[3]));
                  corpse.runsSinceDeath = std::stoi(argv[4]);
                  corpse.hasLoot = std::stoi(argv[5]) != 0;
                  corpses.push_back(corpse);
              }
          });
    
    return corpses;
}

bool Database::age_corpses() {
    return execute("UPDATE corpses SET runs_since_death = runs_since_death + 1");
}

bool Database::delete_old_corpses(int maxAge) {
    std::stringstream ss;
    ss << "DELETE FROM corpses WHERE runs_since_death > " << maxAge;
    return execute(ss.str());
}

bool Database::save_config(const std::string& key, const std::string& value) {
    std::stringstream ss;
    ss << "INSERT OR REPLACE INTO config (key, value) VALUES ('"
       << key << "', '" << value << "')";
    return execute(ss.str());
}

std::string Database::load_config(const std::string& key, const std::string& defaultValue) {
    std::string result = defaultValue;
    
    std::stringstream ss;
    ss << "SELECT value FROM config WHERE key = '" << key << "'";
    
    query(ss.str(), [&](int argc, char** argv, char**) {
        if (argc > 0 && argv[0]) {
            result = argv[0];
        }
    });
    
    return result;
}

bool Database::save_stat(const std::string& key, int value) {
    std::stringstream ss;
    ss << "INSERT OR REPLACE INTO stats (key, value) VALUES ('"
       << key << "', " << value << ")";
    return execute(ss.str());
}

int Database::load_stat(const std::string& key, int defaultValue) {
    int result = defaultValue;
    
    std::stringstream ss;
    ss << "SELECT value FROM stats WHERE key = '" << key << "'";
    
    query(ss.str(), [&](int argc, char** argv, char**) {
        if (argc > 0 && argv[0]) {
            result = std::stoi(argv[0]);
        }
    });
    
    return result;
}

std::vector<uint8_t> Database::serialize_dungeon(const Dungeon& dungeon) {
    std::vector<uint8_t> data;
    
    // Header: width, height
    int w = dungeon.width();
    int h = dungeon.height();
    data.push_back(static_cast<uint8_t>(w & 0xFF));
    data.push_back(static_cast<uint8_t>((w >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>(h & 0xFF));
    data.push_back(static_cast<uint8_t>((h >> 8) & 0xFF));
    
    // Tiles
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            data.push_back(static_cast<uint8_t>(dungeon.get_tile(x, y)));
        }
    }
    
    return data;
}

bool Database::deserialize_dungeon(const std::vector<uint8_t>& data, Dungeon& dungeon) {
    if (data.size() < 4) return false;
    
    int w = data[0] | (data[1] << 8);
    int h = data[2] | (data[3] << 8);
    
    if (data.size() < 4 + static_cast<size_t>(w * h)) return false;
    
    dungeon = Dungeon(w, h);
    
    size_t idx = 4;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            dungeon.set_tile(x, y, static_cast<TileType>(data[idx++]));
        }
    }
    
    return true;
}

std::vector<uint8_t> Database::serialize_enemies(const std::vector<Enemy>& enemies) {
    std::vector<uint8_t> data;
    
    // Count
    size_t count = enemies.size();
    data.push_back(static_cast<uint8_t>(count & 0xFF));
    data.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));
    
    // Each enemy: type, x, y, hp
    for (const auto& enemy : enemies) {
        data.push_back(static_cast<uint8_t>(enemy.enemy_type()));
        auto pos = enemy.get_position();
        data.push_back(static_cast<uint8_t>(pos.x & 0xFF));
        data.push_back(static_cast<uint8_t>((pos.x >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(pos.y & 0xFF));
        data.push_back(static_cast<uint8_t>((pos.y >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(enemy.stats().hp & 0xFF));
        data.push_back(static_cast<uint8_t>((enemy.stats().hp >> 8) & 0xFF));
    }
    
    return data;
}

bool Database::deserialize_enemies(const std::vector<uint8_t>& data, std::vector<Enemy>& enemies) {
    if (data.size() < 2) return false;
    
    size_t count = data[0] | (data[1] << 8);
    size_t idx = 2;
    
    enemies.clear();
    for (size_t i = 0; i < count && idx + 7 <= data.size(); i++) {
        EnemyType type = static_cast<EnemyType>(data[idx++]);
        int x = data[idx] | (data[idx + 1] << 8); idx += 2;
        int y = data[idx] | (data[idx + 1] << 8); idx += 2;
        int hp = data[idx] | (data[idx + 1] << 8); idx += 2;
        
        Enemy enemy(type);
        enemy.set_position(x, y);
        enemy.stats().hp = hp;
        enemies.push_back(enemy);
    }
    
    return true;
}


