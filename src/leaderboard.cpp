#include "leaderboard.h"
#include "ui.h"
#include "glyphs.h"
#include "constants.h"
#include "player.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <ctime>

void Leaderboard::add_entry(const LeaderboardEntry& entry) {
    entries_.push_back(entry);
    sort_entries();
    
    // Keep only top MAX_ENTRIES
    if (entries_.size() > MAX_ENTRIES) {
        entries_.resize(MAX_ENTRIES);
    }
    
    save();
}

void Leaderboard::sort_entries() {
    std::sort(entries_.begin(), entries_.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            // Sort by floors reached (descending), then enemies killed (descending)
            if (a.floorsReached != b.floorsReached) {
                return a.floorsReached > b.floorsReached;
            }
            return a.enemiesKilled > b.enemiesKilled;
        });
}

void Leaderboard::display(int startRow, int startCol, int width) const {
    using namespace ui;
    
    const int height = std::min(static_cast<int>(entries_.size()) + 4, 15);
    fill_rect(startRow, startCol, width, height);
    draw_box_double(startRow, startCol, width, height, constants::color_frame_main);
    
    // Title
    move_cursor(startRow, startCol + 2);
    set_color(constants::color_frame_main);
    std::cout << " " << glyphs::artifact() << " LEADERBOARD ";
    reset_color();
    
    int row = startRow + 1;
    
    if (entries_.empty()) {
        move_cursor(row++, startCol + 2);
        std::cout << "No entries yet. Complete a run to appear here!";
    } else {
        // Header
        move_cursor(row++, startCol + 2);
        std::cout << "Rank  Class      Floors  Kills  Gold    Date";
        
        // Entries
        for (size_t i = 0; i < entries_.size() && row < startRow + height - 1; ++i) {
            const auto& entry = entries_[i];
            move_cursor(row++, startCol + 2);
            
            // Rank
            std::cout << std::setw(2) << (i + 1) << ".  ";
            
            // Class name (truncated to 8 chars)
            std::string className = entry.className;
            if (className.length() > 8) className = className.substr(0, 8);
            std::cout << std::setw(8) << std::left << className << "  ";
            
            // Floors
            std::cout << std::setw(3) << std::right << entry.floorsReached << "   ";
            
            // Kills
            std::cout << std::setw(4) << std::right << entry.enemiesKilled << "  ";
            
            // Gold
            std::cout << std::setw(5) << std::right << entry.goldCollected << "  ";
            
            // Date (format: MM/DD)
            if (entry.timestamp > 0) {
                struct tm* timeinfo = localtime(&entry.timestamp);
                std::cout << std::setfill('0') << std::setw(2) << (timeinfo->tm_mon + 1) << "/"
                          << std::setw(2) << timeinfo->tm_mday << std::setfill(' ');
            } else {
                std::cout << "  --  ";
            }
        }
    }
}

bool Leaderboard::load() {
    entries_.clear();
    std::ifstream in(LEADERBOARD_FILE, std::ios::binary);
    if (!in) {
        return false;  // File doesn't exist yet, that's okay
    }
    
    // Read magic number
    uint32_t magic;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x4C424452) {  // "LBDR" in hex
        return false;  // Invalid file
    }
    
    // Read entry count
    uint32_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    count = std::min(count, static_cast<uint32_t>(MAX_ENTRIES));
    
    // Read entries
    for (uint32_t i = 0; i < count; ++i) {
        LeaderboardEntry entry;
        
        // Read string lengths and strings
        uint32_t nameLen, classLen, causeLen;
        in.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        entry.playerName.resize(nameLen);
        in.read(&entry.playerName[0], nameLen);
        
        in.read(reinterpret_cast<char*>(&classLen), sizeof(classLen));
        entry.className.resize(classLen);
        in.read(&entry.className[0], classLen);
        
        in.read(reinterpret_cast<char*>(&causeLen), sizeof(causeLen));
        entry.causeOfDeath.resize(causeLen);
        in.read(&entry.causeOfDeath[0], causeLen);
        
        // Read numeric fields
        in.read(reinterpret_cast<char*>(&entry.floorsReached), sizeof(entry.floorsReached));
        in.read(reinterpret_cast<char*>(&entry.enemiesKilled), sizeof(entry.enemiesKilled));
        in.read(reinterpret_cast<char*>(&entry.goldCollected), sizeof(entry.goldCollected));
        in.read(reinterpret_cast<char*>(&entry.timestamp), sizeof(entry.timestamp));
        in.read(reinterpret_cast<char*>(&entry.seed), sizeof(entry.seed));
        
        entries_.push_back(entry);
    }
    
    sort_entries();
    return true;
}

bool Leaderboard::save() const {
    std::ofstream out(LEADERBOARD_FILE, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    
    // Write magic number
    uint32_t magic = 0x4C424452;  // "LBDR"
    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    
    // Write entry count
    uint32_t count = static_cast<uint32_t>(entries_.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    // Write entries
    for (const auto& entry : entries_) {
        // Write string lengths and strings
        uint32_t nameLen = static_cast<uint32_t>(entry.playerName.length());
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        out.write(entry.playerName.c_str(), nameLen);
        
        uint32_t classLen = static_cast<uint32_t>(entry.className.length());
        out.write(reinterpret_cast<const char*>(&classLen), sizeof(classLen));
        out.write(entry.className.c_str(), classLen);
        
        uint32_t causeLen = static_cast<uint32_t>(entry.causeOfDeath.length());
        out.write(reinterpret_cast<const char*>(&causeLen), sizeof(causeLen));
        out.write(entry.causeOfDeath.c_str(), causeLen);
        
        // Write numeric fields
        out.write(reinterpret_cast<const char*>(&entry.floorsReached), sizeof(entry.floorsReached));
        out.write(reinterpret_cast<const char*>(&entry.enemiesKilled), sizeof(entry.enemiesKilled));
        out.write(reinterpret_cast<const char*>(&entry.goldCollected), sizeof(entry.goldCollected));
        out.write(reinterpret_cast<const char*>(&entry.timestamp), sizeof(entry.timestamp));
        out.write(reinterpret_cast<const char*>(&entry.seed), sizeof(entry.seed));
    }
    
    return true;
}

