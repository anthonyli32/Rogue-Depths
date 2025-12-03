#include "fileio.h"
#include "constants.h" // IMPROVED: Include for game_constants namespace

#include <fstream>
#include <filesystem>
#include <algorithm>

namespace {
    const char* kSavesDir = "saves";
    const uint32_t kMagic = 0x52444744; // 'RDGD'
    const uint32_t kVersion = 3; // v3: added player class
    
    // FIXED: Maximum string length to prevent memory exhaustion from corrupted files
    constexpr uint32_t kMaxStringLength = 1024 * 1024; // 1MB max string length

    template <typename T>
    void write_pod(std::ofstream& out, const T& v) {
        out.write(reinterpret_cast<const char*>(&v), sizeof(T));
    }
    template <typename T>
    bool read_pod(std::ifstream& in, T& v) {
        // FIXED: Check that read operation succeeded
        in.read(reinterpret_cast<char*>(&v), sizeof(T));
        return in.gcount() == sizeof(T);
    }

    void write_string(std::ofstream& out, const std::string& s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        write_pod(out, len);
        if (len) out.write(s.data(), len);
    }
    
    // FIXED: Add corruption protection - validate string length and verify read success
    std::string read_string(std::ifstream& in) {
        uint32_t len = 0;
        // FIXED: Check that length read succeeded
        if (!read_pod(in, len)) {
            return std::string(); // Return empty string on read failure
        }
        
        // FIXED: Validate string length to prevent memory exhaustion
        if (len > kMaxStringLength) {
            // Corrupted file - string length is unreasonably large
            return std::string(); // Return empty string to prevent crash
        }
        
        std::string s;
        if (len == 0) {
            return s; // Empty string
        }
        
        s.resize(len);
        // FIXED: Verify that we actually read the expected number of bytes
        in.read(&s[0], len);
        if (static_cast<size_t>(in.gcount()) != len) {
            // Read failed or incomplete - return empty string
            return std::string();
        }
        return s;
    }

    void write_item(std::ofstream& out, const Item& it) {
        write_string(out, it.name);
        write_pod(out, it.type);
        write_pod(out, it.rarity);
        write_pod(out, it.attackBonus);
        write_pod(out, it.defenseBonus);
        write_pod(out, it.hpBonus);
        write_pod(out, it.isEquippable);
        write_pod(out, it.isConsumable);
        write_pod(out, it.slot);
        write_pod(out, it.healAmount);
        write_pod(out, it.onUseStatus);
        write_pod(out, it.onUseMagnitude);
        write_pod(out, it.onUseDuration);
    }

    Item read_item(std::ifstream& in) {
        Item it;
        it.name = read_string(in);
        read_pod(in, it.type);
        read_pod(in, it.rarity);
        read_pod(in, it.attackBonus);
        read_pod(in, it.defenseBonus);
        read_pod(in, it.hpBonus);
        read_pod(in, it.isEquippable);
        read_pod(in, it.isConsumable);
        read_pod(in, it.slot);
        read_pod(in, it.healAmount);
        read_pod(in, it.onUseStatus);
        read_pod(in, it.onUseMagnitude);
        read_pod(in, it.onUseDuration);
        return it;
    }
}

namespace fileio {
    // Add file corruption protection: write checksum at end of file
    bool save_to_slot(const GameState& state, int slot) {
        try {
            if (slot < 1 || slot > 3) {
                return false;
            }
            std::filesystem::create_directories(kSavesDir);
            std::string path = std::string(kSavesDir) + "/slot" + std::to_string(slot) + ".bin";
            std::string tmpPath = path + ".tmp";
            std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
            if (!out) {
                return false;
            }
            std::vector<char> buffer;
            auto append = [&](const void* data, size_t size) {
                const char* c = reinterpret_cast<const char*>(data);
                buffer.insert(buffer.end(), c, c + size);
            };
            auto write_and_append = [&](auto v) {
                write_pod(out, v);
                append(&v, sizeof(v));
            };

            write_and_append(kMagic);
            write_and_append(kVersion);
            uint32_t diff = static_cast<uint32_t>(state.difficulty);
            write_and_append(diff);
            write_and_append(state.depth);
            write_and_append(state.seed);
            write_and_append(state.stairsDown.x);
            write_and_append(state.stairsDown.y);

            // Player basic
            const Position& ppos = state.player.get_position();
            write_and_append(ppos.x);
            write_and_append(ppos.y);
            const Stats& pst = state.player.get_stats();
            write_and_append(pst.maxHp);
            write_and_append(pst.hp);
            write_and_append(pst.attack);
            write_and_append(pst.defense);
            write_and_append(pst.speed);
            // Player class (v3+)
            uint32_t pclass = static_cast<uint32_t>(state.player.player_class());
            write_and_append(pclass);

            // Inventory
            const auto& inv = const_cast<Player&>(state.player).inventory();
            uint32_t invCount = static_cast<uint32_t>(inv.size());
            write_and_append(invCount);
            for (const auto& it : inv) {
                write_item(out, it);
                // Not included in checksum for simplicity
            }
            // Equipment
            const auto& eq = state.player.equipment();
            uint32_t eqCount = static_cast<uint32_t>(eq.size());
            write_and_append(eqCount);
            for (const auto& kv : eq) {
                write_and_append(kv.first);
                write_item(out, kv.second);
            }
            // Statuses
            const auto& sts = state.player.statuses();
            uint32_t stCount = static_cast<uint32_t>(sts.size());
            write_and_append(stCount);
            for (const auto& s : sts) {
                write_and_append(s.type);
                write_and_append(s.remainingTurns);
                write_and_append(s.magnitude);
            }

            // Enemies (minimal)
            uint32_t enemyCount = static_cast<uint32_t>(state.enemies.size());
            write_and_append(enemyCount);
            for (const auto& e : state.enemies) {
                const Position& epos = e.get_position();
                write_and_append(epos.x);
                write_and_append(epos.y);
                const Stats& est = e.stats();
                write_and_append(est.maxHp);
                write_and_append(est.hp);
                write_and_append(est.attack);
                write_and_append(est.defense);
                write_and_append(est.speed);
                uint32_t arch = static_cast<uint32_t>(e.archetype());
                write_and_append(arch);
            }
            // Compute checksum (simple sum)
            uint32_t checksum = 0;
            for (char c : buffer) checksum += static_cast<unsigned char>(c);
            write_pod(out, checksum);
            out.close();
            std::filesystem::rename(tmpPath, path);
            return true;
        } catch (...) {
            return false;
        }
    }

    // Add file corruption protection: verify checksum at end of file
    bool load_from_slot(GameState& outState, int slot) {
        try {
            if (slot < 1 || slot > 3) {
                return false;
            }
            std::string path = std::string(kSavesDir) + "/slot" + std::to_string(slot) + ".bin";
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                return false;
            }
            std::vector<char> buffer;
            // FIXED: Update read_and_append to check for read failures
            auto read_and_append = [&](auto& v) -> bool {
                if (!read_pod(in, v)) {
                    return false; // Read failed
                }
                const char* c = reinterpret_cast<const char*>(&v);
                buffer.insert(buffer.end(), c, c + sizeof(v));
                return true;
            };
            uint32_t magic = 0;
            uint32_t version = 0;
            // FIXED: Check that magic and version reads succeeded
            if (!read_pod(in, magic) || !read_pod(in, version)) {
                return false; // File too short or corrupted
            }
            if (magic != kMagic || (version != 1 && version != 2 && version != 3)) {
                return false;
            }
            // FIXED: Add error checking for all read operations
            uint32_t diff = 0;
            if (!read_and_append(diff)) return false;
            outState.difficulty = static_cast<Difficulty>(diff);
            if (!read_and_append(outState.depth)) return false;
            if (!read_and_append(outState.seed)) return false;
            if (!read_and_append(outState.stairsDown.x)) return false;
            if (!read_and_append(outState.stairsDown.y)) return false;

            // Player
            int px = 0, py = 0;
            if (!read_and_append(px)) return false;
            if (!read_and_append(py)) return false;
            outState.player.set_position(px, py);
            Stats pst{};
            if (!read_and_append(pst.maxHp)) return false;
            if (!read_and_append(pst.hp)) return false;
            if (!read_and_append(pst.attack)) return false;
            if (!read_and_append(pst.defense)) return false;
            if (!read_and_append(pst.speed)) return false;

            // Player class (v3+)
            PlayerClass pclass = PlayerClass::Warrior;
            if (version >= 3) {
                uint32_t pclassVal = 0;
                if (!read_and_append(pclassVal)) return false;
                pclass = static_cast<PlayerClass>(pclassVal);
            }

            std::vector<Item> inv;
            std::unordered_map<EquipmentSlot, Item> eq;
            std::vector<StatusEffect> sts;
            if (version >= 2) {
                uint32_t invCount = 0;
                if (!read_and_append(invCount)) return false;
                // FIXED: Validate inventory count to prevent excessive memory allocation
                if (invCount > 1000) return false; // Unreasonably large inventory
                inv.reserve(invCount);
                for (uint32_t i = 0; i < invCount; ++i) {
                    Item item = read_item(in);
                    if (item.name.empty() && invCount > 0) {
                        // Item read may have failed - skip it to prevent corruption
                        continue;
                    }
                    inv.push_back(item);
                }
                uint32_t eqCount = 0;
                if (!read_and_append(eqCount)) return false;
                // FIXED: Validate equipment count
                // IMPROVED: Use named constant for max equipment slots
                if (eqCount > game_constants::MAX_EQUIPMENT_SLOTS) return false; // Max equipment slots
                for (uint32_t i = 0; i < eqCount; ++i) {
                    EquipmentSlot slot{};
                    if (!read_and_append(slot)) return false;
                    Item it = read_item(in);
                    if (it.name.empty()) continue; // Skip invalid items
                    eq[slot] = it;
                }
                uint32_t stCount = 0;
                if (!read_and_append(stCount)) return false;
                // FIXED: Validate status count
                // IMPROVED: Use named constant for max status effects
                if (stCount > game_constants::MAX_STATUS_EFFECTS) return false; // Unreasonably many status effects
                sts.reserve(stCount);
                for (uint32_t i = 0; i < stCount; ++i) {
                    StatusEffect s{};
                    if (!read_and_append(s.type)) return false;
                    if (!read_and_append(s.remainingTurns)) return false;
                    if (!read_and_append(s.magnitude)) return false;
                    sts.push_back(s);
                }
                outState.player.load_from_persisted(pst, inv, eq, sts, pclass);
            } else {
                // v1 compatibility: no inventory/equipment/statuses
                outState.player.get_stats() = pst;
            }

            // Enemies
            uint32_t enemyCount = 0;
            if (!read_and_append(enemyCount)) return false;
            // FIXED: Validate enemy count to prevent excessive memory allocation
            // IMPROVED: Use named constant for max enemies
            if (enemyCount > game_constants::MAX_ENEMIES_PER_FLOOR) return false; // Unreasonably many enemies
            outState.enemies.clear();
            outState.enemies.reserve(enemyCount);
            for (uint32_t i = 0; i < enemyCount; ++i) {
                int ex = 0, ey = 0;
                Stats est{};
                uint32_t arch = 0;
                if (!read_and_append(ex)) return false;
                if (!read_and_append(ey)) return false;
                if (!read_and_append(est.maxHp)) return false;
                if (!read_and_append(est.hp)) return false;
                if (!read_and_append(est.attack)) return false;
                if (!read_and_append(est.defense)) return false;
                if (!read_and_append(est.speed)) return false;
                if (!read_and_append(arch)) return false;
                Enemy e(static_cast<EnemyArchetype>(arch), 'e', "\033[38;5;160m");
                e.set_position(ex, ey);
                e.stats() = est;
                outState.enemies.push_back(e);
            }
            // FIXED: Read and verify checksum with error checking
            uint32_t fileChecksum = 0;
            if (!read_pod(in, fileChecksum)) {
                return false; // Checksum read failed - file truncated
            }
            uint32_t calcChecksum = 0;
            for (char c : buffer) calcChecksum += static_cast<unsigned char>(c);
            if (fileChecksum != calcChecksum) {
                return false; // Corrupt file - checksum mismatch
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    bool delete_slot(int slot) {
        if (slot < 1 || slot > 3) {
            return false;
        }
        std::string path = std::string(kSavesDir) + "/slot" + std::to_string(slot) + ".bin";
        return std::filesystem::remove(path);
    }

    // Corpse management
    static const char* kCorpseFile = "saves/corpses.bin";
    static const uint32_t kCorpseMagic = 0x43525053; // 'CRPS'
    
    void save_corpse(const CorpseData& corpse) {
        // Load existing corpses
        auto corpses = load_corpses();
        
        // Add new corpse at front
        corpses.insert(corpses.begin(), corpse);
        
        // Keep only last 3
        if (corpses.size() > 3) {
            corpses.resize(3);
        }
        
        // Write all corpses
        std::filesystem::create_directories(kSavesDir);
        std::ofstream out(kCorpseFile, std::ios::binary | std::ios::trunc);
        if (!out) return;
        
        write_pod(out, kCorpseMagic);
        uint32_t count = static_cast<uint32_t>(corpses.size());
        write_pod(out, count);
        
        for (const auto& c : corpses) {
            write_pod(out, c.position.x);
            write_pod(out, c.position.y);
            write_pod(out, c.floor);
            write_pod(out, c.runsSinceDeath);
            write_pod(out, c.cause);
            write_pod(out, c.hasLoot);
        }
    }
    
    std::vector<CorpseData> load_corpses() {
        std::vector<CorpseData> corpses;
        std::ifstream in(kCorpseFile, std::ios::binary);
        if (!in) return corpses;
        
        uint32_t magic = 0;
        // FIXED: Check that magic read succeeded
        if (!read_pod(in, magic)) return corpses;
        if (magic != kCorpseMagic) return corpses;

        uint32_t count = 0;
        // FIXED: Check that count read succeeded
        if (!read_pod(in, count)) return corpses;
        // FIXED: Validate count to prevent excessive reads
        // IMPROVED: Use named constant for max corpses
        if (count > game_constants::MAX_CORPSES) return corpses; // Max corpses (safety limit)

        for (uint32_t i = 0; i < count && i < 3; ++i) {
            CorpseData c;
            // FIXED: Check each read operation
            if (!read_pod(in, c.position.x)) break;
            if (!read_pod(in, c.position.y)) break;
            if (!read_pod(in, c.floor)) break;
            if (!read_pod(in, c.runsSinceDeath)) break;
            if (!read_pod(in, c.cause)) break;
            if (!read_pod(in, c.hasLoot)) break;
            corpses.push_back(c);
        }
        return corpses;
    }
    
    void age_corpses() {
        auto corpses = load_corpses();
        for (auto& c : corpses) {
            c.runsSinceDeath++;
        }
        
        // Remove ancient corpses (6+ runs old)
        corpses.erase(
            std::remove_if(corpses.begin(), corpses.end(),
                [](const CorpseData& c) { return c.runsSinceDeath > 6; }),
            corpses.end()
        );
        
        // Re-save
        if (!corpses.empty()) {
            std::filesystem::create_directories(kSavesDir);
            std::ofstream out(kCorpseFile, std::ios::binary | std::ios::trunc);
            if (!out) return;
            
            write_pod(out, kCorpseMagic);
            uint32_t count = static_cast<uint32_t>(corpses.size());
            write_pod(out, count);
            
            for (const auto& c : corpses) {
                write_pod(out, c.position.x);
                write_pod(out, c.position.y);
                write_pod(out, c.floor);
                write_pod(out, c.runsSinceDeath);
                write_pod(out, c.cause);
                write_pod(out, c.hasLoot);
            }
        }
    }
}

