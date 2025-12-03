#pragma once

#include <string>
#include <vector>
#include "types.h"
#include "entity.h"

class MessageLog;

enum class EnemyArchetype {
    Melee,
    Archer
};

// AI Learning Tier
enum class AITier {
    Basic,      // 0-2 observations: Default behavior
    Learning,   // 3-6 observations: Pattern recognition (brain icon)
    Adapted,    // 7-9 observations: Counter-tactics
    Master      // 10+ observations: Advanced tactics (lightning icon)
};

struct EnemyKnowledge {
    // Observation counters
    int timesPlayerKited = 0;
    int timesPlayerChoked = 0;
    int timesPlayerRangedSpam = 0;
    int timesPlayerMelee = 0;
    int timesPlayerFled = 0;
    
    // Total observations for tier calculation
    int totalObservations = 0;
    
    // Last 10 player actions (circular buffer)
    int actionHistory[10] = {0}; // 0=none, 1=melee, 2=ranged, 3=flee, 4=kite
    int historyIndex = 0;
    
    // Counter-tactic success tracking
    int counterSuccesses = 0;
    int counterAttempts = 0;
    
    // Calculated tier
    AITier tier = AITier::Basic;
    
    // Update tier based on observations
    void update_tier() {
        if (totalObservations >= 10) {
            tier = AITier::Master;
        } else if (totalObservations >= 7) {
            tier = AITier::Adapted;
        } else if (totalObservations >= 3) {
            tier = AITier::Learning;
        } else {
            tier = AITier::Basic;
        }
    }
    
    // Record a player action
    void record_action(int actionType) {
        actionHistory[historyIndex] = actionType;
        historyIndex = (historyIndex + 1) % 10;
        totalObservations++;
        update_tier();
    }
    
    // Get most common player tactic (for counter-play)
    int get_dominant_tactic() const {
        int counts[5] = {0};
        for (int i = 0; i < 10; ++i) {
            if (actionHistory[i] > 0 && actionHistory[i] < 5) {
                counts[actionHistory[i]]++;
            }
        }
        int maxIdx = 1;
        for (int i = 2; i < 5; ++i) {
            if (counts[i] > counts[maxIdx]) maxIdx = i;
        }
        return maxIdx;
    }
};

class Enemy {
public:
    // Legacy constructor (for backward compatibility)
    Enemy(EnemyArchetype archetype, char glyph, const std::string& color);
    // New constructor using EnemyType
    Enemy(EnemyType type, EnemyArchetype archetype = EnemyArchetype::Melee);

    const Position& get_position() const;
    void set_position(int x, int y);
    void move_by(int dx, int dy);

    EnemyArchetype archetype() const;
    EnemyType enemy_type() const;
    Stats& stats();
    const Stats& stats() const;
    EnemyKnowledge& knowledge();
    const EnemyKnowledge& knowledge() const;
    
    void apply_status(const StatusEffect& effect);
    void tick_statuses(MessageLog& log);
    bool has_status(StatusType type) const;
    const std::vector<StatusEffect>& statuses() const;

    char glyph() const;
    const std::string& color() const;
    const std::string& name() const;

    // Height level for flying enemies
    HeightLevel height() const;
    void set_height(HeightLevel h);
    bool is_grounded() const;
    void descend();  // Flying enemies occasionally land

    // Static helpers
    static char glyph_for_type(EnemyType type);
    static std::string color_for_type(EnemyType type);
    static std::string name_for_type(EnemyType type);
    static Stats base_stats_for_type(EnemyType type);
    static HeightLevel default_height_for_type(EnemyType type);

private:
    Position position_{};
    Stats stats_{};
    EnemyArchetype archetype_;
    EnemyType enemyType_ = EnemyType::Goblin;
    EnemyKnowledge knowledge_{};
    HeightLevel height_ = HeightLevel::Ground;
    char glyph_;
    std::string color_;
    std::string name_;
    std::vector<StatusEffect> statuses_{};
};


