#include "ui.h"
#include "glyphs.h"
#include "input.h"
#include "constants.h"
#include "player.h"
#include "enemy.h"
#include "types.h"
#include "logger.h"

#include <iostream>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <thread>
#include <sstream>
#include <random>
#include <fstream>
#include <deque>
#include <algorithm>
#include <map>
#include <deque>

void MessageLog::add(const std::string& line) {
    lines_.push_back(line);
    // Keep max 100 messages
    if (lines_.size() > 100) {
        lines_.erase(lines_.begin());
    }
}

void MessageLog::add(MessageType type, const std::string& line) {
    std::string prefix;
    std::string colorCode;
    
    switch (type) {
        case MessageType::Info:
            prefix = glyphs::msg_info();
            colorCode = constants::color_msg_info;
            break;
        case MessageType::Combat:
            prefix = glyphs::msg_combat();
            colorCode = constants::color_msg_combat;
            break;
        case MessageType::Damage:
            prefix = glyphs::msg_damage();
            colorCode = constants::color_msg_damage;
            break;
        case MessageType::Heal:
            prefix = glyphs::msg_heal();
            colorCode = constants::color_msg_heal;
            break;
        case MessageType::Warning:
            prefix = glyphs::msg_warning();
            colorCode = constants::color_msg_warning;
            break;
        case MessageType::Loot:
            prefix = glyphs::msg_loot();
            colorCode = constants::color_msg_loot;
            break;
        case MessageType::Level:
            prefix = glyphs::msg_level();
            colorCode = constants::color_msg_level;
            break;
        case MessageType::Death:
            prefix = glyphs::msg_death();
            colorCode = constants::color_msg_death;
            break;
        case MessageType::Debug:
            prefix = glyphs::msg_debug();
            colorCode = constants::color_msg_info;
            break;
    }
    
    // IMPROVED: Optimize string building - reserve capacity and use append to avoid multiple allocations
    std::string fullMessage;
    if (glyphs::use_color) {
        // Reserve space for color code + prefix + line + reset (estimate ~50 chars overhead)
        fullMessage.reserve(colorCode.size() + prefix.size() + line.size() + constants::ansi_reset.size());
        fullMessage = colorCode;
        fullMessage += prefix;
        fullMessage += line;
        fullMessage += constants::ansi_reset;
    } else {
        // Reserve space for prefix + line
        fullMessage.reserve(prefix.size() + line.size());
        fullMessage = prefix;
        fullMessage += line;
    }
    
    add(fullMessage);
}

void MessageLog::clear() {
    lines_.clear();
}

void MessageLog::render(int row, int col, int maxLines) const {
    // PHASE 3: Check output stream health before operations
    if (!std::cout.good()) {
        LOG_WARN("MessageLog::render: std::cout is in bad state - attempting to clear");
        std::cout.clear();
        if (!std::cout.good()) {
            LOG_ERROR("MessageLog::render: Failed to recover std::cout state - aborting render");
            return;
        }
    }
    
    int start = 0;
    if (static_cast<int>(lines_.size()) > maxLines) {
        start = static_cast<int>(lines_.size()) - maxLines;
    }
    for (int i = start; i < static_cast<int>(lines_.size()); ++i) {
        // PHASE 3: Check output stream health periodically during message rendering
        if (!std::cout.good()) {
            LOG_WARN("MessageLog::render: std::cout bad state during message rendering - clearing");
            std::cout.clear();
            if (!std::cout.good()) {
                LOG_ERROR("MessageLog::render: Failed to recover during message render - stopping");
                break; // Stop rendering messages but don't crash
            }
        }
        
        ui::move_cursor(row + (i - start), col);
        // Don't set color here - messages already have color codes embedded
        // This preserves the different colors for combat, info, damage, etc.
        std::cout << lines_[i];
        // Reset color after each message to prevent color bleeding
        ui::reset_color();
    }
}

void MessageLog::render_framed(int row, int col, int width, int maxLines) const {
    // PHASE 3: Check output stream health before operations
    if (!std::cout.good()) {
        LOG_WARN("MessageLog::render_framed: std::cout is in bad state - attempting to clear");
        std::cout.clear();
        if (!std::cout.good()) {
            LOG_ERROR("MessageLog::render_framed: Failed to recover std::cout state - aborting render");
            return;
        }
    }
    
    // PHASE 1: Validate parameters
    if (width < 2) {
        LOG_WARN("MessageLog::render_framed: Invalid width (" + std::to_string(width) + ") - skipping render");
        return;
    }
    if (width > 1000) {
        LOG_WARN("MessageLog::render_framed: Width too large (" + std::to_string(width) + ") - clamping to 1000");
        width = 1000;
    }
    
    // Draw frame
    ui::draw_box_single(row, col, width, maxLines + 2, constants::color_frame_message);
    
    // PHASE 3: Check output stream health after frame drawing
    if (!std::cout.good()) {
        LOG_WARN("MessageLog::render_framed: std::cout bad state after draw_box_single - clearing");
        std::cout.clear();
        if (!std::cout.good()) {
            LOG_ERROR("MessageLog::render_framed: Failed to recover after frame - aborting");
            return;
        }
    }
    
    // Draw title
    ui::move_cursor(row, col + 2);
    ui::set_color(constants::color_frame_message);
    std::cout << " Messages ";
    ui::reset_color();
    
    // Draw messages inside frame
    int start = 0;
    if (static_cast<int>(lines_.size()) > maxLines) {
        start = static_cast<int>(lines_.size()) - maxLines;
    }
    for (int i = start; i < static_cast<int>(lines_.size()); ++i) {
        // PHASE 3: Check output stream health periodically during message rendering
        if (!std::cout.good()) {
            LOG_WARN("MessageLog::render_framed: std::cout bad state during message rendering - clearing");
            std::cout.clear();
            if (!std::cout.good()) {
                LOG_ERROR("MessageLog::render_framed: Failed to recover during message render - stopping");
                break; // Stop rendering messages but don't crash
            }
        }
        
        ui::move_cursor(row + 1 + (i - start), col + 1);
        // Don't set color here - messages already have color codes embedded
        // This preserves the different colors for combat, info, damage, etc.
        // Truncate if too long
        std::string msg = lines_[i];
        if (static_cast<int>(msg.size()) > width - 2) {
            msg = msg.substr(0, width - 5) + "...";
        }
        std::cout << msg;
        // Reset color after each message to prevent color bleeding
        ui::reset_color();
    }
}

namespace ui {
    bool init() {
        std::cout << "\033[?25l";
        clear();
        return true;
    }

    void shutdown() {
        reset_color();
        std::cout << "\033[?25h";
        std::cout.flush();
    }

    void clear() {
        // PHASE 3: Check output stream health before clearing
        if (!std::cout.good()) {
            LOG_WARN("ui::clear: std::cout is in bad state - attempting to clear");
            std::cout.clear();
            if (!std::cout.good()) {
                LOG_ERROR("ui::clear: Failed to recover std::cout state - aborting clear");
                return;
            }
        }
        std::cout << "\033[2J\033[H";
        std::cout.flush();
        
        // PHASE 3: Verify clear succeeded
        if (!std::cout.good()) {
            LOG_WARN("ui::clear: std::cout bad state after clear operation");
            std::cout.clear();
        }
    }

    void move_cursor(int row, int col) {
        std::cout << "\033[" << row << ";" << col << "H";
    }

    void set_color(const std::string& code) {
        // Only output color if colors are enabled
        if (glyphs::use_color) {
            std::cout << code;
        }
    }

    void reset_color() {
        if (glyphs::use_color) {
            std::cout << constants::ansi_reset;
        }
    }

    // Fill a rectangular area with spaces (to clear background for overlays)
    void fill_rect(int startRow, int startCol, int width, int height) {
        std::string spaces(width, ' ');
        for (int r = 0; r < height; ++r) {
            move_cursor(startRow + r, startCol);
            std::cout << spaces;
        }
    }

    void draw_text(int row, int col, const std::string& text) {
        move_cursor(row, col);
        std::cout << text;
    }

    // Draw a double-line box frame (uses glyphs for Unicode/ASCII fallback)
    void draw_box_double(int row, int col, int width, int height, const std::string& color) {
        set_color(color);
        
        // Top border
        move_cursor(row, col);
        std::cout << glyphs::box_dbl_tl();
        for (int i = 0; i < width - 2; ++i) std::cout << glyphs::box_dbl_h();
        std::cout << glyphs::box_dbl_tr();
        
        // Side borders
        for (int r = 1; r < height - 1; ++r) {
            move_cursor(row + r, col);
            std::cout << glyphs::box_dbl_v();
            move_cursor(row + r, col + width - 1);
            std::cout << glyphs::box_dbl_v();
        }
        
        // Bottom border
        move_cursor(row + height - 1, col);
        std::cout << glyphs::box_dbl_bl();
        for (int i = 0; i < width - 2; ++i) std::cout << glyphs::box_dbl_h();
        std::cout << glyphs::box_dbl_br();
        
        reset_color();
    }

    // Draw a single-line box frame (uses glyphs for Unicode/ASCII fallback)
    void draw_box_single(int row, int col, int width, int height, const std::string& color) {
        // PHASE 1: Input validation - prevent crashes from invalid parameters
        if (width < 2 || height < 1) {
            LOG_WARN("draw_box_single: Invalid dimensions (width=" + std::to_string(width) + 
                     ", height=" + std::to_string(height) + ") - skipping");
            return;
        }
        if (width > 1000 || height > 1000) {
            LOG_WARN("draw_box_single: Dimensions too large (width=" + std::to_string(width) + 
                     ", height=" + std::to_string(height) + ") - skipping");
            return;
        }
        
        // PHASE 3: Check output stream health before operations
        if (!std::cout.good()) {
            LOG_WARN("draw_box_single: std::cout is in bad state - attempting to clear");
            std::cout.clear();
            if (!std::cout.good()) {
                LOG_ERROR("draw_box_single: Failed to recover std::cout state - aborting");
                return;
            }
        }
        
        set_color(color);
        
        // Top border
        move_cursor(row, col);
        std::cout << glyphs::box_sgl_tl();
        for (int i = 0; i < width - 2; ++i) std::cout << glyphs::box_sgl_h();
        std::cout << glyphs::box_sgl_tr();
        
        // Side borders
        for (int r = 1; r < height - 1; ++r) {
            move_cursor(row + r, col);
            std::cout << glyphs::box_sgl_v();
            move_cursor(row + r, col + width - 1);
            std::cout << glyphs::box_sgl_v();
        }
        
        // Bottom border
        move_cursor(row + height - 1, col);
        std::cout << glyphs::box_sgl_bl();
        for (int i = 0; i < width - 2; ++i) std::cout << glyphs::box_sgl_h();
        std::cout << glyphs::box_sgl_br();
        
        reset_color();
    }

    void draw_horizontal_line_double(int row, int col, int width, const std::string& color) {
        set_color(color);
        move_cursor(row, col);
        std::cout << glyphs::box_dbl_lt();
        for (int i = 0; i < width - 2; ++i) std::cout << glyphs::box_dbl_h();
        std::cout << glyphs::box_dbl_rt();
        reset_color();
    }

    void draw_status_bar(int row, const Player& player, int depth) {
        move_cursor(row, 1);
        set_color(constants::ansi_bold);
        std::cout << Player::class_name(player.player_class()) << "  ";
        std::cout << "Depth " << depth << "/10  ";
        std::cout << "HP " << player.get_stats().hp << "/" << player.get_stats().maxHp << "  ";
        reset_color();
        std::cout << "ATK " << player.get_stats().attack << "  DEF " << player.get_stats().defense << "  ";
        std::cout << "SPD " << player.get_stats().speed << "  ";
        std::cout << "[";
        bool first = true;
        for (const auto& s : player.statuses()) {
            if (!first) std::cout << " ";
            first = false;
            switch (s.type) {
                case StatusType::Bleed: std::cout << "BLD(" << s.remainingTurns << ")"; break;
                case StatusType::Poison: std::cout << "PSN(" << s.remainingTurns << ")"; break;
                case StatusType::Fortify: std::cout << "FOR(" << s.remainingTurns << ")"; break;
                case StatusType::Haste: std::cout << "HST(" << s.remainingTurns << ")"; break;
                case StatusType::Burn: std::cout << "BRN(" << s.remainingTurns << ")"; break;
                case StatusType::Freeze: std::cout << "FRZ(" << s.remainingTurns << ")"; break;
                case StatusType::Stun: std::cout << "STN(" << s.remainingTurns << ")"; break;
                default: break;
            }
        }
        std::cout << "]";
    }

    void draw_status_bar_framed(int row, int col, int width, const Player& player, int depth) {
        LOG_OP_START("draw_status_bar_framed_internal");
        
        // PHASE 3: Log parameters for debugging
        LOG_DEBUG("draw_status_bar_framed: row=" + std::to_string(row) + 
                  ", col=" + std::to_string(col) + ", width=" + std::to_string(width));
        
        // PHASE 1: Validate width parameter before calling draw_box_single
        if (width < 2) {
            LOG_WARN("draw_status_bar_framed: Invalid width (" + std::to_string(width) + 
                     ") - using minimum width of 2");
            width = 2;
        }
        if (width > 1000) {
            LOG_WARN("draw_status_bar_framed: Width too large (" + std::to_string(width) + 
                     ") - using maximum width of 1000");
            width = 1000;
        }
        
        // PHASE 3: Check output stream health before operations
        if (!std::cout.good()) {
            LOG_WARN("draw_status_bar_framed: std::cout is in bad state - attempting to clear");
            std::cout.clear();
            if (!std::cout.good()) {
                LOG_ERROR("draw_status_bar_framed: Failed to recover std::cout state - aborting");
                LOG_OP_END("draw_status_bar_framed_internal");
                return;
            }
        }
        
        // Draw frame
        LOG_OP_START("draw_box_single_status");
        draw_box_single(row, col, width, 3, constants::color_frame_status);
        LOG_OP_END("draw_box_single_status");
        // PHASE 2: Removed intermediate flush - rely on main loop flush
        
        // PHASE 3: Check output stream health before title
        if (!std::cout.good()) {
            LOG_WARN("draw_status_bar_framed: std::cout bad state after draw_box_single - clearing");
            std::cout.clear();
        }
        
        // Draw title
        LOG_OP_START("draw_status_title");
        move_cursor(row, col + 2);
        set_color(constants::color_frame_status);
        std::cout << " Status ";
        reset_color();
        // PHASE 2: Removed intermediate flush
        LOG_OP_END("draw_status_title");
        
        // Draw status content
        LOG_OP_START("draw_status_content");
        move_cursor(row + 1, col + 2);
        set_color(constants::ansi_bold);
        set_color(constants::color_player);
        std::cout << Player::class_name(player.player_class());
        reset_color();
        std::cout << " ";
        set_color(constants::color_ui);
        std::cout << "D:" << depth << "/10 ";
        // PHASE 2: Removed intermediate flush
        LOG_OP_END("draw_status_content");
        
        // HP with heart icon and color based on health
        LOG_OP_START("draw_status_hp");
        int hpPercent = (player.get_stats().maxHp > 0) ? 
            (player.get_stats().hp * 100 / player.get_stats().maxHp) : 0;
        if (hpPercent > 60) set_color("\033[38;5;46m");       // Green
        else if (hpPercent > 30) set_color("\033[38;5;226m"); // Yellow
        else set_color("\033[38;5;196m");                     // Red
        std::cout << glyphs::stat_hp() << ":" << player.get_stats().hp << "/" << player.get_stats().maxHp;
        reset_color();
        // PHASE 2: Removed intermediate flush
        LOG_OP_END("draw_status_hp");
        
        // Mana system removed
        
        // Attack icon
        LOG_OP_START("draw_status_stats");
        std::cout << " " << glyphs::stat_attack() << ":" << player.get_stats().attack;
        // Defense icon
        std::cout << " " << glyphs::stat_defense() << ":" << player.get_stats().defense;
        // Speed icon
        std::cout << " " << glyphs::stat_speed() << ":" << player.get_stats().speed;
        // PHASE 2: Removed intermediate flush
        LOG_OP_END("draw_status_stats");
        
        // Status effects with icons
        LOG_OP_START("draw_status_effects");
        if (!player.statuses().empty()) {
            std::cout << " [";
            bool first = true;
            // PHASE 1: Add safety limit for status effects loop (max 20 statuses)
            constexpr int MAX_STATUS_EFFECTS = 20;
            int statusCount = 0;
            for (const auto& s : player.statuses()) {
                if (statusCount >= MAX_STATUS_EFFECTS) {
                    LOG_WARN("draw_status_bar_framed: Too many status effects (" + 
                             std::to_string(player.statuses().size()) + ") - truncating at " + 
                             std::to_string(MAX_STATUS_EFFECTS));
                    break;
                }
                statusCount++;
                
                if (!first) std::cout << " ";
                first = false;
                switch (s.type) {
                    case StatusType::Bleed: 
                        set_color(constants::color_status_bleed);
                        std::cout << glyphs::status_bleed(); 
                        break;
                    case StatusType::Poison: 
                        set_color(constants::color_status_poison);
                        std::cout << glyphs::status_poison(); 
                        break;
                    case StatusType::Fortify: 
                        set_color(constants::color_status_fortify);
                        std::cout << glyphs::status_fortify(); 
                        break;
                    case StatusType::Haste: 
                        set_color(constants::color_status_haste);
                        std::cout << glyphs::status_haste(); 
                        break;
                    case StatusType::Burn:
                        set_color(constants::color_status_burn);
                        std::cout << glyphs::status_fire();
                        break;
                    case StatusType::Freeze:
                        set_color(constants::color_status_freeze);
                        std::cout << glyphs::status_ice();
                        break;
                    case StatusType::Stun:
                        set_color(constants::color_status_stun);
                        std::cout << glyphs::status_stun();
                        break;
                    default: break;
                }
                reset_color();
                std::cout << "(" << s.remainingTurns << ")";
            }
            std::cout << "]";
            // PHASE 2: Removed intermediate flush
        }
        LOG_OP_END("draw_status_effects");
        
        // PHASE 2: Single flush at end of function (main loop also flushes, but this ensures completion)
        // PHASE 3: Check output stream health before final flush
        if (!std::cout.good()) {
            LOG_WARN("draw_status_bar_framed: std::cout bad state before final flush - clearing");
            std::cout.clear();
        }
        std::cout.flush();
        
        LOG_OP_END("draw_status_bar_framed_internal");
    }

    void draw_inventory_items(int row, int col, int width, const Player& player, int selectedIndex, int maxItems, int scrollOffset, bool showStats, bool compact) {
        const auto& inv = const_cast<Player&>(player).inventory();
        int count = (maxItems > 0) ? std::min(static_cast<int>(inv.size()) - scrollOffset, maxItems) : static_cast<int>(inv.size()) - scrollOffset;
        int irow = row;
        for (int i = 0; i < count; ++i) {
            int idx = scrollOffset + i;
            move_cursor(irow++, col);
            if (static_cast<int>(idx) == selectedIndex) {
                set_color(constants::ansi_bold);
                std::cout << "> ";
            } else {
                std::cout << "  ";
            }
            // Color by rarity
            switch (inv[idx].rarity) {
                case Rarity::Common:    set_color(constants::color_item_common); break;
                case Rarity::Uncommon:  set_color(constants::color_item_uncommon); break;
                case Rarity::Rare:      set_color(constants::color_item_rare); break;
                case Rarity::Epic:      set_color(constants::color_item_epic); break;
                case Rarity::Legendary: set_color(constants::color_item_legendary); break;
            }
            std::string name = inv[idx].name;
            if (compact && name.length() > 18) name = name.substr(0, 15) + "...";
            std::cout << (idx + 1) << ". " << name;
            reset_color();
            if (inv[idx].isEquippable) {
                set_color("\033[38;5;226m");
                std::cout << " [E]";
            }
            if (inv[idx].isConsumable) {
                set_color("\033[38;5;46m");
                std::cout << " [U]";
            }
            reset_color();
            if (showStats) {
                if (inv[idx].attackBonus > 0) std::cout << " +ATK:" << inv[idx].attackBonus;
                if (inv[idx].defenseBonus > 0) std::cout << " +DEF:" << inv[idx].defenseBonus;
                if (inv[idx].healAmount > 0) std::cout << " +HP:" << inv[idx].healAmount;
            }
        }
        if (inv.empty()) {
            move_cursor(row, col + 2);
            set_color(constants::color_floor);
            std::cout << "(empty)";
            reset_color();
        }
    }

    void draw_inventory_panel(int row, int col, const Player& player, int selectedIndex) {
        move_cursor(row, col);
        set_color(constants::ansi_bold);
        std::cout << "Inventory (i: close, e: equip, u: use, d: drop)";
        reset_color();
        draw_inventory_items(row + 1, col, 0, player, selectedIndex, -1, 0, false, true);
    }

    void draw_inventory_panel_framed(int row, int col, int width, const Player& player, int selectedIndex) {
        const auto& inv = const_cast<Player&>(player).inventory();
        int height = std::max(5, static_cast<int>(inv.size()) + 3);
        // Draw frame
        draw_box_single(row, col, width, height, constants::color_frame_inventory);
        // Draw title
        move_cursor(row, col + 2);
        set_color(constants::color_frame_inventory);
        std::cout << " Inventory (i:close e:equip u:use d:drop) ";
        reset_color();
        // Draw items
        draw_inventory_items(row + 1, col + 1, width - 2, player, selectedIndex, -1, 0, false, true);
    }

    void draw_help_screen(int page) {
        clear();
        int width = 60;
        int height = 25;
        int startRow = 2;
        int startCol = 10;
        
        // Draw main frame
        draw_box_double(startRow, startCol, width, height, constants::color_frame_main);
        
        // Draw title
        move_cursor(startRow, startCol + 2);
        set_color(constants::color_frame_main);
        std::cout << " HELP - Page " << (page + 1) << "/" << help_page_count << " ";
        reset_color();
        
        int r = startRow + 2;
        
        switch (page) {
            case 0: // Controls
                move_cursor(r++, startCol + 2);
                set_color(constants::ansi_bold);
                std::cout << "=== CONTROLS ===";
                reset_color();
                r++;
                move_cursor(r++, startCol + 2);
                std::cout << "Movement:    WASD or Arrow Keys";
                move_cursor(r++, startCol + 2);
                std::cout << "Descend:     > (on stairs)";
                move_cursor(r++, startCol + 2);
                std::cout << "Inventory:   I (toggle)";
                move_cursor(r++, startCol + 2);
                std::cout << "Equip:       E (in inventory)";
                move_cursor(r++, startCol + 2);
                std::cout << "Use item:    U (in inventory)";
                move_cursor(r++, startCol + 2);
                std::cout << "Drop:        D (in inventory)";
                move_cursor(r++, startCol + 2);
                std::cout << "Help:        ? (this screen)";
                move_cursor(r++, startCol + 2);
                std::cout << "Quit:        Q";
                r++;
                move_cursor(r++, startCol + 2);
                set_color(constants::color_floor);
                std::cout << "Debug: R=reset, G=spawn items, N=spawn enemy";
                reset_color();
                break;
                
            case 1: // Symbols
                move_cursor(r++, startCol + 2);
                set_color(constants::ansi_bold);
                std::cout << "=== SYMBOLS ===";
                reset_color();
                r++;
                move_cursor(r++, startCol + 2);
                set_color(constants::color_player);
                std::cout << glyphs::player();
                reset_color();
                std::cout << " Player        ";
                set_color(constants::color_wall);
                std::cout << glyphs::wall();
                reset_color();
                std::cout << " Wall";
                
                move_cursor(r++, startCol + 2);
                set_color(constants::color_floor);
                std::cout << glyphs::floor_tile();
                reset_color();
                std::cout << " Floor         ";
                std::cout << glyphs::door_closed();
                std::cout << " Door";
                
                move_cursor(r++, startCol + 2);
                set_color(constants::color_stairs);
                std::cout << glyphs::stairs_down();
                reset_color();
                std::cout << " Stairs Down   ";
                set_color(constants::color_trap);
                std::cout << glyphs::trap();
                reset_color();
                std::cout << " Trap";
                
                move_cursor(r++, startCol + 2);
                set_color(constants::color_shrine);
                std::cout << glyphs::shrine();
                reset_color();
                std::cout << " Shrine";
                r++;
                
                move_cursor(r++, startCol + 2);
                set_color(constants::ansi_bold);
                std::cout << "Monsters:";
                reset_color();
                move_cursor(r++, startCol + 2);
                set_color(constants::color_monster_weak);
                std::cout << "r";
                reset_color();
                std::cout << "at ";
                set_color(constants::color_monster_weak);
                std::cout << "s";
                reset_color();
                std::cout << "pider ";
                set_color(constants::color_monster_common);
                std::cout << "g";
                reset_color();
                std::cout << "oblin ";
                set_color(constants::color_monster_common);
                std::cout << "k";
                reset_color();
                std::cout << "obold ";
                set_color(constants::color_monster_strong);
                std::cout << "o";
                reset_color();
                std::cout << "rc ";
                set_color(constants::color_monster_strong);
                std::cout << "z";
                reset_color();
                std::cout << "ombie";
                
                move_cursor(r++, startCol + 2);
                set_color(constants::color_monster_elite);
                std::cout << "G";
                reset_color();
                std::cout << "nome ";
                set_color(constants::color_monster_elite);
                std::cout << "O";
                reset_color();
                std::cout << "gre ";
                set_color(constants::color_monster_elite);
                std::cout << "T";
                reset_color();
                std::cout << "roll ";
                set_color(constants::color_monster_boss);
                std::cout << "D";
                reset_color();
                std::cout << "ragon ";
                set_color(constants::color_monster_boss);
                std::cout << "L";
                reset_color();
                std::cout << "ich";
                break;
                
            case 2: // Classes
                move_cursor(r++, startCol + 2);
                set_color(constants::ansi_bold);
                std::cout << "=== PLAYER CLASSES ===";
                reset_color();
                r++;
                move_cursor(r++, startCol + 2);
                set_color("\033[38;5;196m");
                std::cout << "WARRIOR";
                reset_color();
                std::cout << " - Tough fighter";
                move_cursor(r++, startCol + 4);
                std::cout << "+3 HP, +1 ATK";
                move_cursor(r++, startCol + 4);
                set_color(constants::color_floor);
                std::cout << "Best for: Learning the game, tanking hits";
                reset_color();
                r++;
                
                // Rogue hidden from class selection
                
                move_cursor(r++, startCol + 2);
                set_color("\033[38;5;33m");
                std::cout << "MAGE";
                reset_color();
                std::cout << " - Defensive caster";
                move_cursor(r++, startCol + 4);
                std::cout << "-1 HP, +2 DEF";
                move_cursor(r++, startCol + 4);
                set_color(constants::color_floor);
                std::cout << "Best for: Careful play, attrition";
                reset_color();
                break;
                
            case 3: // Tips
                move_cursor(r++, startCol + 2);
                set_color(constants::ansi_bold);
                std::cout << "=== TIPS ===";
                reset_color();
                r++;
                move_cursor(r++, startCol + 2);
                std::cout << "* Equip weapons and armor for stat boosts";
                move_cursor(r++, startCol + 2);
                std::cout << "* Use potions before tough fights";
                move_cursor(r++, startCol + 2);
                std::cout << "* Shrines (_) heal or buff you";
                move_cursor(r++, startCol + 2);
                std::cout << "* Avoid traps (^) - they hurt!";
                move_cursor(r++, startCol + 2);
                std::cout << "* Enemies get stronger on deeper floors";
                move_cursor(r++, startCol + 2);
                std::cout << "* Reach floor 10 to win!";
                r++;
                move_cursor(r++, startCol + 2);
                set_color(constants::color_corpse);
                std::cout << "* If you die, your corpse spawns a";
                move_cursor(r++, startCol + 2);
                std::cout << "  vengeful spirit (C) on your next run!";
                reset_color();
                break;
        }
        
        // Navigation hint
        move_cursor(startRow + height - 2, startCol + 2);
        set_color(constants::color_floor);
        std::cout << "Arrow Keys: Change Page | Any other key: Close";
        reset_color();
        
        std::cout.flush();
    }

    const char* view_name(UIView view) {
        switch (view) {
            case UIView::MAP: return "MAP";
            case UIView::INVENTORY: return "INVENTORY";
            case UIView::STATS: return "STATS";
            case UIView::EQUIPMENT: return "EQUIPMENT";
            case UIView::MESSAGE_LOG: return "MESSAGES";
            default: return "UNKNOWN";
        }
    }

    void draw_full_inventory_view(int startRow, int startCol, int width, int height,
                                   const Player& player, int selectedIndex, int scrollOffset) {
        // Clear background first (prevent map showing through)
        fill_rect(startRow, startCol, width, height);
        // Draw main frame
        draw_box_double(startRow, startCol, width, height, constants::color_frame_inventory);
        // Draw title
        move_cursor(startRow, startCol + 2);
        set_color(constants::color_frame_inventory);
        std::cout << " INVENTORY [TAB: next view | ESC: map] ";
        reset_color();
        // Column headers
        int r = startRow + 2;
        move_cursor(r++, startCol + 2);
        set_color(constants::ansi_bold);
        std::cout << "# ";
        std::cout.width(20);
        std::cout << std::left << "Name";
        std::cout.width(10);
        std::cout << "Type";
        std::cout.width(10);
        std::cout << "Rarity";
        std::cout << "Stats";
        reset_color();
        move_cursor(r++, startCol + 2);
        for (int i = 0; i < width - 4; i++) std::cout << "-";
        // Draw items with scrolling
        int contentHeight = height - 6;
        draw_inventory_items(r, startCol + 2, width - 4, player, selectedIndex, contentHeight, scrollOffset, true, true);
        // Footer with controls
        move_cursor(startRow + height - 2, startCol + 2);
        set_color(constants::color_floor);
        std::cout << "W/S: Navigate | E: Equip | U: Use | D: Drop";
        reset_color();
    }

    void draw_stats_view(int startRow, int startCol, int width, int height,
                         const Player& player, int depth, int killCount) {
        // Clear background first (prevent map showing through)
        fill_rect(startRow, startCol, width, height);
        
        // Draw main frame
        draw_box_double(startRow, startCol, width, height, constants::color_frame_status);
        
        // Draw title
        move_cursor(startRow, startCol + 2);
        set_color(constants::color_frame_status);
        std::cout << " CHARACTER STATS [TAB: next view | ESC: map] ";
        reset_color();
        
        int r = startRow + 2;
        int col = startCol + 3;
        
        // Class and level
        move_cursor(r++, col);
        set_color(constants::ansi_bold);
        set_color(constants::color_player);
        std::cout << "Class: " << Player::class_name(player.player_class());
        reset_color();
        
        r++;
        
        // Primary stats
        move_cursor(r++, col);
        set_color(constants::ansi_bold);
        std::cout << "=== PRIMARY STATS ===";
        reset_color();
        
        move_cursor(r++, col);
        int hpPercent = (player.get_stats().maxHp > 0) ? 
            (player.get_stats().hp * 100 / player.get_stats().maxHp) : 0;
        if (hpPercent > 60) set_color("\033[38;5;46m");
        else if (hpPercent > 30) set_color("\033[38;5;226m");
        else set_color("\033[38;5;196m");
        std::cout << glyphs::stat_hp() << " Health: " << player.get_stats().hp << " / " << player.get_stats().maxHp;
        reset_color();
        
        // HP bar with hearts
        std::cout << "  [";
        int barWidth = 10;  // Using hearts takes more space
        int filled = (barWidth * hpPercent) / 100;
        for (int i = 0; i < barWidth; i++) {
            if (i < filled) {
                if (hpPercent > 60) set_color("\033[38;5;46m");
                else if (hpPercent > 30) set_color("\033[38;5;226m");
                else set_color("\033[38;5;196m");
                std::cout << glyphs::heart_full();
            } else {
                set_color(constants::color_floor);
                std::cout << glyphs::heart_empty();
            }
        }
        reset_color();
        std::cout << "]";
        
        move_cursor(r++, col);
        std::cout << glyphs::stat_attack() << " Attack:  " << player.get_stats().attack;
        
        move_cursor(r++, col);
        std::cout << glyphs::stat_defense() << " Defense: " << player.get_stats().defense;
        
        move_cursor(r++, col);
        std::cout << glyphs::stat_speed() << " Speed:   " << player.get_stats().speed;
        
        r++;
        
        // Progress
        move_cursor(r++, col);
        set_color(constants::ansi_bold);
        std::cout << "=== PROGRESS ===";
        reset_color();
        
        move_cursor(r++, col);
        std::cout << "Current Floor: " << depth << " / 10";
        
        move_cursor(r++, col);
        std::cout << "Enemies Slain: " << killCount;
        
        r++;
        
        // Status effects
        move_cursor(r++, col);
        set_color(constants::ansi_bold);
        std::cout << "=== STATUS EFFECTS ===";
        reset_color();
        
        if (player.statuses().empty()) {
            move_cursor(r++, col);
            set_color(constants::color_floor);
            std::cout << "(none active)";
            reset_color();
        } else {
            for (const auto& status : player.statuses()) {
                move_cursor(r++, col);
                switch (status.type) {
                    case StatusType::Bleed:
                        set_color(constants::color_status_bleed);
                        std::cout << glyphs::status_bleed() << " BLEEDING";
                        break;
                    case StatusType::Poison:
                        set_color(constants::color_status_poison);
                        std::cout << glyphs::status_poison() << " POISONED";
                        break;
                    case StatusType::Fortify:
                        set_color(constants::color_status_fortify);
                        std::cout << glyphs::status_fortify() << " FORTIFIED";
                        break;
                    case StatusType::Haste:
                        set_color(constants::color_status_haste);
                        std::cout << glyphs::status_haste() << " HASTE";
                        break;
                    case StatusType::Burn:
                        set_color(constants::color_status_burn);
                        std::cout << glyphs::status_fire() << " BURNING";
                        break;
                    case StatusType::Freeze:
                        set_color(constants::color_status_freeze);
                        std::cout << glyphs::status_ice() << " FROZEN";
                        break;
                    case StatusType::Stun:
                        set_color(constants::color_status_stun);
                        std::cout << glyphs::status_stun() << " STUNNED";
                        break;
                    default: break;
                }
                reset_color();
                std::cout << " (" << status.remainingTurns << " turns)";
            }
        }
        
        // Footer
        move_cursor(startRow + height - 2, startCol + 2);
        set_color(constants::color_floor);
        std::cout << "TAB: Next View | ESC: Return to Map";
        reset_color();
    }

    void draw_equipment_view(int startRow, int startCol, int width, int height,
                             const Player& player) {
        // Clear background first (prevent map showing through)
        fill_rect(startRow, startCol, width, height);
        
        // Draw main frame
        draw_box_double(startRow, startCol, width, height, constants::color_frame_inventory);
        
        // Draw title
        move_cursor(startRow, startCol + 2);
        set_color(constants::color_frame_inventory);
        std::cout << " EQUIPMENT [TAB: next view | ESC: map] ";
        reset_color();
        
        int r = startRow + 2;
        int col = startCol + 3;
        
        // ASCII art character with equipment slots
        move_cursor(r++, col);
        set_color(constants::ansi_bold);
        std::cout << "=== EQUIPPED ITEMS ===";
        reset_color();
        r++;
        
        const auto& equipment = player.get_equipment();
        
        // Equipment slots visualization
        move_cursor(r++, col);
        std::cout << "     [HEAD]     ";
        
        move_cursor(r++, col);
        std::cout << "       O        ";  // Head
        
        move_cursor(r++, col);
        std::cout << " [WPN]/|\\[OFF] ";  // Arms
        
        move_cursor(r++, col);
        std::cout << "     [CHEST]    ";  // Body
        
        move_cursor(r++, col);
        std::cout << "      / \\       ";  // Legs
        
        move_cursor(r++, col);
        std::cout << "    [ACC]       ";  // Accessory
        
        r += 2;
        
        // List equipped items
        move_cursor(r++, col);
        set_color(constants::ansi_bold);
        std::cout << "=== SLOT DETAILS ===";
        reset_color();
        r++;
        
        auto printSlot = [&](const char* slotName, EquipmentSlot slot) {
            move_cursor(r++, col);
            std::cout << slotName << ": ";
            auto it = equipment.find(slot);
            if (it != equipment.end()) {
                // Color by rarity
                switch (it->second.rarity) {
                    case Rarity::Common:    set_color(constants::color_item_common); break;
                    case Rarity::Uncommon:  set_color(constants::color_item_uncommon); break;
                    case Rarity::Rare:      set_color(constants::color_item_rare); break;
                    case Rarity::Epic:      set_color(constants::color_item_epic); break;
                    case Rarity::Legendary: set_color(constants::color_item_legendary); break;
                }
                std::cout << it->second.name;
                reset_color();
                if (it->second.attackBonus > 0) std::cout << " (+ATK:" << it->second.attackBonus << ")";
                if (it->second.defenseBonus > 0) std::cout << " (+DEF:" << it->second.defenseBonus << ")";
            } else {
                set_color(constants::color_floor);
                std::cout << "(empty)";
                reset_color();
            }
        };
        
        printSlot("Head    ", EquipmentSlot::Head);
        printSlot("Chest   ", EquipmentSlot::Chest);
        printSlot("Weapon  ", EquipmentSlot::Weapon);
        printSlot("Offhand ", EquipmentSlot::Offhand);
        printSlot("Accessory", EquipmentSlot::Accessory);
        
        // Footer
        move_cursor(startRow + height - 2, startCol + 2);
        set_color(constants::color_floor);
        std::cout << "TAB: Next View | ESC: Return to Map";
        reset_color();
    }

    void draw_message_log_view(int startRow, int startCol, int width, int height,
                               const MessageLog& log, int scrollOffset) {
        // Clear background first (prevent map showing through)
        fill_rect(startRow, startCol, width, height);
        
        // Draw main frame
        draw_box_double(startRow, startCol, width, height, constants::color_frame_message);
        
        // Draw title
        move_cursor(startRow, startCol + 2);
        set_color(constants::color_frame_message);
        std::cout << " MESSAGE LOG [TAB: next view | ESC: map] ";
        reset_color();
        
        // Access internal lines (we need to make MessageLog friend or add getter)
        // For now, use the render method with offset
        int contentHeight = height - 3;
        
        move_cursor(startRow + 2, startCol + 2);
        set_color(constants::color_floor);
        std::cout << "Recent messages (newest at bottom):";
        reset_color();
        
        // Render messages inside frame
        // Note: This uses the existing render method but positioned inside our frame
        log.render(startRow + 3, startCol + 2, contentHeight - 2);
        
        // Footer with scroll hint
        move_cursor(startRow + height - 2, startCol + 2);
        set_color(constants::color_floor);
        std::cout << "W/S: Scroll | TAB: Next View | ESC: Return to Map";
        reset_color();
        
        // Suppress unused parameter warning
        (void)scrollOffset;
    }

    // ============================================
    // Combat Arena Visualization
    // ============================================
    
    void draw_combat_arena(int startRow, int startCol, int width,
                          const Position3D& playerPos, const Position3D& enemyPos,
                          CombatDistance currentDistance, const CombatArena* arena) {
        using namespace combat_balance;
        
        // Draw frame
        draw_box_single(startRow, startCol, width, 12, constants::color_frame_main);
        
        // Title
        move_cursor(startRow, startCol + 2);
        set_color(constants::color_frame_main);
        std::cout << " " << glyphs::msg_combat() << " COMBAT ARENA ";
        reset_color();
        
        // Start damage / accuracy / hazard info a few rows from top so HP/status can be drawn inside arena
        int row = startRow + 7;
        
        // Damage modifier info (no depth/coordinates/zone text)
        move_cursor(row++, startCol + 2);
        float damageMod = 0.0f;
        switch (currentDistance) {
            case CombatDistance::MELEE: damageMod = DAMAGE_MELEE; break;
            case CombatDistance::CLOSE: damageMod = DAMAGE_CLOSE; break;
            case CombatDistance::MEDIUM: damageMod = DAMAGE_MEDIUM; break;
            case CombatDistance::FAR: damageMod = DAMAGE_FAR; break;
            case CombatDistance::EXTREME: damageMod = DAMAGE_EXTREME; break;
        }
        std::cout << "Damage Modifier: " << (int)(damageMod * 100) << "%";
        
        // Accuracy info
        move_cursor(row++, startCol + 2);
        int accuracy = 0;
        switch (currentDistance) {
            case CombatDistance::MELEE: accuracy = ACCURACY_MELEE; break;
            case CombatDistance::CLOSE: accuracy = ACCURACY_CLOSE; break;
            case CombatDistance::MEDIUM: accuracy = ACCURACY_MEDIUM; break;
            case CombatDistance::FAR: accuracy = ACCURACY_FAR; break;
            case CombatDistance::EXTREME: accuracy = ACCURACY_EXTREME; break;
        }
        std::cout << "Hit Chance: " << accuracy << "%";
        
        // Show hazards if arena provided
        if (arena && !arena->hazards.empty()) {
            row++;
            move_cursor(row++, startCol + 2);
            std::cout << "Hazards: ";
            for (size_t i = 0; i < arena->hazards.size() && i < 5; ++i) {
                const auto& pos = arena->hazardPositions[i];
                std::string hazardGlyph = "?";
                switch (arena->hazards[i]) {
                    case CombatHazard::SPIKE_FLOOR: hazardGlyph = glyphs::trap(); break;
                    case CombatHazard::FIRE_PILLAR: hazardGlyph = glyphs::fire(); break;
                    case CombatHazard::ICE_PATCH: hazardGlyph = glyphs::ice(); break;
                    case CombatHazard::POISON_CLOUD: hazardGlyph = std::string(glyphs::status_poison()); break;
                    case CombatHazard::HEALING_SPRING: hazardGlyph = glyphs::potion(); break;
                    default: break;
                }
                std::cout << hazardGlyph << "(" << pos.x << "," << pos.y << "," << pos.depth << ") ";
            }
        }
    }
    
    // ============================================
    // Visual Feedback Effects
    // ============================================
    
    void flash_damage() {
        if (!glyphs::use_color) return;  // Skip if colors disabled
        
        // Flash red background briefly
        std::cout << "\033[41m" << std::flush;  // Red background
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        std::cout << "\033[0m" << std::flush;   // Reset
    }
    
    void flash_heal() {
        if (!glyphs::use_color) return;
        
        // Flash green background briefly
        std::cout << "\033[42m" << std::flush;  // Green background
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        std::cout << "\033[0m" << std::flush;
    }
    
    void flash_critical() {
        if (!glyphs::use_color) return;
        
        // Flash yellow background briefly
        std::cout << "\033[43m" << std::flush;  // Yellow background
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        std::cout << "\033[0m" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        std::cout << "\033[43m" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        std::cout << "\033[0m" << std::flush;
    }
    
    void flash_warning() {
        if (!glyphs::use_color) return;
        
        // Flash orange/yellow background for warnings (telegraphed attacks)
        std::cout << "\033[43m" << std::flush;  // Yellow background
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "\033[0m" << std::flush;
    }
    
    // ============================================
    // Sound Effects (Terminal Bell)
    // ============================================
    
    void play_hit_sound() {
        std::cout << '\a' << std::flush;  // Single bell
    }
    
    void play_critical_sound() {
        std::cout << '\a' << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << '\a' << std::flush;
    }
    
    void play_death_sound() {
        // Slow, mournful pattern
        for (int i = 0; i < 3; i++) {
            std::cout << '\a' << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
    
    void play_victory_sound() {
        // Triumphant ascending pattern
        for (int i = 0; i < 5; i++) {
            std::cout << '\a' << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }
    
    void play_level_up_sound() {
        // Quick double bell
        std::cout << '\a' << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        std::cout << '\a' << std::flush;
    }
    
    // ============================================
    // Screen Transitions
    // ============================================
    
    void wipe_transition_down(int steps) {
        auto termSize = input::get_terminal_size();
        int h = termSize.height;
        int w = termSize.width;
        
        std::string blankLine(w, ' ');
        int rowsPerStep = h / steps;
        
        for (int s = 0; s < steps; s++) {
            for (int y = 0; y < rowsPerStep; y++) {
                int row = s * rowsPerStep + y;
                if (row < h) {
                    move_cursor(row + 1, 1);
                    std::cout << blankLine;
                }
            }
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(90));
        }
    }
    
    void wipe_transition_up(int steps) {
        auto termSize = input::get_terminal_size();
        int h = termSize.height;
        int w = termSize.width;
        
        std::string blankLine(w, ' ');
        int rowsPerStep = h / steps;
        
        for (int s = steps - 1; s >= 0; s--) {
            for (int y = 0; y < rowsPerStep; y++) {
                int row = s * rowsPerStep + y;
                if (row < h) {
                    move_cursor(row + 1, 1);
                    std::cout << blankLine;
                }
            }
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(90));
        }
    }
    
    void fade_transition(int steps) {
        // Simulate fade by progressively dimming colors
        if (!glyphs::use_color) {
            clear();
            return;
        }
        
        // Use ANSI dim attribute for fade effect
        for (int s = 0; s < steps; s++) {
            std::cout << "\033[2m";  // Dim
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        clear();
        std::cout << "\033[0m";  // Reset
    }

    // ============================================
    // Pokemon-Style Combat Viewport
    // ============================================
    
    // Damage number display system
    struct DamageNumber {
        int value;
        int row;
        int col;
        std::chrono::steady_clock::time_point startTime;
        bool isPlayer; // true if player took damage, false if enemy
        bool isCritical;
        
        DamageNumber(int v, int r, int c, bool player, bool crit = false)
            : value(v), row(r), col(c), isPlayer(player), isCritical(crit) {
            startTime = std::chrono::steady_clock::now();
        }
        
        bool isExpired() const {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
            return elapsed.count() > 1000; // Show for 1 second
        }
        
        int getOffset() const {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
            // Move up 1 row per 200ms, max 3 rows
            return std::min(3, static_cast<int>(elapsed.count() / 200));
        }
    };
    
    // Global damage numbers queue (simple implementation)
    static std::deque<DamageNumber> damageNumbers;
    
    // Helper: Add damage number to display (implementation)
    void add_damage_number(int damage, int spriteRow, int spriteCol, bool isPlayer, bool isCritical) {
        // Center the damage number above the sprite
        int displayCol = spriteCol + 8; // Approximate center of sprite
        damageNumbers.emplace_back(damage, spriteRow - 1, displayCol, isPlayer, isCritical);
        
        // Keep only last 5 damage numbers
        if (damageNumbers.size() > 5) {
            damageNumbers.pop_front();
        }
    }
    
    // Helper: Draw damage numbers
    void draw_damage_numbers(int viewportRow, int /*viewportCol*/) {
        // Remove expired damage numbers
        damageNumbers.erase(
            std::remove_if(damageNumbers.begin(), damageNumbers.end(),
                [](const DamageNumber& dn) { return dn.isExpired(); }),
            damageNumbers.end()
        );
        
        // Draw active damage numbers
        for (const auto& dn : damageNumbers) {
            int offset = dn.getOffset();
            int drawRow = dn.row - offset;
            int drawCol = dn.col;
            
            // Only draw if within viewport bounds
            if (drawRow >= viewportRow && drawRow < viewportRow + 15) {
                move_cursor(drawRow, drawCol);
                
                // Choose color based on damage type
                if (dn.isCritical) {
                    set_color("\033[93m"); // Yellow for critical
                } else if (dn.isPlayer) {
                    set_color("\033[91m"); // Red for player damage
                } else {
                    set_color("\033[92m"); // Green for enemy damage
                }
                
                std::cout << "-" << dn.value;
                reset_color();
            }
        }
    }
    
    // Helper: Get status effect name and icon
    static std::pair<std::string, std::string> get_status_display(StatusType type) {
        switch (type) {
            case StatusType::Bleed:
                return {glyphs::status_bleed(), "BLEED"};
            case StatusType::Poison:
                return {glyphs::status_poison(), "POISON"};
            case StatusType::Fortify:
                return {glyphs::status_fortify(), "FORTIFY"};
            case StatusType::Haste:
                return {glyphs::status_haste(), "HASTE"};
            case StatusType::Burn:
                return {glyphs::status_fire(), "BURN"};
            case StatusType::Freeze:
                return {glyphs::status_ice(), "FREEZE"};
            case StatusType::Stun:
                return {glyphs::status_stun(), "STUN"};
            default:
                return {"", ""};
        }
    }
    
    // Helper: Get status effect color
    static std::string get_status_color(StatusType type) {
        switch (type) {
            case StatusType::Bleed:
                return constants::color_status_bleed;
            case StatusType::Poison:
                return constants::color_status_poison;
            case StatusType::Fortify:
                return constants::color_status_fortify;
            case StatusType::Haste:
                return constants::color_status_haste;
            case StatusType::Burn:
                return constants::color_status_burn;
            case StatusType::Freeze:
                return constants::color_status_freeze;
            case StatusType::Stun:
                return constants::color_status_stun;
            default:
                return constants::color_floor;
        }
    }
    
    // Helper: Load sprite from file with fallback
    static std::string load_sprite_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return ""; // File not found
        }
        
        std::string sprite;
        std::string line;
        bool firstLine = true;
        while (std::getline(file, line)) {
            if (!firstLine) {
                sprite += "\n";
            }
            sprite += line;
            firstLine = false;
        }
        return sprite;
    }
    
    // Helper: Get sprite for player class (loads from file, falls back to hardcoded)
    std::string get_player_sprite(PlayerClass pclass) {
        std::string sprite;
        
        // Try to load from file first
        switch (pclass) {
            case PlayerClass::Warrior:
                sprite = load_sprite_from_file("assets/ascii/combat/warrior.txt");
                if (!sprite.empty()) return sprite;
                // Fallback to hardcoded
                return "  /\\\n"
                       " |  |\n"
                       " |__|\n"
                       " ||||\n"
                       "  ||";
            case PlayerClass::Rogue:
                sprite = load_sprite_from_file("assets/ascii/combat/rogue.txt");
                if (!sprite.empty()) return sprite;
                // Fallback to hardcoded
                return "   /\\\n"
                       "  |  |\n"
                       "  |__|\n"
                       "  ||\n"
                       "  ||";
            case PlayerClass::Mage:
                sprite = load_sprite_from_file("assets/ascii/combat/mage.txt");
                if (!sprite.empty()) return sprite;
                // Fallback to hardcoded
                return "  /\\\n"
                       " |  |\n"
                       " |__|\n"
                       "  ||\n"
                       "  *";
            default:
                return "  @\n"
                       " /|\\\n"
                       " / \\";
        }
    }
    
    // Helper: Get sprite for enemy type (loads from file, falls back to glyph-based)
    std::string get_enemy_sprite(const Enemy& enemy) {
        // Try to load enemy sprites from file
        EnemyType etype = enemy.enemy_type();
        std::string sprite;
        
        switch (etype) {
            case EnemyType::Rat:
                sprite = load_sprite_from_file("assets/ascii/combat/rat.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Spider:
                sprite = load_sprite_from_file("assets/ascii/combat/spider.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Goblin:
                sprite = load_sprite_from_file("assets/ascii/combat/goblin.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Kobold:
                sprite = load_sprite_from_file("assets/ascii/combat/kobold.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Orc:
                sprite = load_sprite_from_file("assets/ascii/combat/orc.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Zombie:
                sprite = load_sprite_from_file("assets/ascii/combat/zombie.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Archer:
                sprite = load_sprite_from_file("assets/ascii/combat/archer.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Gnome:
                sprite = load_sprite_from_file("assets/ascii/combat/gnome.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Ogre:
                sprite = load_sprite_from_file("assets/ascii/combat/ogre.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Troll:
                sprite = load_sprite_from_file("assets/ascii/combat/troll.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Dragon:
                sprite = load_sprite_from_file("assets/ascii/combat/dragon.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::Lich:
                sprite = load_sprite_from_file("assets/ascii/combat/skeleton.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::StoneGolem:
                sprite = load_sprite_from_file("assets/ascii/combat/stonegolem.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::ShadowLord:
                sprite = load_sprite_from_file("assets/ascii/combat/shadowlord.txt");
                if (!sprite.empty()) return sprite;
                break;
            case EnemyType::CorpseEnemy:
                sprite = load_sprite_from_file("assets/ascii/combat/corpse.txt");
                if (!sprite.empty()) return sprite;
                break;
            default:
                break;
        }
        
        // Fallback to glyph-based sprite
        char glyph = enemy.glyph();
        switch (glyph) {
            case 'D': // Dragon
            case 'O': // Ogre
            case 'T': // Troll
                sprite = "  /\\\n"
                         " |  |\n"
                         " |__|\n"
                         " ||||\n"
                         "  ||";
                break;
            default:
                // Generic enemy sprite
                sprite = "  " + std::string(1, glyph) + "\n"
                         " /|\\\n"
                         " / \\";
        }
        return sprite;
    }
    
    void draw_combat_sprite(int row, int col, const std::string& sprite, const std::string& color) {
        std::istringstream iss(sprite);
        std::string line;
        int currentRow = row;
        set_color(color);
        while (std::getline(iss, line)) {
            move_cursor(currentRow++, col);
            std::cout << line;
        }
        reset_color();
    }
    
    void draw_combat_hp_bars(int row, int col, const Player& player, const Enemy& enemy) {
        // Player HP bar
        move_cursor(row, col);
        set_color(constants::color_player);
        std::cout << "Player HP: ";
        reset_color();
        
        const int barWidth = 20;
        const auto& pStats = player.get_stats();
        int playerFilled = (pStats.maxHp > 0) ? (pStats.hp * barWidth) / pStats.maxHp : 0;
        
        std::cout << glyphs::bar_left();
        for (int i = 0; i < barWidth; ++i) {
            if (i < playerFilled) {
                set_color("\033[92m"); // Green
                std::cout << glyphs::bar_full();
            } else {
                set_color("\033[90m"); // Dark gray
                std::cout << glyphs::bar_quarter();
            }
        }
        reset_color();
        std::cout << glyphs::bar_right() << " " << pStats.hp << "/" << pStats.maxHp;
        
        // Enemy HP bar
        move_cursor(row + 1, col);
        set_color("\033[91m"); // Red
        std::cout << enemy.name() << " HP: ";
        reset_color();
        
        const auto& eStats = enemy.stats();
        int enemyFilled = (eStats.maxHp > 0) ? (eStats.hp * barWidth) / eStats.maxHp : 0;
        
        std::cout << glyphs::bar_left();
        for (int i = 0; i < barWidth; ++i) {
            if (i < enemyFilled) {
                set_color("\033[91m"); // Red
                std::cout << glyphs::bar_full();
            } else {
                set_color("\033[90m"); // Dark gray
                std::cout << glyphs::bar_quarter();
            }
        }
        reset_color();
        std::cout << glyphs::bar_right() << " " << eStats.hp << "/" << eStats.maxHp;
    }
    
    void draw_combat_status_info(int row, int col, const Player& player, const Enemy& enemy) {
        // Draw HP bars
        draw_combat_hp_bars(row, col, player, enemy);
        
        // Draw status effects with icons and names
        int statusRow = row + 2;
        move_cursor(statusRow, col);
        set_color(constants::ansi_bold);
        std::cout << "Player Status:";
        reset_color();
        
        // Player statuses
        const auto& pStatuses = player.statuses();
        if (!pStatuses.empty()) {
            bool hasActive = false;
            for (const auto& status : pStatuses) {
                if (status.remainingTurns > 0) {
                    hasActive = true;
                    auto [icon, name] = get_status_display(status.type);
                    std::string color = get_status_color(status.type);
                    
                    std::cout << " ";
                    set_color(color);
                    std::cout << icon << " " << name << " " << status.remainingTurns;
                    reset_color();
                }
            }
            if (!hasActive) {
                set_color(constants::color_floor);
                std::cout << " (none)";
                reset_color();
            }
        } else {
            set_color(constants::color_floor);
            std::cout << " (none)";
            reset_color();
        }
        
        // Enemy statuses
        move_cursor(statusRow + 1, col);
        set_color(constants::ansi_bold);
        std::cout << enemy.name() << " Status:";
        reset_color();
        
        const auto& eStatuses = enemy.statuses();
        if (!eStatuses.empty()) {
            bool hasActive = false;
            for (const auto& status : eStatuses) {
                if (status.remainingTurns > 0) {
                    hasActive = true;
                    auto [icon, name] = get_status_display(status.type);
                    std::string color = get_status_color(status.type);
                    
                    std::cout << " ";
                    set_color(color);
                    std::cout << icon << " " << name << " " << status.remainingTurns;
                    reset_color();
                }
            }
            if (!hasActive) {
                set_color(constants::color_floor);
                std::cout << " (none)";
                reset_color();
            }
        } else {
            set_color(constants::color_floor);
            std::cout << " (none)";
            reset_color();
        }
    }
    
    void draw_combat_viewport(int startRow, int startCol, int width, int height,
                              const Player& player, const Enemy& enemy, CombatDistance distance) {
        // Draw frame
        draw_box_double(startRow, startCol, width, height, constants::color_frame_main);
        
        // Title
        move_cursor(startRow, startCol + 2);
        set_color(constants::color_frame_main);
        std::cout << " " << glyphs::msg_combat() << " BATTLE ";
        reset_color();
        
        // Calculate positions for sprites (left side for player, right side for enemy)
        // Position player at ~25% from left, enemy at ~65% from left for better balance
        int playerSpriteCol = startCol + (width / 4);  // ~25% from left
        int enemySpriteCol = startCol + (width * 2 / 3);  // ~67% from left
        
        // Get sprites
        std::string playerSprite = get_player_sprite(player.player_class());
        std::string enemySprite = get_enemy_sprite(enemy);
        
        // Calculate sprite dimensions
        auto playerDims = calculate_sprite_dimensions(playerSprite);
        auto enemyDims = calculate_sprite_dimensions(enemySprite);
        
        // Position player sprite from bottom of viewport (accounting for frame border)
        // Leave some space from bottom (2 rows for frame + 1 row padding)
        int playerSpriteRow = startRow + height - playerDims.first - 3;
        
        // Position enemy sprite from top of viewport (accounting for frame border)
        // Leave space for title (1 row) + 1 row padding
        int enemySpriteRow = startRow + 2;
        
        // Draw player sprite
        draw_combat_sprite(playerSpriteRow, playerSpriteCol, playerSprite, constants::color_player);
        
        // Draw enemy sprite
        std::string enemyColor = enemy.color();
        if (enemyColor.empty()) {
            enemyColor = "\033[91m"; // Default red
        }
        draw_combat_sprite(enemySpriteRow, enemySpriteCol, enemySprite, enemyColor);
        
        // Draw damage numbers (only visual feedback, HP/status moved to bottom right)
        draw_damage_numbers(startRow, startCol);
        
        // Note: HP bars and status effects are now drawn in bottom right (below arena)
    }
    
    void animate_sprite_attack(int startRow, int startCol, const std::string& sprite,
                               const std::string& color, bool isPlayer) {
        const int frames = 3;
        const int frameDelay = 100; // milliseconds
        
        int currentCol = startCol;
        int step = isPlayer ? 1 : -1;
        
        // Slide forward
        for (int i = 0; i < frames; ++i) {
            // Clear previous position
            std::istringstream iss(sprite);
            std::string line;
            int row = startRow;
            while (std::getline(iss, line)) {
                move_cursor(row++, currentCol);
                for (size_t j = 0; j < line.length() + 2; ++j) {
                    std::cout << " ";
                }
            }
            
            currentCol += step;
            draw_combat_sprite(startRow, currentCol, sprite, color);
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
        }
        
        // Return to original position
        for (int i = 0; i < frames; ++i) {
            std::istringstream iss(sprite);
            std::string line;
            int row = startRow;
            while (std::getline(iss, line)) {
                move_cursor(row++, currentCol);
                for (size_t j = 0; j < line.length() + 2; ++j) {
                    std::cout << " ";
                }
            }
            
            currentCol -= step;
            draw_combat_sprite(startRow, currentCol, sprite, color);
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
        }
    }
    
    void animate_sprite_shake(int baseRow, int baseCol, const std::string& sprite,
                              const std::string& color, int intensity, int duration) {
        const int shakeFrames = duration / 50; // 50ms per frame
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> offsetDist(-intensity, intensity);
        
        for (int i = 0; i < shakeFrames; ++i) {
            // Clear sprite
            std::istringstream iss(sprite);
            std::string line;
            int row = baseRow;
            while (std::getline(iss, line)) {
                move_cursor(row++, baseCol);
                for (size_t j = 0; j < line.length() + 2; ++j) {
                    std::cout << " ";
                }
            }
            
            // Draw at offset position
            int offsetCol = baseCol + offsetDist(rng);
            int offsetRow = baseRow + offsetDist(rng);
            draw_combat_sprite(offsetRow, offsetCol, sprite, color);
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Return to base position
        std::istringstream iss(sprite);
        std::string line;
        int row = baseRow;
        while (std::getline(iss, line)) {
            move_cursor(row++, baseCol);
            for (size_t j = 0; j < line.length() + 2; ++j) {
                std::cout << " ";
            }
        }
        draw_combat_sprite(baseRow, baseCol, sprite, color);
        std::cout.flush();
    }
    
    std::pair<int, int> calculate_sprite_dimensions(const std::string& sprite) {
        std::istringstream iss(sprite);
        std::string line;
        int height = 0;
        int maxWidth = 0;
        while (std::getline(iss, line)) {
            height++;
            int lineWidth = static_cast<int>(line.length());
            if (lineWidth > maxWidth) {
                maxWidth = lineWidth;
            }
        }
        return std::make_pair(height, maxWidth);
    }
    
    void animate_projectile(int fromRow, int fromCol, int toRow, int toCol,
                           const std::string& projectile, const std::string& color) {
        const int steps = 15;
        const int frameDelay = 30; // milliseconds
        
        int currentRow = fromRow;
        int currentCol = fromCol;
        int rowStep = (toRow - fromRow) / steps;
        int colStep = (toCol - fromCol) / steps;
        
        for (int i = 0; i < steps; ++i) {
            // Clear previous position
            move_cursor(currentRow, currentCol);
            std::cout << " ";
            
            currentRow += rowStep;
            currentCol += colStep;
            
            // Draw projectile
            move_cursor(currentRow, currentCol);
            set_color(color);
            std::cout << projectile;
            reset_color();
            std::cout.flush();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
        }
        
        // Clear final position
        move_cursor(currentRow, currentCol);
        std::cout << " ";
        std::cout.flush();
    }
    
    void animate_explosion(int row, int col, const std::string& color) {
        const std::vector<std::string> explosionFrames = {
            " * ",
            "***",
            " * ",
            " * "
        };
        
        set_color(color);
        for (size_t i = 0; i < explosionFrames.size(); ++i) {
            // Clear previous frame
            if (i > 0) {
                move_cursor(row - 1, col - 1);
                std::cout << "   ";
                move_cursor(row, col - 1);
                std::cout << "   ";
                move_cursor(row + 1, col - 1);
                std::cout << "   ";
            }
            
            // Draw explosion frame
            move_cursor(row - 1, col - 1);
            std::cout << explosionFrames[i];
            std::cout.flush();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
        reset_color();
        
        // Clear explosion
        move_cursor(row - 1, col - 1);
        std::cout << "   ";
        move_cursor(row, col - 1);
        std::cout << "   ";
        move_cursor(row + 1, col - 1);
        std::cout << "   ";
        std::cout.flush();
    }
    
    void animate_rogue_slide(int startRow, int startCol, int targetCol,
                            const std::string& sprite, const std::string& color) {
        const int frames = 8;
        const int frameDelay = 60; // milliseconds
        
        int currentCol = startCol;
        int step = (targetCol - startCol) / frames;
        if (step == 0) step = (targetCol > startCol) ? 1 : -1;
        
        // Slide forward to target
        for (int i = 0; i < frames; ++i) {
            // Clear previous position
            std::istringstream iss(sprite);
            std::string line;
            int row = startRow;
            while (std::getline(iss, line)) {
                move_cursor(row++, currentCol);
                for (size_t j = 0; j < line.length() + 2; ++j) {
                    std::cout << " ";
                }
            }
            
            currentCol += step;
            if ((step > 0 && currentCol >= targetCol) || (step < 0 && currentCol <= targetCol)) {
                currentCol = targetCol;
            }
            
            draw_combat_sprite(startRow, currentCol, sprite, color);
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
            
            if (currentCol == targetCol) break;
        }
        
        // Brief pause at target
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Slide back to original position
        for (int i = 0; i < frames; ++i) {
            // Clear previous position
            std::istringstream iss(sprite);
            std::string line;
            int row = startRow;
            while (std::getline(iss, line)) {
                move_cursor(row++, currentCol);
                for (size_t j = 0; j < line.length() + 2; ++j) {
                    std::cout << " ";
                }
            }
            
            currentCol -= step;
            if ((step > 0 && currentCol <= startCol) || (step < 0 && currentCol >= startCol)) {
                currentCol = startCol;
            }
            
            draw_combat_sprite(startRow, currentCol, sprite, color);
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
            
            if (currentCol == startCol) break;
        }
    }
    
    void animate_warrior_charge(int startRow, int startCol, int targetCol,
                               const std::string& sprite, const std::string& color) {
        const int frames = 6;
        const int frameDelay = 70; // milliseconds
        
        int currentCol = startCol;
        int step = (targetCol - startCol) / frames;
        if (step == 0) step = (targetCol > startCol) ? 1 : -1;
        
        // Charge forward to target
        for (int i = 0; i < frames; ++i) {
            // Clear previous position
            std::istringstream iss(sprite);
            std::string line;
            int row = startRow;
            while (std::getline(iss, line)) {
                move_cursor(row++, currentCol);
                for (size_t j = 0; j < line.length() + 2; ++j) {
                    std::cout << " ";
                }
            }
            
            currentCol += step;
            if ((step > 0 && currentCol >= targetCol) || (step < 0 && currentCol <= targetCol)) {
                currentCol = targetCol;
            }
            
            draw_combat_sprite(startRow, currentCol, sprite, color);
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
            
            if (currentCol == targetCol) break;
        }
        
        // Weapon slash effect at target
        move_cursor(startRow, targetCol + 10);
        set_color("\033[93m"); // Yellow for slash
        std::cout << "╲╱";
        reset_color();
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        
        // Clear slash
        move_cursor(startRow, targetCol + 10);
        std::cout << "  ";
        std::cout.flush();
        
        // Charge back to original position
        for (int i = 0; i < frames; ++i) {
            // Clear previous position
            std::istringstream iss(sprite);
            std::string line;
            int row = startRow;
            while (std::getline(iss, line)) {
                move_cursor(row++, currentCol);
                for (size_t j = 0; j < line.length() + 2; ++j) {
                    std::cout << " ";
                }
            }
            
            currentCol -= step;
            if ((step > 0 && currentCol <= startCol) || (step < 0 && currentCol >= startCol)) {
                currentCol = startCol;
            }
            
            draw_combat_sprite(startRow, currentCol, sprite, color);
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
            
            if (currentCol == startCol) break;
        }
    }
}

