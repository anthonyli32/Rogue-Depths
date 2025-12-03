#pragma once

#include "player.h"
#include "enemy.h"
#include "dungeon.h"
#include "ui.h"
#include <set>
#include <map>
#include <string>
#include <vector>

namespace tutorial {
    // Tutorial state tracking
    struct TutorialState {
        int currentSection;  // 0-16 (17 sections total)
        std::set<std::string> completedObjectives;
        bool showTips;
        bool infiniteHealth;
        Player* player;
        Dungeon* dungeon;
        MessageLog* log;
        std::map<std::string, bool> sectionFlags;  // Track section-specific state
        
        // Movement tracking
        bool movedNorth;
        bool movedSouth;
        bool movedEast;
        bool movedWest;
        bool reachedMarker;
        int movementPhase;  // 0 = no movement yet, 1 = moved once (spawn X), 2 = reached X (spawn monster)
        
        // UI views tracking
        std::set<int> viewedScreens;  // Track which UI views were seen
        bool returnedToMap;
        
        // Inventory tracking
        bool inventoryOpened;
        int itemsNavigated;
        bool inventoryClosed;
        
        // Equipment tracking
        bool itemPickedUp;
        bool itemEquipped;
        bool equipmentViewed;
        
        // Dual wielding tracking
        bool firstWeaponEquipped;
        bool secondWeaponEquipped;
        
        // Combat tracking
        bool combatEntered;
        bool basicAttackUsed;
        bool weaponAttackUsed;
        bool cooldownObserved;
        bool cooldownReset;
        bool defendUsed;
        bool consumableUsed;
        bool enemyDefeated;
        
        // Loot tracking
        int itemsPickedUp;
        
        // Status effects tracking
        bool statusEffectReceived;
        
        // Hazards tracking
        bool trapTriggered;
        bool shrineInteracted;
        bool waterTraversed;
        
        // Stairs tracking
        bool standingOnStairs;
        bool stairsPressed;
        
        // Tip history for review functionality
        std::vector<std::vector<std::string>> tipHistory;  // Store last 10 tips per section
        int tipHistoryIndex;  // Current position in tip history
        bool reviewingTips;  // Whether currently reviewing tip history
        
        // Stuck detection for context-aware prompts
        std::chrono::steady_clock::time_point lastActionTime;
        bool promptDismissed;  // Whether current prompt was dismissed
    };
    
    // Run the combat tutorial mode (legacy, kept for compatibility)
    // Returns true if tutorial completed, false if player quit
    bool run_combat_tutorial();
    
    // Run the full tutorial level
    // Returns true if tutorial completed, false if player quit
    bool run_tutorial_level();
    
    // Side tip panel rendering
    void render_side_tips(int row, int col, const std::vector<std::string>& tips, 
                          const std::string& objective, int progressPercent,
                          const TutorialState& state);
    
    // Check tutorial objective completion
    bool check_objective(const std::string& objectiveId, TutorialState& state);
    
    // Generate custom tutorial dungeon (40x20 with 10 pre-designed rooms)
    Dungeon generate_tutorial_dungeon();
    
    // Tutorial section handlers
    bool section_movement(TutorialState& state);
    bool section_ui_views(TutorialState& state);
    bool section_inventory(TutorialState& state);
    bool section_equipment(TutorialState& state);
    bool section_dual_wielding(TutorialState& state);
    bool section_combat_basic(TutorialState& state);
    bool section_combat_weapons(TutorialState& state);
    bool section_combat_cooldowns(TutorialState& state);
    bool section_combat_defend(TutorialState& state);
    bool section_combat_consumables(TutorialState& state);
    bool section_combat_victory(TutorialState& state);
    bool section_loot(TutorialState& state);
    bool section_status_effects(TutorialState& state);
    bool section_hazards(TutorialState& state);
    bool section_stairs(TutorialState& state);
    
    // Helper functions
    void show_tutorial_completion_screen();
    int calculate_tutorial_progress(const TutorialState& state);
}

