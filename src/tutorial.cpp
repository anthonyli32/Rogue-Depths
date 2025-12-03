#include "tutorial.h"
#include "combat.h"
#include "input.h"
#include "glyphs.h"
#include "constants.h"
#include "logger.h"
#include "loot.h"
#include "types.h"
#include "viewport.h"
#include "globals.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <sstream>

namespace tutorial {
    
    // Calculate tutorial progress percentage
    int calculate_tutorial_progress(const TutorialState& state) {
        const int totalSections = 17;
        return (state.currentSection * 100) / totalSections;
    }
    
    // Calculate required width for tip panel based on content
    static int calculate_tip_width(const std::vector<std::string>& tips, 
                                   const std::string& objective, int progressPercent,
                                   const TutorialState& state) {
        int minWidth = 30;  // Minimum width
        int maxWidth = 50;  // Maximum width to prevent taking too much space
        
        // Check section info width
        std::string sectionInfo = "Section " + std::to_string(state.currentSection + 1) + "/17";
        int remainingSections = 17 - state.currentSection;
        int estimatedMinutes = remainingSections * 2;
        sectionInfo += " (~" + std::to_string(estimatedMinutes) + " min)";
        int requiredWidth = static_cast<int>(sectionInfo.length()) + 4;  // +4 for padding
        
        // Check tips width
        const std::vector<std::string>* tipsToShow = &tips;
        if (state.reviewingTips && !state.tipHistory.empty() && 
            state.currentSection < static_cast<int>(state.tipHistory.size()) &&
            !state.tipHistory[state.currentSection].empty()) {
            tipsToShow = &state.tipHistory[state.currentSection];
        }
        
        for (const auto& tipLine : *tipsToShow) {
            int lineWidth = static_cast<int>(tipLine.length()) + 4;  // +4 for padding
            if (lineWidth > requiredWidth) {
                requiredWidth = lineWidth;
            }
        }
        
        // Check objective width
        std::string objText = "[Objective: " + objective + "]";
        int objWidth = static_cast<int>(objText.length()) + 4;
        if (objWidth > requiredWidth) {
            requiredWidth = objWidth;
        }
        
        // Check progress bar width (always fits, but check anyway)
        std::string progressText = "Progress: ██████████ 100%";
        int progressWidth = static_cast<int>(progressText.length()) + 4;
        if (progressWidth > requiredWidth) {
            requiredWidth = progressWidth;
        }
        
        // Check controls width
        std::string controlsText = state.reviewingTips ? "[R] Back | [H] Hide" : "[R] Review | [H] Hide";
        int controlsWidth = static_cast<int>(controlsText.length()) + 4;
        if (controlsWidth > requiredWidth) {
            requiredWidth = controlsWidth;
        }
        
        // Clamp between min and max
        return std::max(minWidth, std::min(maxWidth, requiredWidth));
    }
    
    // Render side tip panel (right side of screen)
    void render_side_tips(int row, int col, const std::vector<std::string>& tips, 
                          const std::string& objective, int progressPercent, 
                          const TutorialState& state) {
        // Calculate dynamic width based on content
        int tipWidth = calculate_tip_width(tips, objective, progressPercent, state);
        const int tipHeight = 22;  // Increased height slightly
        
        // Draw tip panel box
        ui::draw_box_double(row, col, tipWidth, tipHeight, constants::color_frame_main);
        
        // Title with section number
        ui::move_cursor(row, col + 2);
        ui::set_color(constants::color_frame_main);
        std::cout << " 💡 TUTORIAL TIPS ";
        ui::reset_color();
        
        // Section number and time estimate
        ui::move_cursor(row + 1, col + 2);
        ui::set_color(constants::color_floor);
        std::string sectionInfo = "Section " + std::to_string(state.currentSection + 1) + "/17";
        int remainingSections = 17 - state.currentSection;
        int estimatedMinutes = remainingSections * 2;
        sectionInfo += " (~" + std::to_string(estimatedMinutes) + " min)";
        std::cout << sectionInfo;
        ui::reset_color();
        
        // Tips (max 8 lines to prevent overflow) - show history if reviewing
        int tipRow = row + 3;
        const std::vector<std::string>* tipsToShow = &tips;
        if (state.reviewingTips && !state.tipHistory.empty() && 
            state.currentSection < static_cast<int>(state.tipHistory.size()) &&
            !state.tipHistory[state.currentSection].empty()) {
            tipsToShow = &state.tipHistory[state.currentSection];
        }
        
        int maxTipLines = 8;  // Reduced from 10 to prevent overflow
        for (size_t i = 0; i < tipsToShow->size() && tipRow < row + tipHeight - 8 && i < static_cast<size_t>(maxTipLines); ++i) {
            std::string tipLine = (*tipsToShow)[i];
            ui::move_cursor(tipRow++, col + 2);
            std::cout << tipLine;
        }
        
        // Objective
        tipRow += 1;  // Spacing before objective
        ui::move_cursor(tipRow++, col + 2);
        ui::set_color(constants::color_msg_warning);
        std::string objText = "[Objective: " + objective + "]";
        std::cout << objText;
        ui::reset_color();
        
        // Progress bar
        tipRow += 1;  // Spacing before progress
        ui::move_cursor(tipRow++, col + 2);
        std::string progressText = "Progress: ";
        int filled = (progressPercent / 10);
        for (int i = 0; i < 10; ++i) {
            progressText += (i < filled ? "█" : "░");
        }
        progressText += " " + std::to_string(progressPercent) + "%";
        std::cout << progressText;
        
        // Controls hint
        tipRow += 1;  // Spacing before controls
        ui::move_cursor(tipRow, col + 2);
        ui::set_color(constants::color_floor);
        std::string controlsText = state.reviewingTips ? "[R] Back | [H] Hide" : "[R] Review | [H] Hide";
        std::cout << controlsText;
        ui::reset_color();
    }
    
    // Check tutorial objective completion
    bool check_objective(const std::string& objectiveId, TutorialState& state) {
        return state.completedObjectives.find(objectiveId) != state.completedObjectives.end();
    }
    
    // Generate custom tutorial dungeon (50x20 with 10 pre-designed rooms - expanded for larger rooms)
    Dungeon generate_tutorial_dungeon() {
        Dungeon dungeon(50, 20);
        
        // Fill with walls first
        for (int y = 0; y < 20; ++y) {
            for (int x = 0; x < 50; ++x) {
                dungeon.set_tile(x, y, TileType::Wall);
            }
        }
        
        // Room 1: Movement practice (6x6) at (2, 2) - made bigger for breathing room
        // Clear Room 1 completely - no items, just floor tiles
        for (int y = 2; y < 8; ++y) {
            for (int x = 2; x < 8; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Room 1 is empty - player spawns at center, learns movement, then moves to Room 2
        
        // Corridor from Room 1 to Room 2 - made longer for spacing
        for (int x = 8; x < 14; ++x) {
            dungeon.set_tile(x, 4, TileType::Floor);
        }
        
        // Room 2: UI views (4x4) at (14, 2) - made bigger
        for (int y = 2; y < 6; ++y) {
            for (int x = 14; x < 18; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        
        // Corridor to Room 3 - made longer
        for (int x = 18; x < 23; ++x) {
            dungeon.set_tile(x, 3, TileType::Floor);
        }
        
        // Room 3: Inventory basics (6x5) at (23, 1) - made bigger and repositioned
        for (int y = 1; y < 6; ++y) {
            for (int x = 23; x < 29; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Pre-place items: weapon at (25, 2), armor at (26, 2), consumable at (27, 2)
        
        // Corridor to Room 4 - made longer
        for (int x = 29; x < 33; ++x) {
            dungeon.set_tile(x, 3, TileType::Floor);
        }
        
        // Room 4: Equipment (5x5) at (33, 1) - made bigger
        for (int y = 1; y < 6; ++y) {
            for (int x = 33; x < 38; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Weapon at (35, 2)
        
        // Corridor to Room 5 - made longer
        for (int x = 38; x < 42; ++x) {
            dungeon.set_tile(x, 3, TileType::Floor);
        }
        
        // Room 5: Dual wielding (5x5) at (42, 1) - made bigger
        for (int y = 1; y < 6; ++y) {
            for (int x = 42; x < 47; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Two weapons at (43, 2) and (45, 2)
        
        // Corridor down to Room 6 - made longer
        for (int y = 6; y < 12; ++y) {
            dungeon.set_tile(44, y, TileType::Floor);
        }
        
        // Room 6: Combat arena (10x10) at (35, 12) - made bigger and repositioned
        for (int y = 12; y < 22; ++y) {
            for (int x = 35; x < 45; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Enemy rat at (39, 17)
        
        // Corridor to Room 7
        for (int x = 22; x < 30; ++x) {
            dungeon.set_tile(x, 14, TileType::Floor);
        }
        
        // Room 7: Loot collection (5x5) at (17, 12)
        for (int y = 12; y < 17; ++y) {
            for (int x = 17; x < 22; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Corpse at (19, 14) with items
        
        // Corridor to Room 8
        for (int x = 10; x < 17; ++x) {
            dungeon.set_tile(x, 14, TileType::Floor);
        }
        
        // Room 8: Status effects (5x5) at (5, 12)
        for (int y = 12; y < 17; ++y) {
            for (int x = 5; x < 10; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Spider at (7, 14)
        
        // Corridor to Room 9 (from Room 1 area, going down)
        // Connect from Room 1 area (around x=4, y=7) down to Room 9
        for (int y = 8; y < 10; ++y) {
            dungeon.set_tile(4, y, TileType::Floor);  // Vertical corridor from Room 1 area
        }
        for (int x = 2; x < 8; ++x) {
            dungeon.set_tile(x, 10, TileType::Floor);  // Horizontal connection to Room 9
        }
        
        // Room 9: Hazards (6x6) - moved to avoid overlap with Room 1
        // Room 9 is now at (2, 11) to (7, 16) to avoid Room 1 (2,2 to 7,7)
        for (int y = 11; y < 17; ++y) {
            for (int x = 2; x < 8; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Trap at (4, 13), Shrine at (5, 13), Water at (6, 13) - updated positions
        dungeon.set_tile(4, 13, TileType::Trap);
        dungeon.set_tile(5, 13, TileType::Shrine);
        dungeon.set_tile(6, 13, TileType::Water);
        
        // Corridor to Room 10 (from Room 9 area)
        for (int x = 8; x < 10; ++x) {
            dungeon.set_tile(x, 13, TileType::Floor);  // Updated to connect from Room 9
        }
        
        // Room 10: Stairs (3x3) at (10, 6)
        for (int y = 6; y < 9; ++y) {
            for (int x = 10; x < 13; ++x) {
                dungeon.set_tile(x, y, TileType::Floor);
            }
        }
        // Stairs down at (11, 7)
        dungeon.set_tile(11, 7, TileType::StairsDown);
        
        return dungeon;
    }
    
    // Draw tutorial marker (X for objectives)
    static void draw_tutorial_marker(int x, int y, bool pulsing = false) {
        static auto lastPulse = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPulse);
        
        if (elapsed.count() > 500) {
            lastPulse = now;
        }
        
        bool bright = (elapsed.count() % 1000) < 500;
        ui::move_cursor(y, x);
        if (bright && pulsing) {
            ui::set_color(constants::ansi_bold);
            ui::set_color(constants::color_msg_warning);
        } else {
            ui::set_color(constants::color_msg_info);
        }
        std::cout << "X";
        ui::reset_color();
    }
    
    // Show enhanced success animation (multi-frame: sparkles → checkmark → fade)
    static void show_success_animation(int row, int col) {
        // Frame 1: Sparkles
        ui::move_cursor(row, col);
        ui::set_color(constants::color_msg_heal);
        std::cout << glyphs::sparkle() << " " << glyphs::sparkle() << " " << glyphs::sparkle();
        ui::reset_color();
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Frame 2: Checkmark with sparkles
        ui::move_cursor(row, col);
        ui::set_color(constants::color_msg_heal);
        std::cout << glyphs::sparkle() << " " << glyphs::checkmark() << " Success! " << glyphs::checkmark() << " " << glyphs::sparkle();
        ui::reset_color();
        std::cout.flush();
        ui::play_victory_sound();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Frame 3: Fade out
        for (int i = 0; i < 3; ++i) {
            ui::move_cursor(row, col);
            ui::set_color(constants::color_floor);
            std::cout << glyphs::checkmark() << " Success! ";
            ui::reset_color();
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // Clear the animation
        ui::move_cursor(row, col);
        for (int i = 0; i < 20; ++i) std::cout << " ";
        std::cout.flush();
    }
    
    // Show section completion celebration
    static void show_section_completion_celebration(int row, int col) {
        // Multiple sparkles animation
        for (int frame = 0; frame < 5; ++frame) {
            ui::move_cursor(row, col);
            ui::set_color(constants::color_msg_heal);
            for (int i = 0; i < 10; ++i) {
                if ((i + frame) % 3 == 0) {
                    std::cout << glyphs::sparkle() << " ";
                } else {
                    std::cout << "  ";
                }
            }
            ui::reset_color();
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
        // Clear
        ui::move_cursor(row, col);
        for (int i = 0; i < 20; ++i) std::cout << " ";
        std::cout.flush();
    }
    
    // Helper function for section completion transition
    static void complete_section_transition(TutorialState& state, const std::string& message, 
                                           int nextX, int nextY) {
        auto termSize = input::get_terminal_size();
        int mapStartRow = 1;
        int mapStartCol = 1;
        int viewport_h = std::min(20, termSize.height - 5);
        
        show_success_animation(mapStartRow + viewport_h + 2, mapStartCol);
        show_section_completion_celebration(mapStartRow + viewport_h + 3, mapStartCol);
        
        // Show "Section Complete!" message
        int msgRow = termSize.height / 2;
        int msgCol = termSize.width / 2 - 10;
        ui::move_cursor(msgRow, msgCol);
        ui::set_color(constants::color_msg_heal);
        std::cout << glyphs::checkmark() << " Section Complete! " << glyphs::checkmark();
        ui::reset_color();
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));  // Increased from 1000
        
        state.log->add(MessageType::Info, message);
        
        // Add a brief pause before moving player
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        state.player->set_position(nextX, nextY);
        ui::fade_transition(5);  // Longer fade (was 3)
    }
    
    // Unified section completion function - handles all section completions consistently
    static void complete_tutorial_section(TutorialState& state, int& lastSection, 
                                         const std::string& completionMessage,
                                         const std::string& nextSectionMessage,
                                         int nextX, int nextY,
                                         const std::string& nextSectionPrompt) {
        // 1. Clear screen
        ui::clear();
        std::cout.flush();
        
        // 2. Show completion animation
        auto termSize = input::get_terminal_size();
        int msgRow = termSize.height / 2;
        int msgCol = termSize.width / 2 - 10;
        ui::move_cursor(msgRow, msgCol);
        ui::set_color(constants::color_msg_heal);
        std::cout << glyphs::checkmark() << " Section Complete! " << glyphs::checkmark();
        ui::reset_color();
        std::cout.flush();
        
        // 3. Add completion message
        state.log->add(MessageType::Info, completionMessage);
        
        // 4. Wait for user acknowledgment
        ui::move_cursor(msgRow + 2, msgCol - 5);
        ui::set_color(constants::color_floor);
        std::cout << "Press any key to continue...";
        ui::reset_color();
        std::cout.flush();
        input::read_key_blocking();  // Wait for any key
        
        // 5. Increment section
        state.currentSection++;
        lastSection = state.currentSection;
        
        // 6. Add next section message
        state.log->add(MessageType::Info, nextSectionMessage);
        
        // 7. Move player
        state.player->set_position(nextX, nextY);
        
        // 8. Show transition
        ui::fade_transition(5);
        
        // 9. Show guided prompt (if provided) - will be shown on next frame
        // Note: show_guided_prompt is called from main loop, so we set a flag
        // For now, we'll just add the message and let the main loop handle the prompt
    }
    
    // Show skip confirmation dialog
    static bool show_skip_confirmation() {
        auto termSize = input::get_terminal_size();
        int dialogRow = termSize.height / 2 - 2;
        int dialogCol = termSize.width / 2 - 20;
        
        ui::draw_box_double(dialogRow, dialogCol, 40, 8, constants::color_msg_warning);
        ui::move_cursor(dialogRow, dialogCol + 2);
        ui::set_color(constants::color_msg_warning);
        std::cout << " ⚠ SKIP TUTORIAL? ";
        ui::reset_color();
        
        ui::move_cursor(dialogRow + 2, dialogCol + 4);
        std::cout << "Progress will be lost.";
        ui::move_cursor(dialogRow + 3, dialogCol + 4);
        std::cout << "Are you sure?";
        ui::move_cursor(dialogRow + 5, dialogCol + 6);
        ui::set_color(constants::color_msg_info);
        std::cout << "[Y] Yes  [N] No";
        ui::reset_color();
        std::cout.flush();
        
        while (true) {
            int key = input::read_key_blocking();
            if (key == 'y' || key == 'Y') {
                return true;
            } else if (key == 'n' || key == 'N' || key == 27) {
                return false;
            }
        }
    }
    
    // Draw indicator above element (! for items, ? for interactables)
    static void draw_element_indicator(int row, int col, const std::string& indicator) {
        ui::move_cursor(row, col);
        ui::set_color(constants::color_msg_warning);
        std::cout << indicator;
        ui::reset_color();
    }
    
    // Draw tutorial-specific overlays (X markers, item highlighting, indicators)
    // This is called after draw_map_viewport to add tutorial elements
    static void draw_tutorial_overlays(const TutorialState& state, 
                                      const std::vector<Item>& room3Items,
                                      const std::vector<Position>& room3ItemPositions,
                                      const std::vector<Position>& room4ItemPositions,
                                      const std::vector<Position>& room5ItemPositions,
                                      const std::vector<Item>& room7Items,
                                      const std::vector<Position>& room7ItemPositions,
                                      const std::vector<Enemy>& enemies,
                                      int mapStartRow, int mapStartCol,
                                      int viewport_w, int viewport_h,
                                      int cam_x, int cam_y) {
        Position playerPos = state.player->get_position();
        
        // Draw tutorial elements on top of the viewport
        for (int vy = 0; vy < viewport_h; ++vy) {
            for (int vx = 0; vx < viewport_w; ++vx) {
                int mapX = cam_x + vx;
                int mapY = cam_y + vy;
                
                // Check if this position is in viewport bounds (tutorial dungeon is 50x20)
                if (mapX < 0 || mapX >= 50 || mapY < 0 || mapY >= 20) continue;
                
                // Draw items on ground with highlighting
                // Only draw items if they're in FOV and in the appropriate section
                // Room 3 items (sections 2-3: Inventory)
                if (state.currentSection >= 2 && state.currentSection <= 3) {
                    for (size_t i = 0; i < room3ItemPositions.size(); ++i) {
                        if (room3ItemPositions[i].x == mapX && room3ItemPositions[i].y == mapY) {
                            // Check FOV before drawing
                            Position itemPos{mapX, mapY};
                            if (!in_simple_fov(playerPos, itemPos.x, itemPos.y, constants::fov_radius)) continue;
                            
                            ui::move_cursor(mapStartRow + 1 + vy, mapStartCol + 1 + vx);
                            // Check if player is nearby (within 2 tiles) for highlighting
                            int distX = std::abs(playerPos.x - mapX);
                            int distY = std::abs(playerPos.y - mapY);
                            bool isNearby = (distX + distY) <= 2;
                            bool shouldHighlight = (state.currentSection == 2 || state.currentSection == 3) && isNearby;
                            
                            if (shouldHighlight) {
                                ui::set_color(constants::ansi_bold);
                            }
                            if (room3Items[i].type == ItemType::Weapon) {
                                std::cout << glyphs::weapon();
                            } else if (room3Items[i].type == ItemType::Armor) {
                                std::cout << glyphs::armor();
                            } else if (room3Items[i].type == ItemType::Consumable) {
                                std::cout << glyphs::potion();
                            }
                            ui::reset_color();
                            
                            // Draw indicator above item
                            if (shouldHighlight && state.currentSection == 3) {
                                draw_element_indicator(mapStartRow + vy, mapStartCol + 1 + vx, "!");
                            }
                        }
                    }
                }
                
                // Room 4 items (section 3: Equipment)
                if (state.currentSection == 3) {
                    for (size_t i = 0; i < room4ItemPositions.size(); ++i) {
                        if (room4ItemPositions[i].x == mapX && room4ItemPositions[i].y == mapY) {
                            // Check FOV before drawing
                            Position itemPos{mapX, mapY};
                            if (!in_simple_fov(playerPos, itemPos.x, itemPos.y, constants::fov_radius)) continue;
                            
                            ui::move_cursor(mapStartRow + 1 + vy, mapStartCol + 1 + vx);
                            int distX = std::abs(playerPos.x - mapX);
                            int distY = std::abs(playerPos.y - mapY);
                            bool isNearby = (distX + distY) <= 2;
                            bool shouldHighlight = state.currentSection == 3 && isNearby;
                            if (shouldHighlight) {
                                ui::set_color(constants::ansi_bold);
                                draw_element_indicator(mapStartRow + vy, mapStartCol + 1 + vx, "!");
                            }
                            std::cout << glyphs::weapon();
                            ui::reset_color();
                        }
                    }
                }
                
                // Room 5 items (section 4: Dual Wielding)
                if (state.currentSection == 4) {
                    for (size_t i = 0; i < room5ItemPositions.size(); ++i) {
                        if (room5ItemPositions[i].x == mapX && room5ItemPositions[i].y == mapY) {
                            // Check FOV before drawing
                            Position itemPos{mapX, mapY};
                            if (!in_simple_fov(playerPos, itemPos.x, itemPos.y, constants::fov_radius)) continue;
                            
                            ui::move_cursor(mapStartRow + 1 + vy, mapStartCol + 1 + vx);
                            int distX = std::abs(playerPos.x - mapX);
                            int distY = std::abs(playerPos.y - mapY);
                            bool isNearby = (distX + distY) <= 2;
                            bool shouldHighlight = state.currentSection == 4 && isNearby;
                            if (shouldHighlight) {
                                ui::set_color(constants::ansi_bold);
                                draw_element_indicator(mapStartRow + vy, mapStartCol + 1 + vx, "!");
                            }
                            std::cout << glyphs::weapon();
                            ui::reset_color();
                        }
                    }
                }
                
                // Room 7 items (loot) - section 11: Loot Collection
                if (state.currentSection == 11) {
                    for (size_t i = 0; i < room7ItemPositions.size(); ++i) {
                        if (room7ItemPositions[i].x == mapX && room7ItemPositions[i].y == mapY) {
                            // Check FOV before drawing
                            Position itemPos{mapX, mapY};
                            if (!in_simple_fov(playerPos, itemPos.x, itemPos.y, constants::fov_radius)) continue;
                            
                            ui::move_cursor(mapStartRow + 1 + vy, mapStartCol + 1 + vx);
                            if (room7Items[i].type == ItemType::Weapon) {
                                std::cout << glyphs::weapon();
                            } else if (room7Items[i].type == ItemType::Armor) {
                                std::cout << glyphs::armor();
                            } else if (room7Items[i].type == ItemType::Consumable) {
                                std::cout << glyphs::potion();
                            }
                        }
                    }
                }
                
                // Draw tutorial enemies with pulsing effect
                for (const auto& enemy : enemies) {
                    Position enemyPos = enemy.get_position();
                    if (enemyPos.x == mapX && enemyPos.y == mapY) {
                        // Only draw enemy if we're in the right section/phase
                        bool shouldDraw = false;
                        if (enemy.enemy_type() == EnemyType::Rat) {
                            // Show rat in movement section phase 2, or in combat sections 5-10
                            if (state.currentSection == 0 && state.movementPhase >= 2) {
                                shouldDraw = true;
                            } else if (state.currentSection >= 5 && state.currentSection <= 10) {
                                shouldDraw = true;
                            }
                        } else if (enemy.enemy_type() == EnemyType::Spider && 
                                   state.currentSection == 12) {
                            shouldDraw = true;
                        }
                        
                        if (shouldDraw) {
                            ui::move_cursor(mapStartRow + 1 + vy, mapStartCol + 1 + vx);
                            // Pulsing effect for enemies
                            static int pulseFrame = 0;
                            pulseFrame++;
                            bool pulsing = (pulseFrame / 10) % 2 == 0;
                            if (pulsing) {
                                ui::set_color(constants::ansi_bold + constants::color_monster_weak);
                            } else {
                                ui::set_color(constants::color_monster_weak);
                            }
                            std::cout << enemy.glyph();
                            ui::reset_color();
                        }
                    }
                }
                
                // Draw shrine indicator
                if (state.dungeon->get_tile(mapX, mapY) == TileType::Shrine && state.currentSection == 13) {
                    int distX = std::abs(playerPos.x - mapX);
                    int distY = std::abs(playerPos.y - mapY);
                    if ((distX + distY) <= 2) {
                        draw_element_indicator(mapStartRow + vy, mapStartCol + 1 + vx, "?");
                    }
                }
            }
        }
    }
    
    // Show guided prompt at bottom of screen (dismissible with Space)
    static void show_guided_prompt(const std::string& prompt, TutorialState& state, bool forceShow = false) {
        // Check if player is stuck (no action for 5 seconds) or force show
        auto now = std::chrono::steady_clock::now();
        auto timeSinceAction = std::chrono::duration_cast<std::chrono::seconds>(now - state.lastActionTime).count();
        
        if (!forceShow && (timeSinceAction < 5 || state.promptDismissed)) {
            return;  // Don't show if not stuck or already dismissed
        }
        
        auto termSize = input::get_terminal_size();
        int promptRow = termSize.height - 2;
        int promptCol = 2;
        
        // Clear previous prompt
        ui::move_cursor(promptRow, promptCol);
        for (int i = 0; i < termSize.width - 4; ++i) {
            std::cout << " ";
        }
        
        // Draw new prompt with dismiss hint
        ui::move_cursor(promptRow, promptCol);
        ui::set_color(constants::color_msg_info);
        std::cout << glyphs::arrow_right() << " " << prompt;
        ui::move_cursor(promptRow + 1, promptCol);
        ui::set_color(constants::color_floor);
        std::cout << "[Space] Dismiss";
        ui::reset_color();
        state.promptDismissed = false;  // Reset dismiss flag when showing new prompt
    }
    
    // Helper to store tips in history when section changes
    static void store_tip_history(TutorialState& state, const std::vector<std::string>& tips) {
        // Ensure tipHistory has enough space
        if (state.currentSection >= static_cast<int>(state.tipHistory.size())) {
            state.tipHistory.resize(state.currentSection + 1);
        }
        
        // Store tips for current section (keep last 10)
        state.tipHistory[state.currentSection] = tips;
        if (state.tipHistory[state.currentSection].size() > 10) {
            state.tipHistory[state.currentSection].resize(10);
        }
    }
    
    // Section 1: Movement
    bool section_movement(TutorialState& state) {
        // Section is complete when:
        // - Player has moved in all 4 directions (phase 2 - monster spawned)
        // - Player has encountered the monster (combat entered)
        // Note: Combat encounter advances section directly in the handler,
        // so this function is mainly for tracking progress
        
        // Section completes when combat is entered (handled in movement handler)
        // This function just tracks that we've reached phase 2 (monster spawned)
        return state.movementPhase >= 2;  // Monster has been spawned
    }
    
    // Section 2: UI Views
    bool section_ui_views(TutorialState& state) {
        // This section is handled in the main tutorial loop
        // Check if all 5 views were seen
        if (state.viewedScreens.size() >= 5 && state.returnedToMap) {
            if (!check_objective("section_ui_views_complete", state)) {
                state.completedObjectives.insert("section_ui_views_complete");
                return true;
            }
        }
        return false;
    }
    
    // Section 3: Inventory
    bool section_inventory(TutorialState& state) {
        if (state.inventoryOpened && state.itemsNavigated >= 2 && state.inventoryClosed) {
            if (!check_objective("section_inventory_complete", state)) {
                state.completedObjectives.insert("section_inventory_complete");
                return true;
            }
        }
        return false;
    }
    
    // Section 4: Equipment
    bool section_equipment(TutorialState& state) {
        if (state.itemPickedUp && state.itemEquipped && state.equipmentViewed) {
            if (!check_objective("section_equipment_complete", state)) {
                state.completedObjectives.insert("section_equipment_complete");
                return true;
            }
        }
        return false;
    }
    
    // Section 5: Dual Wielding
    bool section_dual_wielding(TutorialState& state) {
        if (state.firstWeaponEquipped && state.secondWeaponEquipped) {
            if (!check_objective("section_dual_wielding_complete", state)) {
                state.completedObjectives.insert("section_dual_wielding_complete");
                return true;
            }
        }
        return false;
    }
    
    // Section 6-12: Combat sections (handled in main loop)
    bool section_combat_basic(TutorialState& state) {
        if (state.combatEntered && state.basicAttackUsed) {
            if (!check_objective("section_combat_basic_complete", state)) {
                state.completedObjectives.insert("section_combat_basic_complete");
                return true;
            }
        }
        return false;
    }
    
    bool section_combat_weapons(TutorialState& state) {
        if (state.weaponAttackUsed) {
            if (!check_objective("section_combat_weapons_complete", state)) {
                state.completedObjectives.insert("section_combat_weapons_complete");
                return true;
            }
        }
        return false;
    }
    
    bool section_combat_cooldowns(TutorialState& state) {
        if (state.cooldownObserved && state.cooldownReset) {
            if (!check_objective("section_combat_cooldowns_complete", state)) {
                state.completedObjectives.insert("section_combat_cooldowns_complete");
                return true;
            }
        }
        return false;
    }
    
    bool section_combat_defend(TutorialState& state) {
        if (state.defendUsed) {
            if (!check_objective("section_combat_defend_complete", state)) {
                state.completedObjectives.insert("section_combat_defend_complete");
                return true;
            }
        }
        return false;
    }
    
    bool section_combat_consumables(TutorialState& state) {
        if (state.consumableUsed) {
            if (!check_objective("section_combat_consumables_complete", state)) {
                state.completedObjectives.insert("section_combat_consumables_complete");
                return true;
            }
        }
        return false;
    }
    
    bool section_combat_victory(TutorialState& state) {
        if (state.enemyDefeated) {
            if (!check_objective("section_combat_victory_complete", state)) {
                state.completedObjectives.insert("section_combat_victory_complete");
                return true;
            }
        }
        return false;
    }
    
    // Section 13: Loot
    bool section_loot(TutorialState& state) {
        if (state.itemsPickedUp >= 2) {
            if (!check_objective("section_loot_complete", state)) {
                state.completedObjectives.insert("section_loot_complete");
                return true;
            }
        }
        return false;
    }
    
    // Section 14: Status Effects
    bool section_status_effects(TutorialState& state) {
        if (state.statusEffectReceived) {
            if (!check_objective("section_status_effects_complete", state)) {
                state.completedObjectives.insert("section_status_effects_complete");
                return true;
            }
        }
        return false;
    }
    
    // Section 15: Hazards
    bool section_hazards(TutorialState& state) {
        if (state.trapTriggered && state.shrineInteracted && state.waterTraversed) {
            if (!check_objective("section_hazards_complete", state)) {
                state.completedObjectives.insert("section_hazards_complete");
                return true;
            }
        }
        return false;
    }
    
    // Section 16: Stairs
    bool section_stairs(TutorialState& state) {
        if (state.standingOnStairs && state.stairsPressed) {
            if (!check_objective("section_stairs_complete", state)) {
                state.completedObjectives.insert("section_stairs_complete");
                return true;
            }
        }
        return false;
    }
    
    // Show tutorial completion screen
    void show_tutorial_completion_screen() {
        ui::clear();
        
        auto termSize = input::get_terminal_size();
        const int boxWidth = 50;
        const int boxHeight = 20;
        int boxRow = std::max(2, (termSize.height - boxHeight) / 2);
        int boxCol = std::max(2, (termSize.width - boxWidth) / 2);
        
        ui::draw_box_double(boxRow, boxCol, boxWidth, boxHeight, constants::color_frame_main);
        
        ui::move_cursor(boxRow, boxCol + 2);
        ui::set_color(constants::color_frame_main);
        std::cout << " 🎉 TUTORIAL COMPLETE! ";
        ui::reset_color();
        
        int row = boxRow + 2;
        ui::move_cursor(row++, boxCol + 4);
        std::cout << "You've learned:";
        row++;
        
        std::vector<std::string> learned = {
            "✓ Movement & Navigation",
            "✓ UI Views (TAB cycling)",
            "✓ Inventory Management",
            "✓ Equipment & Dual Wielding",
            "✓ Combat System",
            "✓ Weapon Attacks & Cooldowns",
            "✓ Consumables",
            "✓ Loot Collection",
            "✓ Status Effects",
            "✓ Environmental Hazards",
            "✓ Descending Stairs"
        };
        
        for (const auto& item : learned) {
            ui::move_cursor(row++, boxCol + 6);
            std::cout << item;
        }
        
        row += 2;
        ui::move_cursor(row++, boxCol + 4);
        std::cout << "Ready to start your adventure?";
        row++;
        ui::move_cursor(row++, boxCol + 4);
        ui::set_color(constants::color_msg_info);
        std::cout << glyphs::msg_info() << " Press any key to begin Floor 1...";
        ui::reset_color();
        
        std::cout.flush();
        input::read_key_blocking();
    }
    
    // Main tutorial level function
    bool run_tutorial_level() {
        ui::clear();
        
        // Introduction screen
        auto termSize = input::get_terminal_size();
        const int boxWidth = 70;
        const int boxHeight = 20;
        int boxRow = std::max(2, (termSize.height - boxHeight) / 2);
        int boxCol = std::max(2, (termSize.width - boxWidth) / 2);
        
        ui::draw_box_double(boxRow, boxCol, boxWidth, boxHeight, constants::color_frame_main);
        
        ui::move_cursor(boxRow, boxCol + 2);
        ui::set_color(constants::color_frame_main);
        std::cout << " " << glyphs::msg_info() << " TUTORIAL LEVEL - THE TRAINING HALL ";
        ui::reset_color();
        
        int row = boxRow + 2;
        ui::move_cursor(row++, boxCol + 4);
        std::cout << "Welcome to the Comprehensive Tutorial!";
        row++;
        ui::move_cursor(row++, boxCol + 4);
        std::cout << "In this tutorial, you'll learn about:";
        row++;
        ui::move_cursor(row++, boxCol + 6);
        std::cout << glyphs::arrow_right() << " Movement & Navigation";
        ui::move_cursor(row++, boxCol + 6);
        std::cout << glyphs::arrow_right() << " UI Views & Interface";
        ui::move_cursor(row++, boxCol + 6);
        std::cout << glyphs::arrow_right() << " Inventory & Equipment";
        ui::move_cursor(row++, boxCol + 6);
        std::cout << glyphs::arrow_right() << " Dual Wielding";
        ui::move_cursor(row++, boxCol + 6);
        std::cout << glyphs::arrow_right() << " Combat System";
        ui::move_cursor(row++, boxCol + 6);
        std::cout << glyphs::arrow_right() << " Loot & Status Effects";
        ui::move_cursor(row++, boxCol + 6);
        std::cout << glyphs::arrow_right() << " Environmental Hazards";
        row++;
        ui::move_cursor(row++, boxCol + 4);
        std::cout << "You have infinite health for safe practice.";
        row++;
        ui::move_cursor(row++, boxCol + 4);
        ui::set_color(constants::color_msg_info);
        std::cout << glyphs::msg_info() << " Press any key to begin...";
        ui::reset_color();
        
        std::cout.flush();
        input::read_key_blocking();
        
        // Initialize tutorial state
        TutorialState state;
        state.currentSection = 0;
        state.showTips = true;
        state.infiniteHealth = true;
        state.tipHistoryIndex = -1;
        state.reviewingTips = false;
        state.lastActionTime = std::chrono::steady_clock::now();
        state.promptDismissed = false;
        state.movementPhase = 0;  // Start at phase 0: no movement yet
        
        // Track last section for message reminders
        int lastSection = -1;
        int lastMovementPhase = -1;
        
        // Create tutorial dungeon
        Dungeon tutorialDungeon = generate_tutorial_dungeon();
        state.dungeon = &tutorialDungeon;
        
        // Create tutorial player (Warrior class, infinite health)
        Player tutorialPlayer(PlayerClass::Warrior);
        tutorialPlayer.get_stats().hp = 9999;
        tutorialPlayer.get_stats().maxHp = 9999;
        // Spawn player at center of Room 1 (Room 1 is 6x6 from (2,2) to (7,7), center is ~(4.5, 4.5))
        // Use (4, 4) for center position
        tutorialPlayer.set_position(4, 4);  // Center of Room 1
        state.player = &tutorialPlayer;
        
        // Create message log
        MessageLog log;
        state.log = &log;
        
        // Pre-place items in rooms
        std::mt19937 rng(12345);  // Fixed seed for tutorial
        
        // Room 3: Pre-place items
        Item weapon1 = loot::generate_weapon(1, rng);
        weapon1.name = "Training Sword";
        Item armor1 = loot::generate_armor(1, rng);
        armor1.name = "Training Armor";
        Item consumable1 = loot::generate_consumable(1, rng);
        consumable1.name = "Minor Tonic";
        
        // Store items for pickup (we'll handle this in the main loop)
        std::vector<Item> room3Items = {weapon1, armor1, consumable1};
        std::vector<Position> room3ItemPositions = {{25, 2}, {26, 2}, {27, 2}};  // Updated for new room position
        
        // Room 4: Weapon
        Item weapon2 = loot::generate_weapon(1, rng);
        weapon2.name = "Practice Blade";
        std::vector<Item> room4Items = {weapon2};
        std::vector<Position> room4ItemPositions = {{35, 2}};  // Updated for new room position
        
        // Room 5: Two weapons
        Item weapon3 = loot::generate_weapon(1, rng);
        weapon3.name = "Main Hand Sword";
        Item weapon4 = loot::generate_weapon(1, rng);
        weapon4.name = "Offhand Dagger";
        std::vector<Item> room5Items = {weapon3, weapon4};
        std::vector<Position> room5ItemPositions = {{43, 2}, {45, 2}};  // Updated for new room position
        
        // Room 7: Loot items (will be placed after combat)
        std::vector<Item> room7Items;
        std::vector<Position> room7ItemPositions;
        
        // Initialize enemies
        std::vector<Enemy> enemies;
        
        // Room 6: Weak rat for combat practice (will be spawned later in movement section)
        // No enemies in Room 1 - combat tutorial comes later
        
        // Room 8: Spider for status effects (add to enemies for drawing, but handle separately in section 12)
        Enemy tutorialSpider(EnemyType::Spider);
        tutorialSpider.stats().hp = 9999;  // Infinite for practice
        tutorialSpider.stats().maxHp = 9999;
        tutorialSpider.set_position(7, 14);  // Keep same position (Room 8 unchanged)
        enemies.push_back(tutorialSpider);  // Add to enemies vector so it's drawn on map
        
        // Main tutorial loop
        bool tutorialRunning = true;
        UIView currentView = UIView::MAP;
        int invSel = 0;
        
        while (tutorialRunning) {
            ui::clear();
            
            // Get terminal size
            auto termSize = input::get_terminal_size();
            
            // Calculate viewport using standard function
            auto vpSize = input::calculate_viewport(termSize.width, termSize.height);
            int viewport_w = vpSize.width;
            int viewport_h = vpSize.height;
            
            // Calculate tips and objective first to determine dynamic width (if tips are shown)
            std::vector<std::string> tips;
            std::string objective;
            int tipPanelWidth = 32;  // Default width
            if (state.showTips) {
                // Section-specific tips
                switch (state.currentSection) {
                    case 0:  // Movement
                        if (state.movementPhase == 0) {
                            int directionsMoved = 0;
                            if (state.movedNorth) directionsMoved++;
                            if (state.movedSouth) directionsMoved++;
                            if (state.movedEast) directionsMoved++;
                            if (state.movedWest) directionsMoved++;
                            
                            tips = {
                                "Welcome! You are the @ symbol",
                                "",
                                "Use WASD or Arrow Keys",
                                "to move around",
                                "",
                                "Try moving in all",
                                "4 directions!",
                                "",
                                "Progress: " + std::to_string(directionsMoved) + "/4"
                            };
                            objective = "Move in all 4 directions (W/A/S/D)";
                        } else {
                            tips = {
                                "Great! You can move!",
                                "",
                                "Now move to the room",
                                "on the right",
                                "",
                                "Use D or → to go right",
                                "and follow the corridor"
                            };
                            objective = "Move to the room on the right";
                        }
                        store_tip_history(state, tips);
                        break;
                    case 1:  // UI Views
                        {
                            int viewsSeen = static_cast<int>(state.viewedScreens.size());
                            std::string progressText = "Progress: " + std::to_string(viewsSeen) + "/5 screens";
                            std::string currentViewName = "Map";
                            if (currentView == UIView::INVENTORY) currentViewName = "Inventory";
                            else if (currentView == UIView::STATS) currentViewName = "Stats";
                            else if (currentView == UIView::EQUIPMENT) currentViewName = "Equipment";
                            else if (currentView == UIView::MESSAGE_LOG) currentViewName = "Messages";
                            
                            tips = {
                                "Press TAB to cycle",
                                "through UI views",
                                "",
                                "Current view: " + currentViewName,
                                "",
                                "You must view all 5:",
                                "1. Map",
                                "2. Inventory",
                                "3. Stats",
                                "4. Equipment",
                                "5. Messages",
                                "",
                                progressText,
                                "",
                                "Each view shows a",
                                "description below it",
                                "",
                                "Press ESC from any",
                                "menu to return to Map"
                            };
                            objective = "View all 5 screens, then return to Map";
                            store_tip_history(state, tips);
                        }
                        break;
                    case 2:  // Inventory
                        tips = {
                            "Press 'i' to open",
                            "inventory",
                            "",
                            "Use w/s or arrows",
                            "to navigate items",
                            "",
                            "Press ESC or 'i'",
                            "again to close"
                        };
                        objective = "Open and navigate inventory";
                        store_tip_history(state, tips);
                        break;
                    case 3:  // Equipment
                        tips = {
                            "Walk over items to",
                            "pick them up",
                            "",
                            "Select item and",
                            "press 'e' to equip",
                            "",
                            "Check Equipment view",
                            "(TAB) to see equipped"
                        };
                        objective = "Pick up and equip weapon";
                        store_tip_history(state, tips);
                        break;
                    case 4:  // Dual Wielding
                        tips = {
                            "You can equip",
                            "2 weapons!",
                            "",
                            "First weapon →",
                            "Main Hand",
                            "",
                            "Second weapon →",
                            "Offhand"
                        };
                        objective = "Equip both weapons";
                        store_tip_history(state, tips);
                        break;
                    case 5:  // Combat Basic
                    case 6:  // Combat Weapons
                    case 7:  // Combat Cooldowns
                    case 8:  // Combat Defend
                    case 9:  // Combat Consumables
                    case 10: // Combat Victory
                        tips = {
                            "Walk into enemy to",
                            "start combat!",
                            "",
                            "Use number keys",
                            "(1, 2, 3...) to",
                            "select actions",
                            "",
                            "Basic Attack is",
                            "always available"
                        };
                        objective = "Learn combat system";
                        store_tip_history(state, tips);
                        break;
                    case 11: // Loot
                        tips = {
                            "Enemies drop 1-3",
                            "items when killed!",
                            "",
                            "Walk over items",
                            "to pick them up",
                            "",
                            "Check inventory",
                            "to see what you got"
                        };
                        objective = "Collect loot items";
                        store_tip_history(state, tips);
                        break;
                    case 12: // Status Effects
                        tips = {
                            "Status effects",
                            "change abilities",
                            "",
                            "Poison: Damage",
                            "over time",
                            "",
                            "Check status icons",
                            "above HP bars"
                        };
                        objective = "Observe status effects";
                        store_tip_history(state, tips);
                        break;
                    case 13: // Hazards
                        tips = {
                            "^ = Trap",
                            "(damage)",
                            "",
                            "_ = Shrine",
                            "(heal - press E)",
                            "",
                            "~ = Water",
                            "(slows movement)"
                        };
                        objective = "Interact with hazards";
                        store_tip_history(state, tips);
                        break;
                    case 14: // Stairs
                        tips = {
                            "Stand on > to",
                            "descend",
                            "",
                            "Press > key to",
                            "go to next floor",
                            "",
                            "In real game,",
                            "floors get harder!"
                        };
                        objective = "Descend stairs";
                        store_tip_history(state, tips);
                        break;
                    default:
                        tips = {"Tutorial complete!"};
                        objective = "Complete";
                        store_tip_history(state, tips);
                        break;
                }
                
                // Calculate dynamic tip panel width based on content
                tipPanelWidth = calculate_tip_width(tips, objective, calculate_tutorial_progress(state), state);
            }
            
            // Account for tip panel width with proper spacing
            const int tipPanelSpacing = 4;  // Add spacing between map and tip panel
            const int minMapMargin = 2;      // Minimum margin on left side
            
            // Calculate available width with dynamic tip panel width
            int availableWidth = termSize.width - tipPanelWidth - tipPanelSpacing - minMapMargin;
            
            // Constrain viewport more aggressively for tutorial
            if (state.showTips) {
                // Use smaller max width for tutorial to prevent overflow
                const int maxTutorialWidth = 60;  // Smaller max for tutorial
                viewport_w = std::min({viewport_w, availableWidth - game_constants::UI_BORDER_WIDTH, maxTutorialWidth});
            } else {
                // Even without tips, keep tutorial viewport smaller
                const int maxTutorialWidth = 70;
                viewport_w = std::min(viewport_w, maxTutorialWidth);
            }
            
            // Also constrain height for better proportions
            const int maxTutorialHeight = 22;
            viewport_h = std::min(viewport_h, maxTutorialHeight);
            
            // Calculate centered UI layout with more spacing
            int mapFrameHeight = viewport_h + game_constants::UI_BORDER_WIDTH;
            int statusFrameHeight = game_constants::UI_STATUS_FRAME_HEIGHT;
            int messageFrameHeight = game_constants::UI_MESSAGE_FRAME_HEIGHT;
            int verticalSpacing = 2;  // Add extra spacing between sections
            int totalHeight = mapFrameHeight + statusFrameHeight + messageFrameHeight + 
                              game_constants::UI_BORDER_WIDTH + verticalSpacing;
            int totalWidth = viewport_w + game_constants::UI_BORDER_WIDTH;
            
            // Center on terminal, but leave proper space for tip panel
            int mapStartRow = std::max(2, (termSize.height - totalHeight) / 2);  // More top margin
            int mapStartCol = std::max(minMapMargin, 
                (termSize.width - totalWidth - (state.showTips ? (tipPanelWidth + tipPanelSpacing) : 0)) / 2);
            
            int statusRow = mapStartRow + mapFrameHeight + 1;
            int msgRow = statusRow + statusFrameHeight + 1;
            
            // Draw map viewport only when in MAP view (hide map for other views in UI Views section)
            if (currentView == UIView::MAP) {
                // Use standard viewport rendering
                draw_map_viewport(*state.dungeon, *state.player, enemies, mapStartRow, mapStartCol, viewport_w, viewport_h);
                
                // Calculate camera position for overlay drawing
                Position playerPos = state.player->get_position();
                int cam_x = playerPos.x - viewport_w / 2;
                int cam_y = playerPos.y - viewport_h / 2;
                // Tutorial dungeon is 50x20 (expanded for larger rooms)
                cam_x = std::max(0, std::min(cam_x, 50 - viewport_w));
                cam_y = std::max(0, std::min(cam_y, 20 - viewport_h));
                
                // Draw tutorial-specific overlays (item highlighting, indicators)
                draw_tutorial_overlays(state, room3Items, room3ItemPositions, 
                                      room4ItemPositions,
                                      room5ItemPositions,
                                      room7Items, room7ItemPositions,
                                      enemies,
                                      mapStartRow, mapStartCol,
                                      viewport_w, viewport_h,
                                      cam_x, cam_y);
                
                // Draw status bar and message log
                ui::draw_status_bar_framed(statusRow, mapStartCol, viewport_w + 2, *state.player, 0);
                
                // Draw message log using standard function
                int messageLogWidth = viewport_w + 2;
                if (state.showTips) {
                    messageLogWidth = std::min(termSize.width - mapStartCol - 2, 
                                             viewport_w + tipPanelWidth + tipPanelSpacing + 2);
                } else {
                    messageLogWidth = std::min(termSize.width - mapStartCol - 2, viewport_w + 30);
                }
                log.render_framed(msgRow, mapStartCol, messageLogWidth, 8);
            } else {
                // For other views, show the view with description
                int viewWidth = std::min(70, termSize.width - 4);
                int viewHeight = std::min(25, termSize.height - 4);
                
                // Center horizontally, but account for tip panel if visible
                int availableWidth = termSize.width;
                if (state.showTips) {
                    availableWidth = availableWidth - tipPanelWidth - tipPanelSpacing - 4;
                }
                int viewCol = std::max(2, (availableWidth - viewWidth) / 2);
                int viewRow = std::max(2, (termSize.height - viewHeight) / 2);
                
                // Draw the view
                switch (currentView) {
                    case UIView::INVENTORY:
                        ui::draw_full_inventory_view(viewRow, viewCol, viewWidth, viewHeight, 
                                                      *state.player, invSel, 0);
                        break;
                    case UIView::STATS:
                        ui::draw_stats_view(viewRow, viewCol, viewWidth, viewHeight, 
                                            *state.player, 0, 0);
                        break;
                    case UIView::EQUIPMENT:
                        ui::draw_equipment_view(viewRow, viewCol, viewWidth, viewHeight, *state.player);
                        break;
                    case UIView::MESSAGE_LOG:
                        ui::draw_message_log_view(viewRow, viewCol, viewWidth, viewHeight,
                                                  log, 0);
                        break;
                    default:
                        break;
                }
                
                // Draw description below the view
                int descRow = viewRow + viewHeight + 2;
                int descCol = viewCol;
                std::string description;
                switch (currentView) {
                    case UIView::INVENTORY:
                        description = "INVENTORY: View and manage all your items. Use W/S to navigate, E to equip, U to use, D to drop.";
                        break;
                    case UIView::STATS:
                        description = "STATS: View your character's statistics, level, and combat information.";
                        break;
                    case UIView::EQUIPMENT:
                        description = "EQUIPMENT: See what items you have equipped in each slot (weapons, armor, etc.).";
                        break;
                    case UIView::MESSAGE_LOG:
                        description = "MESSAGE LOG: Review recent game messages and events.";
                        break;
                    default:
                        description = "Press TAB to cycle views, ESC to return to Map.";
                        break;
                }
                
                // Draw description in a box
                int descWidth = std::min(static_cast<int>(description.length()) + 4, termSize.width - descCol - 2);
                ui::draw_box_single(descRow, descCol, descWidth, 3, constants::color_frame_message);
                ui::move_cursor(descRow, descCol + 2);
                ui::set_color(constants::color_frame_message);
                std::cout << " Description ";
                ui::reset_color();
                ui::move_cursor(descRow + 1, descCol + 2);
                // Word wrap description
                std::string wrapped = description;
                if (static_cast<int>(wrapped.length()) > descWidth - 4) {
                    wrapped = wrapped.substr(0, descWidth - 7) + "...";
                }
                std::cout << wrapped;
            }
            
            // Draw side tip panel (if enabled) - tips and objective already calculated above
            if (state.showTips) {
                int tipCol = mapStartCol + viewport_w + game_constants::UI_BORDER_WIDTH + tipPanelSpacing;
                int tipRow = mapStartRow;
                render_side_tips(tipRow, tipCol, tips, objective, calculate_tutorial_progress(state), state);
            }
            
            std::cout.flush();
            
            // Handle input
            int key = input::read_key_nonblocking();
            if (key == -1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }
            
            // Track action time for stuck detection
            state.lastActionTime = std::chrono::steady_clock::now();
            
            // Dismiss prompt (Space key)
            if (key == ' ') {
                state.promptDismissed = true;
                continue;
            }
            
            // Toggle tips
            if (key == 'h' || key == 'H') {
                state.showTips = !state.showTips;
                continue;
            }
            
            // Review tips (R key)
            if (key == 'r' || key == 'R') {
                state.reviewingTips = !state.reviewingTips;
                continue;
            }
            
            // Skip tutorial (K key) - with confirmation (changed from S to avoid conflict with movement)
            if (key == 'k' || key == 'K') {
                if (show_skip_confirmation()) {
                    // Show completion screen and return
                    show_tutorial_completion_screen();
                    return true;  // Return true to indicate completion (even if skipped)
                }
                continue;
            }
            
            // Pause tutorial (P key) - skip option is also in pause menu
            if (key == 'p' || key == 'P') {
                // Show pause menu
                auto termSize = input::get_terminal_size();
                int pauseRow = termSize.height / 2;
                int pauseCol = termSize.width / 2 - 15;
                ui::draw_box_double(pauseRow, pauseCol, 30, 10, constants::color_frame_main);
                ui::move_cursor(pauseRow, pauseCol + 2);
                ui::set_color(constants::color_frame_main);
                std::cout << " TUTORIAL PAUSED ";
                ui::reset_color();
                ui::move_cursor(pauseRow + 2, pauseCol + 4);
                std::cout << "[P] Resume";
                ui::move_cursor(pauseRow + 3, pauseCol + 4);
                std::cout << "[S] Skip Tutorial";
                ui::move_cursor(pauseRow + 4, pauseCol + 4);
                std::cout << "[Q] Quit Tutorial";
                std::cout.flush();
                
                while (true) {
                    int pauseKey = input::read_key_blocking();
                    if (pauseKey == 'p' || pauseKey == 'P') {
                        break;  // Resume
                    } else if (pauseKey == 's' || pauseKey == 'S') {
                        if (show_skip_confirmation()) {
                            show_tutorial_completion_screen();
                            return true;
                        }
                        // Redraw pause menu
                        ui::draw_box_double(pauseRow, pauseCol, 30, 10, constants::color_frame_main);
                        ui::move_cursor(pauseRow, pauseCol + 2);
                        ui::set_color(constants::color_frame_main);
                        std::cout << " TUTORIAL PAUSED ";
                        ui::reset_color();
                        ui::move_cursor(pauseRow + 2, pauseCol + 4);
                        std::cout << "[P] Resume";
                        ui::move_cursor(pauseRow + 3, pauseCol + 4);
                        std::cout << "[S] Skip Tutorial";
                        ui::move_cursor(pauseRow + 4, pauseCol + 4);
                        std::cout << "[Q] Quit Tutorial";
                        std::cout.flush();
                    } else if (pauseKey == 'q' || pauseKey == 'Q') {
                        tutorialRunning = false;
                        return false;  // Quit
                    }
                }
                continue;
            }
            
            // Handle section-specific input
            switch (state.currentSection) {
                case 0:  // Movement - Progressive introduction
                    // Phase 0: Detect any WASD movement
                    if (state.movementPhase == 0) {
                        bool moved = false;
                        if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                                state.player->set_position(pos.x, pos.y - 1);
                                state.movedNorth = true;
                                moved = true;
                            }
                        } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                                state.player->set_position(pos.x, pos.y + 1);
                                state.movedSouth = true;
                                moved = true;
                            }
                        } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                                state.player->set_position(pos.x - 1, pos.y);
                                state.movedWest = true;
                                moved = true;
                            }
                        } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                                state.player->set_position(pos.x + 1, pos.y);
                                state.movedEast = true;
                                moved = true;
                            }
                        }
                        
                        // Advance to phase 2 (move to next room) only after moving in all 4 directions
                        if (state.movedNorth && state.movedSouth && state.movedEast && state.movedWest) {
                            state.movementPhase = 2;
                            log.add(MessageType::Info, "✓ Great! You've moved in all directions. Now move to the room on the right!");
                        } else if (moved) {
                            // Give feedback on progress
                            int directionsMoved = 0;
                            if (state.movedNorth) directionsMoved++;
                            if (state.movedSouth) directionsMoved++;
                            if (state.movedEast) directionsMoved++;
                            if (state.movedWest) directionsMoved++;
                            log.add(MessageType::Info, "✓ Good! Try moving in all 4 directions (W/A/S/D). Progress: " + std::to_string(directionsMoved) + "/4");
                        }
                    }
                    // Phase 2: Normal movement - move to Room 2 on the right
                    else if (state.movementPhase == 2) {
                        if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                                state.player->set_position(pos.x, pos.y - 1);
                                state.movedNorth = true;
                                state.completedObjectives.insert("move_north");
                            }
                        } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                                state.player->set_position(pos.x, pos.y + 1);
                                state.movedSouth = true;
                                state.completedObjectives.insert("move_south");
                            }
                        } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                                state.player->set_position(pos.x - 1, pos.y);
                                state.movedWest = true;
                                state.completedObjectives.insert("move_west");
                            }
                        } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                                state.player->set_position(pos.x + 1, pos.y);
                                state.movedEast = true;
                                state.completedObjectives.insert("move_east");
                            }
                        }
                        
                        // Check if player entered Room 2 (x: 14-17, y: 2-5)
                        Position playerPos = state.player->get_position();
                        if (playerPos.x >= 14 && playerPos.x < 18 && playerPos.y >= 2 && playerPos.y < 6) {
                            // Player reached Room 2 - complete movement section
                            state.completedObjectives.insert("reach_room2");
                            complete_tutorial_section(
                                state, lastSection,
                                "✓ You reached the next room! Movement section complete.",
                                "💡 Next: Learn about UI Views. Press TAB to cycle through different screens!",
                                15, 3,  // Room 2 center
                                "💡 Next Section: UI Views - Press TAB to cycle through different screens!"
                            );
                            continue;  // Skip to next loop iteration
                        }
                    }
                    
                    // Show guided prompt based on phase
                    // Section completion is handled in phase 2 when player enters Room 2
                    if (state.movementPhase == 0) {
                        int directionsMoved = 0;
                        if (state.movedNorth) directionsMoved++;
                        if (state.movedSouth) directionsMoved++;
                        if (state.movedEast) directionsMoved++;
                        if (state.movedWest) directionsMoved++;
                        show_guided_prompt("Use WASD to move in all 4 directions (" + std::to_string(directionsMoved) + "/4)", state, true);
                    } else if (state.movementPhase == 2) {
                        show_guided_prompt("Move to the room on the right (use D or →)", state, true);
                    }
                    break;
                    
                case 1:  // UI Views
                    // Allow movement in UI Views section
                    if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                            state.player->set_position(pos.x, pos.y - 1);
                        }
                    } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                            state.player->set_position(pos.x, pos.y + 1);
                        }
                    } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                            state.player->set_position(pos.x - 1, pos.y);
                        }
                    } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                            state.player->set_position(pos.x + 1, pos.y);
                        }
                    } else if (key == '\t') {
                        int v = static_cast<int>(currentView);
                        v = (v + 1) % 5;
                        currentView = static_cast<UIView>(v);
                        state.viewedScreens.insert(v);
                        // Give feedback on progress
                        int viewsSeen = static_cast<int>(state.viewedScreens.size());
                        if (viewsSeen < 5) {
                            log.add(MessageType::Info, "✓ Viewed " + std::to_string(viewsSeen) + "/5 screens. Keep pressing TAB!");
                        } else {
                            log.add(MessageType::Info, "✓ All 5 screens viewed! Press ESC from any menu to return to the Map.");
                        }
                    } else if (key == 27 || key == 'q' || key == 'Q') {
                        currentView = UIView::MAP;
                        state.returnedToMap = true;
                        if (state.viewedScreens.size() >= 5) {
                            log.add(MessageType::Info, "✓ Returned to Map view! Section complete.");
                        } else {
                            int viewsSeen = static_cast<int>(state.viewedScreens.size());
                            log.add(MessageType::Warning, "You've only viewed " + std::to_string(viewsSeen) + "/5 screens. Press TAB to see more!");
                        }
                    }
                    
                    if (section_ui_views(state)) {
                        complete_tutorial_section(
                            state, lastSection,
                            "✓ UI Views section complete! Moving to next room...",
                            "💡 Next: You'll learn about Inventory management. Move to the next room!",
                            25, 2,  // Room 3 (Inventory)
                            "💡 Next Section: Inventory Management - Move to the room on the right to continue!"
                        );
                        continue;  // Skip to next loop iteration
                    } else {
                        int viewsSeen = static_cast<int>(state.viewedScreens.size());
                        if (viewsSeen < 5) {
                            show_guided_prompt("Press TAB to view all 5 screens (" + std::to_string(viewsSeen) + "/5 viewed)", state);
                        } else {
                            show_guided_prompt("All 5 screens viewed! Press ESC from any menu to return to the Map", state);
                        }
                    }
                    break;
                    
                case 2:  // Inventory
                    // If inventory menu is open, keys should control the menu, not movement
                    if (currentView == UIView::INVENTORY) {
                        if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                            if (invSel > 0) {
                                invSel--;
                                state.itemsNavigated++;
                            }
                        } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                            if (invSel < static_cast<int>(state.player->inventory().size()) - 1) {
                                invSel++;
                                state.itemsNavigated++;
                            }
                        } else if (key == 'e' || key == 'E') {
                            // Allow equipping from inventory in this section as well
                            if (!state.player->inventory().empty()) {
                                size_t idx = static_cast<size_t>(std::clamp(invSel, 0, static_cast<int>(state.player->inventory().size()) - 1));
                                state.player->equip_item(idx);
                                log.add(MessageType::Info, "Equipped: " + state.player->inventory()[idx].name);
                            }
                        } else if (key == 27 || key == 'i' || key == 'I') {
                            // ESC or 'i' closes the inventory and returns to map
                            currentView = UIView::MAP;
                            state.inventoryClosed = true;
                        }
                    } else {
                        // Movement to pick up items in room 3 (when inventory is not open)
                        if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                                state.player->set_position(pos.x, pos.y - 1);
                                pos = state.player->get_position();
                                // Check if standing on item
                                for (size_t i = 0; i < room3ItemPositions.size(); ++i) {
                                    if (room3ItemPositions[i].x == pos.x && room3ItemPositions[i].y == pos.y) {
                                        std::string itemName = room3Items[i].name;
                                        state.player->inventory().push_back(room3Items[i]);
                                        room3Items.erase(room3Items.begin() + i);
                                        room3ItemPositions.erase(room3ItemPositions.begin() + i);
                                        log.add(MessageType::Loot, "Picked up: " + itemName);
                                        break;
                                    }
                                }
                            }
                        } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                                state.player->set_position(pos.x, pos.y + 1);
                                pos = state.player->get_position();
                                for (size_t i = 0; i < room3ItemPositions.size(); ++i) {
                                    if (room3ItemPositions[i].x == pos.x && room3ItemPositions[i].y == pos.y) {
                                        std::string itemName = room3Items[i].name;
                                        state.player->inventory().push_back(room3Items[i]);
                                        room3Items.erase(room3Items.begin() + i);
                                        room3ItemPositions.erase(room3ItemPositions.begin() + i);
                                        log.add(MessageType::Loot, "Picked up: " + itemName);
                                        break;
                                    }
                                }
                            }
                        } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                                state.player->set_position(pos.x - 1, pos.y);
                                pos = state.player->get_position();
                                for (size_t i = 0; i < room3ItemPositions.size(); ++i) {
                                    if (room3ItemPositions[i].x == pos.x && room3ItemPositions[i].y == pos.y) {
                                        std::string itemName = room3Items[i].name;
                                        state.player->inventory().push_back(room3Items[i]);
                                        room3Items.erase(room3Items.begin() + i);
                                        room3ItemPositions.erase(room3ItemPositions.begin() + i);
                                        log.add(MessageType::Loot, "Picked up: " + itemName);
                                        break;
                                    }
                                }
                            }
                        } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                            Position pos = state.player->get_position();
                            if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                                state.player->set_position(pos.x + 1, pos.y);
                                pos = state.player->get_position();
                                for (size_t i = 0; i < room3ItemPositions.size(); ++i) {
                                    if (room3ItemPositions[i].x == pos.x && room3ItemPositions[i].y == pos.y) {
                                        std::string itemName = room3Items[i].name;
                                        state.player->inventory().push_back(room3Items[i]);
                                        room3Items.erase(room3Items.begin() + i);
                                        room3ItemPositions.erase(room3ItemPositions.begin() + i);
                                        log.add(MessageType::Loot, "Picked up: " + itemName);
                                        break;
                                    }
                                }
                            }
                        } else if (key == 'i' || key == 'I') {
                            currentView = UIView::INVENTORY;
                            state.inventoryOpened = true;
                            invSel = 0;
                        }
                    }
                    
                    if (section_inventory(state)) {
                        complete_tutorial_section(
                            state, lastSection,
                            "✓ Inventory section complete! Moving to next room...",
                            "💡 Next: You'll learn about Equipment. Move to the next room!",
                            35, 2,  // Room 4 (Equipment)
                            ""  // No prompt needed, player will see equipment tips
                        );
                        continue;
                    } else {
                        if (!state.inventoryOpened) {
                            show_guided_prompt("Press 'i' to open inventory", state);
                        } else if (state.itemsNavigated < 2) {
                            show_guided_prompt("Navigate through items with W/S or arrow keys", state);
                        } else if (!state.inventoryClosed) {
                            show_guided_prompt("Press 'i' again or ESC to close inventory", state);
                        }
                    }
                    break;
                    
                case 3:  // Equipment
                    // Movement to pick up item
                    if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                            state.player->set_position(pos.x, pos.y - 1);
                            // Check if standing on item
                            for (size_t i = 0; i < room4ItemPositions.size(); ++i) {
                                if (room4ItemPositions[i].x == pos.x && room4ItemPositions[i].y == pos.y) {
                                    std::string itemName = room4Items[i].name;
                                    state.player->inventory().push_back(room4Items[i]);
                                    room4Items.erase(room4Items.begin() + i);
                                    room4ItemPositions.erase(room4ItemPositions.begin() + i);
                                    state.itemPickedUp = true;
                                    log.add(MessageType::Loot, "Picked up: " + itemName);
                                    break;
                                }
                            }
                        }
                    } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                            state.player->set_position(pos.x, pos.y + 1);
                            // Check if standing on item
                            for (size_t i = 0; i < room4ItemPositions.size(); ++i) {
                                if (room4ItemPositions[i].x == pos.x && room4ItemPositions[i].y == pos.y) {
                                    std::string itemName = room4Items[i].name;
                                    state.player->inventory().push_back(room4Items[i]);
                                    room4Items.erase(room4Items.begin() + i);
                                    room4ItemPositions.erase(room4ItemPositions.begin() + i);
                                    state.itemPickedUp = true;
                                    log.add(MessageType::Loot, "Picked up: " + itemName);
                                    break;
                                }
                            }
                        }
                    } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                            state.player->set_position(pos.x - 1, pos.y);
                            // Check if standing on item
                            for (size_t i = 0; i < room4ItemPositions.size(); ++i) {
                                if (room4ItemPositions[i].x == pos.x && room4ItemPositions[i].y == pos.y) {
                                    std::string itemName = room4Items[i].name;
                                    state.player->inventory().push_back(room4Items[i]);
                                    room4Items.erase(room4Items.begin() + i);
                                    room4ItemPositions.erase(room4ItemPositions.begin() + i);
                                    state.itemPickedUp = true;
                                    log.add(MessageType::Loot, "Picked up: " + itemName);
                                    break;
                                }
                            }
                        }
                    } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                            state.player->set_position(pos.x + 1, pos.y);
                            // Check if standing on item
                            for (size_t i = 0; i < room4ItemPositions.size(); ++i) {
                                if (room4ItemPositions[i].x == pos.x && room4ItemPositions[i].y == pos.y) {
                                    std::string itemName = room4Items[i].name;
                                    state.player->inventory().push_back(room4Items[i]);
                                    room4Items.erase(room4Items.begin() + i);
                                    room4ItemPositions.erase(room4ItemPositions.begin() + i);
                                    state.itemPickedUp = true;
                                    log.add(MessageType::Loot, "Picked up: " + itemName);
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Equip item
                    if (key == 'e' || key == 'E') {
                        if (currentView == UIView::INVENTORY && !state.player->inventory().empty()) {
                            size_t idx = static_cast<size_t>(std::clamp(invSel, 0, static_cast<int>(state.player->inventory().size()) - 1));
                            state.player->equip_item(idx);
                            state.itemEquipped = true;
                            log.add(MessageType::Info, "Equipped: " + state.player->inventory()[idx].name);
                        }
                    }
                    
                    // View equipment
                    if (key == '\t') {
                        int v = static_cast<int>(currentView);
                        v = (v + 1) % 5;
                        currentView = static_cast<UIView>(v);
                        if (currentView == UIView::EQUIPMENT) {
                            state.equipmentViewed = true;
                        }
                    }
                    
                    if (section_equipment(state)) {
                        complete_tutorial_section(
                            state, lastSection,
                            "✓ Equipment section complete! Moving to next room...",
                            "💡 Next: You'll learn about Dual Wielding. Move to the next room!",
                            44, 2,  // Room 5 (Dual Wielding)
                            ""
                        );
                        continue;
                    } else {
                        if (!state.itemPickedUp) {
                            show_guided_prompt("Walk over the weapon on the ground to pick it up", state);
                        } else if (!state.itemEquipped) {
                            show_guided_prompt("Open inventory (i) and press 'e' to equip the weapon", state);
                        } else if (!state.equipmentViewed) {
                            show_guided_prompt("Press TAB to view Equipment screen and see your equipped item", state);
                        }
                    }
                    break;
                    
                case 4:  // Dual Wielding
                    // Movement to pick up items
                    if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                            state.player->set_position(pos.x, pos.y - 1);
                            // Check if standing on item
                            for (size_t i = 0; i < room5ItemPositions.size(); ++i) {
                                if (room5ItemPositions[i].x == pos.x && room5ItemPositions[i].y == pos.y) {
                                    state.player->inventory().push_back(room5Items[i]);
                                    std::string itemName = room5Items[i].name;
                                    room5Items.erase(room5Items.begin() + i);
                                    room5ItemPositions.erase(room5ItemPositions.begin() + i);
                                    log.add(MessageType::Loot, "Picked up: " + itemName);
                                    break;
                                }
                            }
                        }
                    } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                            state.player->set_position(pos.x, pos.y + 1);
                            for (size_t i = 0; i < room5ItemPositions.size(); ++i) {
                                if (room5ItemPositions[i].x == pos.x && room5ItemPositions[i].y == pos.y) {
                                    state.player->inventory().push_back(room5Items[i]);
                                    std::string itemName = room5Items[i].name;
                                    room5Items.erase(room5Items.begin() + i);
                                    room5ItemPositions.erase(room5ItemPositions.begin() + i);
                                    log.add(MessageType::Loot, "Picked up: " + itemName);
                                    break;
                                }
                            }
                        }
                    } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                            state.player->set_position(pos.x - 1, pos.y);
                            for (size_t i = 0; i < room5ItemPositions.size(); ++i) {
                                if (room5ItemPositions[i].x == pos.x && room5ItemPositions[i].y == pos.y) {
                                    state.player->inventory().push_back(room5Items[i]);
                                    std::string itemName = room5Items[i].name;
                                    room5Items.erase(room5Items.begin() + i);
                                    room5ItemPositions.erase(room5ItemPositions.begin() + i);
                                    log.add(MessageType::Loot, "Picked up: " + itemName);
                                    break;
                                }
                            }
                        }
                    } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                            state.player->set_position(pos.x + 1, pos.y);
                            for (size_t i = 0; i < room5ItemPositions.size(); ++i) {
                                if (room5ItemPositions[i].x == pos.x && room5ItemPositions[i].y == pos.y) {
                                    state.player->inventory().push_back(room5Items[i]);
                                    std::string itemName = room5Items[i].name;
                                    room5Items.erase(room5Items.begin() + i);
                                    room5ItemPositions.erase(room5ItemPositions.begin() + i);
                                    log.add(MessageType::Loot, "Picked up: " + itemName);
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Equip items
                    if (key == 'e' || key == 'E') {
                        if (currentView == UIView::INVENTORY && !state.player->inventory().empty()) {
                            size_t idx = static_cast<size_t>(std::clamp(invSel, 0, static_cast<int>(state.player->inventory().size()) - 1));
                            Item& item = state.player->inventory()[idx];
                            if (item.type == ItemType::Weapon) {
                                state.player->equip_item(idx);
                                if (!state.firstWeaponEquipped) {
                                    state.firstWeaponEquipped = true;
                                    log.add(MessageType::Info, "Equipped to Main Hand: " + item.name);
                                } else if (!state.secondWeaponEquipped) {
                                    state.secondWeaponEquipped = true;
                                    log.add(MessageType::Info, "Equipped to Offhand: " + item.name);
                                    log.add(MessageType::Info, "Dual wielding active!");
                                }
                            }
                        }
                    }
                    
                    // View equipment
                    if (key == '\t') {
                        int v = static_cast<int>(currentView);
                        v = (v + 1) % 5;
                        currentView = static_cast<UIView>(v);
                    }
                    
                    if (section_dual_wielding(state)) {
                        complete_tutorial_section(
                            state, lastSection,
                            "✓ Dual Wielding section complete! Moving to combat...",
                            "💡 Next: Learn combat basics. Walk into the enemy to start combat!",
                            39, 17,  // Combat room position
                            ""
                        );
                        continue;
                    } else {
                        if (!state.firstWeaponEquipped) {
                            show_guided_prompt("Pick up and equip the first weapon (goes to Main Hand)", state);
                        } else if (!state.secondWeaponEquipped) {
                            show_guided_prompt("Pick up and equip the second weapon (goes to Offhand)", state);
                        }
                    }
                    break;
                    
                // Combat sections (5-10) - handled by entering combat mode
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10: {
                    // Find the rat enemy (for combat sections)
                    Enemy* ratEnemy = nullptr;
                    for (auto& enemy : enemies) {
                        if (enemy.enemy_type() == EnemyType::Rat) {
                            ratEnemy = &enemy;
                            break;
                        }
                    }
                    
                    if (!ratEnemy) break;
                    
                    // Check if player walks into enemy
                    Position playerPos = state.player->get_position();
                    Position enemyPos = ratEnemy->get_position();
                    
                    if (playerPos.x == enemyPos.x && playerPos.y == enemyPos.y) {
                        // Enter combat
                        state.combatEntered = true;
                        bool combatWon = combat::enter_combat_mode(*state.player, *ratEnemy, *state.dungeon, log);
                        
                        // Track combat actions based on section
                        // Note: Actual action tracking would require modifying combat system
                        // For now, we mark as complete after combat for tutorial flow
                        if (state.currentSection == 5 && !state.basicAttackUsed) {
                            state.basicAttackUsed = true;
                            log.add(MessageType::Info, "Tutorial: You used Basic Attack! It has no cooldown.");
                        } else if (state.currentSection == 6 && !state.weaponAttackUsed) {
                            // Check if player has weapons equipped
                            bool hasWeapon = false;
                            for (const auto& item : state.player->inventory()) {
                                if (item.type == ItemType::Weapon) {
                                    hasWeapon = true;
                                    break;
                                }
                            }
                            if (hasWeapon) {
                                state.weaponAttackUsed = true;
                                log.add(MessageType::Info, "Tutorial: Check the Attack category - weapons unlock new attacks!");
                            }
                        } else if (state.currentSection == 7) {
                            state.cooldownObserved = true;
                            state.cooldownReset = true;
                            log.add(MessageType::Info, "Tutorial: Notice attacks with higher damage have cooldowns (CD: X)");
                        } else if (state.currentSection == 8 && !state.defendUsed) {
                            state.defendUsed = true;
                            log.add(MessageType::Info, "Tutorial: Defend reduces incoming damage by 50%!");
                        } else if (state.currentSection == 9 && !state.consumableUsed) {
                            state.consumableUsed = true;
                            log.add(MessageType::Info, "Tutorial: Consumables show effect preview and count in the menu!");
                        }
                        
                        // If combat won (section 10 - victory), spawn loot in room 7
                        if (state.currentSection == 10 && combatWon && ratEnemy->stats().hp <= 0) {
                            state.enemyDefeated = true;
                            // Spawn guaranteed 3-item drop in room 7
                            Position corpsePos{19, 14};  // Room 7 center
                            room7Items.clear();
                            room7ItemPositions.clear();
                            
                            Item loot1 = loot::generate_weapon(1, rng);
                            Item loot2 = loot::generate_armor(1, rng);
                            Item loot3 = loot::generate_consumable(1, rng);
                            
                            room7Items.push_back(loot1);
                            room7Items.push_back(loot2);
                            room7Items.push_back(loot3);
                            room7ItemPositions.push_back({corpsePos.x - 1, corpsePos.y});
                            room7ItemPositions.push_back(corpsePos);
                            room7ItemPositions.push_back({corpsePos.x + 1, corpsePos.y});
                            
                            log.add(MessageType::Loot, "The enemy dropped loot! Walk over items to pick them up.");
                        }
                        
                        // For sections 6-10, auto-advance after first combat (they're all in same combat)
                        // Section 5 (basic attack) requires actual combat completion
                        // Section 10 (victory) requires enemy defeat
                        if (state.currentSection == 5) {
                            // Section 5: Must complete basic attack
                            if (section_combat_basic(state)) {
                                // Reset rat HP for next section
                                ratEnemy->stats().hp = 10;
                                ratEnemy->stats().maxHp = 10;
                                complete_tutorial_section(
                                    state, lastSection,
                                    "✓ Basic Attack section complete!",
                                    "💡 Next: Try weapon attacks. Check the Attack category in combat menu!",
                                    state.player->get_position().x, state.player->get_position().y,  // Stay in same position
                                    ""
                                );
                                continue;
                            }
                        } else if (state.currentSection >= 6 && state.currentSection <= 9) {
                            // Sections 6-9: Auto-advance after combat (they're learning different aspects)
                            bool sectionComplete = false;
                            switch (state.currentSection) {
                                case 6: sectionComplete = section_combat_weapons(state); break;
                                case 7: sectionComplete = section_combat_cooldowns(state); break;
                                case 8: sectionComplete = section_combat_defend(state); break;
                                case 9: sectionComplete = section_combat_consumables(state); break;
                            }
                            
                            if (sectionComplete) {
                                // Reset rat HP for next section
                                ratEnemy->stats().hp = 10;
                                ratEnemy->stats().maxHp = 10;
                                std::string completionMsg = "";
                                std::string nextMsg = "";
                                switch (state.currentSection) {
                                    case 6: 
                                        completionMsg = "✓ Weapon Attacks section complete!";
                                        nextMsg = "💡 Next: Observe cooldowns. Use a high-damage attack and watch the cooldown!";
                                        break;
                                    case 7: 
                                        completionMsg = "✓ Cooldowns section complete!";
                                        nextMsg = "💡 Next: Try defending. Press D or select Defend to reduce damage!";
                                        break;
                                    case 8: 
                                        completionMsg = "✓ Defending section complete!";
                                        nextMsg = "💡 Next: Use consumables. Check the consumables in combat menu!";
                                        break;
                                    case 9: 
                                        completionMsg = "✓ Consumables section complete!";
                                        nextMsg = "💡 Next: Defeat the enemy! Reduce enemy HP to 0 to win!";
                                        break;
                                }
                                complete_tutorial_section(
                                    state, lastSection,
                                    completionMsg,
                                    nextMsg,
                                    state.player->get_position().x, state.player->get_position().y,  // Stay in same position
                                    ""
                                );
                                continue;
                            }
                        } else if (state.currentSection == 10) {
                            // Section 10: Must defeat enemy
                            if (section_combat_victory(state)) {
                                complete_tutorial_section(
                                    state, lastSection,
                                    "✓ Combat Victory section complete! Moving to loot room...",
                                    "💡 Next: Learn about loot collection. Walk over dropped items to pick them up!",
                                    19, 14,  // Room 7 for loot
                                    ""
                                );
                                continue;
                            }
                        }
                        break;
                    }
                    break;
                }
                    
                case 11: { // Loot
                    // Movement to pick up items from ground
                    Position playerPos = state.player->get_position();
                    
                    if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                            state.player->set_position(pos.x, pos.y - 1);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                            state.player->set_position(pos.x, pos.y + 1);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                            state.player->set_position(pos.x - 1, pos.y);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                            state.player->set_position(pos.x + 1, pos.y);
                            playerPos = state.player->get_position();
                        }
                    }
                    
                    // Check if standing on items
                    for (size_t i = 0; i < room7ItemPositions.size(); ++i) {
                        if (room7ItemPositions[i].x == playerPos.x && room7ItemPositions[i].y == playerPos.y) {
                            state.player->inventory().push_back(room7Items[i]);
                            std::string itemName = room7Items[i].name;
                            room7Items.erase(room7Items.begin() + i);
                            room7ItemPositions.erase(room7ItemPositions.begin() + i);
                            state.itemsPickedUp++;
                            log.add(MessageType::Loot, "Picked up: " + itemName);
                            break;
                        }
                    }
                    
                    if (section_loot(state)) {
                        complete_tutorial_section(
                            state, lastSection,
                            "✓ Loot section complete! Moving to status effects room...",
                            "💡 Next: Learn about status effects. Enter combat with the spider to see poison!",
                            7, 12,  // Room 8 (Status Effects)
                            ""
                        );
                        continue;
                    }
                    break;
                }
                    
                case 12: { // Status Effects
                    // Movement to enter combat with spider
                    Position playerPos = state.player->get_position();
                    
                    if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                            state.player->set_position(pos.x, pos.y - 1);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                            state.player->set_position(pos.x, pos.y + 1);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                            state.player->set_position(pos.x - 1, pos.y);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                            state.player->set_position(pos.x + 1, pos.y);
                            playerPos = state.player->get_position();
                        }
                    }
                    
                    // Check if walked into spider
                    Position spiderPos = tutorialSpider.get_position();
                    if (playerPos.x == spiderPos.x && playerPos.y == spiderPos.y) {
                        // Enter combat with spider
                        combat::enter_combat_mode(*state.player, tutorialSpider, *state.dungeon, log);
                        
                        // Spider applies poison on first hit (handled in combat)
                        // Check if player has poison status after combat
                        if (state.player->has_status(StatusType::Poison)) {
                            state.statusEffectReceived = true;
                            log.add(MessageType::Warning, "You are poisoned! Notice the status icon above your HP bar.");
                        } else {
                            // If not poisoned yet, mark as received anyway for tutorial flow
                            // (spider will apply poison in combat)
                            state.statusEffectReceived = true;
                        }
                    }
                    
                    if (section_status_effects(state)) {
                        complete_tutorial_section(
                            state, lastSection,
                            "✓ Status Effects section complete!",
                            "💡 Next: Learn about environmental hazards. Interact with traps, shrines, and water!",
                            4, 13,  // Room 9 (Hazards)
                            ""
                        );
                        continue;
                    }
                    break;
                }
                    
                case 13: { // Hazards
                    // Movement to interact with hazards
                    Position playerPos = state.player->get_position();
                    
                    if (key == 'w' || key == 'W' || key == input::KEY_UP) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y - 1)) {
                            state.player->set_position(pos.x, pos.y - 1);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 's' || key == 'S' || key == input::KEY_DOWN) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x, pos.y + 1)) {
                            state.player->set_position(pos.x, pos.y + 1);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 'a' || key == 'A' || key == input::KEY_LEFT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x - 1, pos.y)) {
                            state.player->set_position(pos.x - 1, pos.y);
                            playerPos = state.player->get_position();
                        }
                    } else if (key == 'd' || key == 'D' || key == input::KEY_RIGHT) {
                        Position pos = state.player->get_position();
                        if (state.dungeon->is_walkable(pos.x + 1, pos.y)) {
                            state.player->set_position(pos.x + 1, pos.y);
                            playerPos = state.player->get_position();
                        }
                    }
                    
                    // Check for trap (at 4, 13) - updated position
                    if (playerPos.x == 4 && playerPos.y == 13 && state.dungeon->get_tile(4, 13) == TileType::Trap) {
                        if (!state.trapTriggered) {
                            // Reduced damage in tutorial
                            state.player->get_stats().hp -= 2;
                            state.trapTriggered = true;
                            state.dungeon->set_tile(4, 13, TileType::Floor);
                            log.add(MessageType::Damage, "You triggered a trap! (-2 HP)");
                        }
                    }
                    
                    // Check for shrine (at 5, 13) - interact with E - updated position
                    if (key == 'e' || key == 'E') {
                        if (playerPos.x == 5 && playerPos.y == 13 && state.dungeon->get_tile(5, 13) == TileType::Shrine) {
                            if (!state.shrineInteracted) {
                                // Always heal in tutorial
                                int healAmt = 10;
                                state.player->get_stats().hp = std::min(state.player->get_stats().hp + healAmt, state.player->get_stats().maxHp);
                                state.shrineInteracted = true;
                                state.dungeon->set_tile(5, 6, TileType::Floor);
                                log.add(MessageType::Heal, "The shrine heals you! (+" + std::to_string(healAmt) + " HP)");
                            }
                        }
                    }
                    
                    // Check for water (at 6, 13) - updated position
                    if (playerPos.x == 6 && playerPos.y == 13 && state.dungeon->get_tile(6, 13) == TileType::Water) {
                        if (!state.waterTraversed) {
                            state.waterTraversed = true;
                            log.add(MessageType::Info, "You wade through the water...");
                        }
                    }
                    
                    if (section_hazards(state)) {
                        complete_tutorial_section(
                            state, lastSection,
                            "✓ Hazards section complete!",
                            "💡 Next: Learn about descending stairs. Stand on > and press > to descend!",
                            11, 7,  // Room 10 (Stairs)
                            ""
                        );
                        continue;
                    }
                    break;
                }
                    
                case 14: { // Stairs
                    Position pos = state.player->get_position();
                    if (state.dungeon->get_tile(pos.x, pos.y) == TileType::StairsDown) {
                        state.standingOnStairs = true;
                        if (key == '>') {
                            state.stairsPressed = true;
                        }
                    }
                    
                    if (section_stairs(state)) {
                        // Tutorial complete!
                        show_tutorial_completion_screen();
                        tutorialRunning = false;
        return true;
                    }
                    break;
                }
            }
            
            // Handle quit
            if (key == 'q' || key == 'Q') {
                if (currentView == UIView::MAP) {
                    tutorialRunning = false;
                    return false;
                } else {
                    currentView = UIView::MAP;
                }
            }
        }
        
        return true;
    }
    
    // Keep the old combat tutorial for compatibility
    bool run_combat_tutorial() {
        // ... (keep existing implementation)
        return true;
    }
}
