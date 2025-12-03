#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include "types.h"
#include "constants.h"
#include "glyphs.h"

// Forward declarations
class Player;
class Enemy;


/**
 * @enum MessageType
 * @brief Types of categorized log entries for the message log system.
 *
 * Used to prefix messages with icons and colors for different gameplay events.
 */
enum class MessageType {
    Info,       /**< General information (●) */
    Combat,     /**< Combat actions (⚔) */
    Damage,     /**< Damage taken (✗) */
    Heal,       /**< Healing (♥) */
    Warning,    /**< Warnings/traps (⚠) */
    Loot,       /**< Item pickup (★) */
    Level,      /**< Level/floor changes (▲) */
    Death,      /**< Death messages (☠) */
    Debug       /**< Debug messages (◇) */
};


/**
 * @class MessageLog
 * @brief Stores and renders categorized log messages for the UI.
 *
 * Provides methods to add, clear, and render messages with optional framing and message types.
 */
class MessageLog {
public:
    /**
     * @brief Add a plain message to the log (no prefix).
     * @param line The message string to add.
     */
    void add(const std::string& line);

    /**
     * @brief Add a categorized message with an auto-prefix icon.
     * @param type The MessageType category.
     * @param line The message string to add.
     */
    void add(MessageType type, const std::string& line);

    /**
     * @brief Clear all messages from the log.
     */
    void clear();

    /**
     * @brief Render the message log at a given position.
     * @param row Row to start rendering.
     * @param col Column to start rendering.
     * @param maxLines Maximum number of lines to display.
     */
    void render(int row, int col, int maxLines) const;

    /**
     * @brief Render the message log with a frame.
     * @param row Row to start rendering.
     * @param col Column to start rendering.
     * @param width Width of the frame.
     * @param maxLines Maximum number of lines to display.
     */
    void render_framed(int row, int col, int width, int maxLines) const;
private:
    std::vector<std::string> lines_{}; /**< Stored log lines */
};


/**
 * @namespace ui
 * @brief Terminal UI rendering and effects for Rogue Depths.
 *
 * Contains functions for drawing UI elements, handling color, sound, and screen transitions.
 */
namespace ui {
    /** @brief Initialize the UI system. */
    bool init();

    /** @brief Shutdown and cleanup the UI system. */
    void shutdown();

    /** @brief Clear the terminal screen. */
    void clear();

    /**
     * @brief Move the cursor to a specific row and column.
     * @param row Row to move to.
     * @param col Column to move to.
     */
    void move_cursor(int row, int col);

    /**
     * @brief Set the terminal color using an ANSI code.
     * @param code ANSI color code string.
     */
    void set_color(const std::string& code);

    /** @brief Reset terminal color to default. */
    void reset_color();

    /**
     * @brief Fill a rectangular area with spaces.
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     */
    void fill_rect(int startRow, int startCol, int width, int height);

    /**
     * @brief Draw text at a specific position.
     * @param row Row to draw at.
     * @param col Column to draw at.
     * @param text Text to display.
     */
    void draw_text(int row, int col, const std::string& text);

    /**
     * @brief Draw a double-lined box frame.
     * @param row Top row.
     * @param col Left column.
     * @param width Box width.
     * @param height Box height.
     * @param color Frame color.
     */
    void draw_box_double(int row, int col, int width, int height, const std::string& color);

    /**
     * @brief Draw a single-lined box frame.
     * @param row Top row.
     * @param col Left column.
     * @param width Box width.
     * @param height Box height.
     * @param color Frame color.
     */
    void draw_box_single(int row, int col, int width, int height, const std::string& color);

    /**
     * @brief Draw a double horizontal line.
     * @param row Row to draw at.
     * @param col Starting column.
     * @param width Line width.
     * @param color Line color.
     */
    void draw_horizontal_line_double(int row, int col, int width, const std::string& color);

    /**
     * @brief Draw the player status bar.
     * @param row Row to draw at.
     * @param player Player reference.
     * @param depth Current dungeon depth.
     */
    void draw_status_bar(int row, const Player& player, int depth = 1);

    /**
     * @brief Draw a framed player status bar.
     * @param row Row to draw at.
     * @param col Column to draw at.
     * @param width Frame width.
     * @param player Player reference.
     * @param depth Current dungeon depth.
     */
    void draw_status_bar_framed(int row, int col, int width, const Player& player, int depth);

    /**
     * @brief Draw the inventory panel.
     * @param row Row to draw at.
     * @param col Column to draw at.
     * @param player Player reference.
     * @param selectedIndex Index of selected item.
     */
    void draw_inventory_panel(int row, int col, const Player& player, int selectedIndex);

    /**
     * @brief Draw a framed inventory panel.
     * @param row Row to draw at.
     * @param col Column to draw at.
     * @param width Frame width.
     * @param player Player reference.
     * @param selectedIndex Index of selected item.
     */
    void draw_inventory_panel_framed(int row, int col, int width, const Player& player, int selectedIndex);

    /**
     * @brief Draw inventory items for all inventory UIs.
     * @param row Row to start drawing.
     * @param col Column to start drawing.
     * @param width Width of the item area.
     * @param player Player reference.
     * @param selectedIndex Index of selected item.
     * @param maxItems Maximum items to display (-1 for all).
     * @param scrollOffset Scroll offset for long lists.
     * @param showStats Show item stats if true.
     * @param compact Use compact display if true.
     */
    void draw_inventory_items(int row, int col, int width, const Player& player, int selectedIndex, int maxItems = -1, int scrollOffset = 0, bool showStats = false, bool compact = false);

    /**
     * @brief Draw the help screen for a given page.
     * @param page Help page index.
     */
    void draw_help_screen(int page);

    /** @brief Number of help pages available. */
    constexpr int help_page_count = 4;

    /**
     * @brief Draw the full inventory view (tab view).
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param width View width.
     * @param height View height.
     * @param player Player reference.
     * @param selectedIndex Index of selected item.
     * @param scrollOffset Scroll offset for long lists.
     */
    void draw_full_inventory_view(int startRow, int startCol, int width, int height, 
                                   const Player& player, int selectedIndex, int scrollOffset);

    /**
     * @brief Draw the player stats view (tab view).
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param width View width.
     * @param height View height.
     * @param player Player reference.
     * @param depth Current dungeon depth.
     * @param killCount Number of enemies killed.
     */
    void draw_stats_view(int startRow, int startCol, int width, int height, 
                         const Player& player, int depth, int killCount);

    /**
     * @brief Draw the equipment view (tab view).
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param width View width.
     * @param height View height.
     * @param player Player reference.
     */
    void draw_equipment_view(int startRow, int startCol, int width, int height, 
                             const Player& player);

    /**
     * @brief Draw the message log view (tab view).
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param width View width.
     * @param height View height.
     * @param log MessageLog reference.
     * @param scrollOffset Scroll offset for long logs.
     */
    void draw_message_log_view(int startRow, int startCol, int width, int height,
                               const MessageLog& log, int scrollOffset);

    /**
     * @brief Draw the combat arena visualization (3D positioning display).
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param width Arena width.
     * @param playerPos Player 3D position.
     * @param enemyPos Enemy 3D position.
     * @param currentDistance Current combat distance.
     * @param arena Optional pointer to CombatArena for extra info.
     */
    void draw_combat_arena(int startRow, int startCol, int width,
                          const Position3D& playerPos, const Position3D& enemyPos,
                          CombatDistance currentDistance, const CombatArena* arena = nullptr);

    /**
     * @brief Draw Pokemon-style combat viewport showing player and enemy sprites.
     * @param startRow Starting row for viewport.
     * @param startCol Starting column for viewport.
     * @param width Viewport width.
     * @param height Viewport height.
     * @param player Player reference.
     * @param enemy Enemy reference.
     * @param distance Current combat distance.
     */
    void draw_combat_viewport(int startRow, int startCol, int width, int height,
                              const Player& player, const Enemy& enemy, CombatDistance distance);

    /**
     * @brief Draw a combat sprite at specified position.
     * @param row Starting row.
     * @param col Starting column.
     * @param sprite Multi-line sprite string.
     * @param color Color code for sprite.
     */
    void draw_combat_sprite(int row, int col, const std::string& sprite, const std::string& color);

    /**
     * @brief Draw HP bars in Pokemon style.
     * @param row Starting row.
     * @param col Starting column.
     * @param player Player reference.
     * @param enemy Enemy reference.
     */
    void draw_combat_hp_bars(int row, int col, const Player& player, const Enemy& enemy);

    /**
     * @brief Draw combat status info (HP bars and status effects).
     * @param row Starting row.
     * @param col Starting column.
     * @param player Player reference.
     * @param enemy Enemy reference.
     */
    void draw_combat_status_info(int row, int col, const Player& player, const Enemy& enemy);

    /**
     * @brief Get sprite string for player class.
     * @param pclass Player class.
     * @return Sprite string (multi-line).
     */
    std::string get_player_sprite(PlayerClass pclass);

    /**
     * @brief Get sprite string for enemy.
     * @param enemy Enemy reference.
     * @return Sprite string (multi-line).
     */
    std::string get_enemy_sprite(const Enemy& enemy);

    /**
     * @brief Animate sprite attack (slide animation).
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param sprite Sprite string to animate.
     * @param color Color code.
     * @param isPlayer True if player attacking (slides right), false if enemy (slides left).
     */
    void animate_sprite_attack(int startRow, int startCol, const std::string& sprite, 
                               const std::string& color, bool isPlayer);

    /**
     * @brief Animate sprite shake effect (for taking damage).
     * @param baseRow Base row position.
     * @param baseCol Base column position.
     * @param sprite Sprite string.
     * @param color Color code.
     * @param intensity Shake intensity (pixels).
     * @param duration Duration in milliseconds.
     */
    void animate_sprite_shake(int baseRow, int baseCol, const std::string& sprite,
                              const std::string& color, int intensity, int duration);

    /**
     * @brief Calculate sprite dimensions from sprite string.
     * @param sprite Sprite string (multi-line).
     * @return Pair of (height, width) in rows and columns.
     */
    std::pair<int, int> calculate_sprite_dimensions(const std::string& sprite);

    /**
     * @brief Animate projectile flying from source to target.
     * @param fromRow Source row.
     * @param fromCol Source column.
     * @param toRow Target row.
     * @param toCol Target column.
     * @param projectile Character to use as projectile.
     * @param color Color code.
     */
    void animate_projectile(int fromRow, int fromCol, int toRow, int toCol, 
                           const std::string& projectile, const std::string& color);

    /**
     * @brief Animate explosion at target position.
     * @param row Explosion row.
     * @param col Explosion column.
     * @param color Color code.
     */
    void animate_explosion(int row, int col, const std::string& color);

    /**
     * @brief Animate rogue slide attack (slide to target and back).
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param targetCol Target column (to slide to).
     * @param sprite Sprite string.
     * @param color Color code.
     */
    void animate_rogue_slide(int startRow, int startCol, int targetCol,
                             const std::string& sprite, const std::string& color);

    /**
     * @brief Animate warrior charge attack with weapon slash.
     * @param startRow Starting row.
     * @param startCol Starting column.
     * @param targetCol Target column (to charge to).
     * @param sprite Sprite string.
     * @param color Color code.
     */
    void animate_warrior_charge(int startRow, int startCol, int targetCol,
                                const std::string& sprite, const std::string& color);

    /**
     * @brief Add a damage number to display in combat viewport.
     * @param damage Damage value to display.
     * @param spriteRow Row of the sprite taking damage.
     * @param spriteCol Column of the sprite taking damage.
     * @param isPlayer True if player took damage, false if enemy.
     * @param isCritical True if this is a critical hit.
     */
    void add_damage_number(int damage, int spriteRow, int spriteCol, bool isPlayer, bool isCritical = false);

    /**
     * @brief Get the display name for a UI view.
     * @param view The UIView enum value.
     * @return Name string for the view.
     */
    const char* view_name(UIView view);

    /** @brief Flash the screen red when the player takes damage. */
    void flash_damage();

    /** @brief Flash the screen green when the player heals. */
    void flash_heal();

    /** @brief Flash the screen yellow for critical hits. */
    void flash_critical();

    /** @brief Flash the screen yellow/orange for warnings. */
    void flash_warning();

    /** @brief Play a bell sound for a combat hit. */
    void play_hit_sound();

    /** @brief Play a double bell for a critical hit. */
    void play_critical_sound();

    /** @brief Play a bell pattern for death. */
    void play_death_sound();

    /** @brief Play a bell pattern for victory. */
    void play_victory_sound();

    /** @brief Play a bell for level/floor progression. */
    void play_level_up_sound();

    /**
     * @brief Wipe the screen from top to bottom.
     * @param steps Number of wipe steps.
     */
    void wipe_transition_down(int steps = 4);

    /**
     * @brief Wipe the screen from bottom to top.
     * @param steps Number of wipe steps.
     */
    void wipe_transition_up(int steps = 4);

    /**
     * @brief Fade out the screen.
     * @param steps Number of fade steps.
     */
    void fade_transition(int steps = 3);
}


