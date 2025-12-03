#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "types.h"

struct LeaderboardEntry {
    std::string playerName;
    int floorsReached;
    int enemiesKilled;
    int goldCollected;
    std::string className;
    std::string causeOfDeath;
    time_t timestamp;
    unsigned int seed;
    
    // Default constructor
    LeaderboardEntry() 
        : floorsReached(0), enemiesKilled(0), goldCollected(0),
          timestamp(0), seed(0) {}
};

class Leaderboard {
public:
    static constexpr int MAX_ENTRIES = 10;
    static constexpr const char* LEADERBOARD_FILE = "saves/leaderboard.bin";
    
    // Add a new entry (automatically sorts and trims to top 10)
    void add_entry(const LeaderboardEntry& entry);
    
    // Get all entries (sorted by floors reached, then enemies killed)
    const std::vector<LeaderboardEntry>& get_entries() const { return entries_; }
    
    // Display leaderboard to screen
    void display(int startRow, int startCol, int width) const;
    
    // Load from file
    bool load();
    
    // Save to file
    bool save() const;
    
private:
    std::vector<LeaderboardEntry> entries_;
    void sort_entries();  // Sort by floors reached (desc), then enemies killed (desc)
};

