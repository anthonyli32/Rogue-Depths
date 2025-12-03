#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cmath>

#include "constants.h"
#include "types.h"
#include "globals.h"
#include "ui.h"
#include "input.h"
#include "dungeon.h"
#include "player.h"
#include "enemy.h"
#include "ai.h"
#include "combat.h"
#include "fileio.h"
#include "cli.h"
#include "logger.h"
#include "glyphs.h"
#include "keybinds.h"
#include "shrine.h"
#include "traps.h"
#include "loot.h"
#include "leaderboard.h"
#include "tutorial.h"
#include "viewport.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
// Undefine Windows macros that conflict with our code
#ifdef ERROR
#undef ERROR
#endif
#ifdef FAR
#undef FAR
#endif
#endif

// Delete all save files and signal restart
static bool delete_all_saves() {
    bool deleted = false;
    // IMPROVED: Use named constants instead of magic numbers
    for (int slot = game_constants::MIN_SAVE_SLOT; slot <= game_constants::MAX_SAVE_SLOT; ++slot) {
        std::string path = std::string("saves/slot") + std::to_string(slot) + ".bin";
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
            deleted = true;
        }
    }
    return deleted;
}

// Spawn a test item set into player inventory
static void spawn_test_items(Player& player, MessageLog& log) {
    // IMPROVED: Use named constants for test item values
    Item weapon;
    weapon.name = "Test Sword";
    weapon.type = ItemType::Weapon;
    weapon.rarity = Rarity::Rare;
    weapon.attackBonus = game_constants::TEST_WEAPON_ATTACK;
    weapon.isEquippable = true;
    weapon.slot = EquipmentSlot::Weapon;
    player.inventory().push_back(weapon);

    Item armor;
    armor.name = "Test Armor";
    armor.type = ItemType::Armor;
    armor.rarity = Rarity::Rare;
    armor.defenseBonus = game_constants::TEST_ARMOR_DEFENSE;
    armor.isEquippable = true;
    armor.slot = EquipmentSlot::Chest;
    player.inventory().push_back(armor);

    Item potion;
    potion.name = "Test Potion";
    potion.type = ItemType::Consumable;
    potion.rarity = Rarity::Common;
    potion.isConsumable = true;
    potion.healAmount = game_constants::TEST_POTION_HEAL;
    player.inventory().push_back(potion);

    log.add(MessageType::Debug, "Spawned test items: sword, armor, potion.");
}

// Check if this is a boss floor
static bool is_boss_floor(int depth) {
    // IMPROVED: Use named constants instead of magic numbers
    return (depth == game_constants::BOSS_FLOOR_1 || 
            depth == game_constants::BOSS_FLOOR_2 || 
            depth == game_constants::BOSS_FLOOR_3);
}

// Get the boss type for the given depth
static EnemyType get_boss_for_depth(int depth) {
    // IMPROVED: Use named constants for boss floors
    switch (depth) {
        case game_constants::BOSS_FLOOR_1:  return EnemyType::StoneGolem;   // Floor 2 boss
        case game_constants::BOSS_FLOOR_2:  return EnemyType::ShadowLord;   // Floor 4 boss
        case game_constants::BOSS_FLOOR_3: return EnemyType::Dragon;       // Floor 6 final boss
        default: return EnemyType::Dragon;       // Fallback
    }
}

static void save_corpse_state(const Player& player, Difficulty difficulty, int depth,
                              unsigned int seed, const Position& stairs) {
    GameState corpse{};
    corpse.difficulty = difficulty;
    corpse.player = player;
    corpse.depth = depth;
    corpse.seed = seed;
    corpse.stairsDown = stairs;
    // IMPROVED: Use named constant for corpse save slot
    fileio::save_to_slot(corpse, game_constants::CORPSE_SAVE_SLOT);
    LOG_INFO("Corpse state saved for corpse run recovery");
}

// Get a random enemy type appropriate for the given depth
static EnemyType get_enemy_type_for_depth(int depth, std::mt19937& rng) {
    std::uniform_int_distribution<int> roll(0, 100);
    int r = roll(rng);
    
    if (depth <= 2) {
        // Early floors: rats, spiders, goblins
        if (r < 40) return EnemyType::Rat;
        if (r < 70) return EnemyType::Spider;
        return EnemyType::Goblin;
    } else if (depth <= 4) {
        // Mid-early: goblins, kobolds, orcs, archers
        if (r < 25) return EnemyType::Goblin;
        if (r < 50) return EnemyType::Kobold;
        if (r < 65) return EnemyType::Archer;  // 15% chance for archer
        if (r < 85) return EnemyType::Orc;
        return EnemyType::Zombie;
    } else if (depth <= 6) {
        // Mid: orcs, zombies, gnomes, archers
        if (r < 20) return EnemyType::Orc;
        if (r < 35) return EnemyType::Archer;  // 15% chance
        if (r < 55) return EnemyType::Zombie;
        if (r < 75) return EnemyType::Gnome;
        return EnemyType::Ogre;
    } else if (depth <= 8) {
        // Late-mid: gnomes, ogres, trolls, archers
        if (r < 25) return EnemyType::Gnome;
        if (r < 40) return EnemyType::Archer;  // 15% chance
        if (r < 60) return EnemyType::Ogre;
        if (r < 90) return EnemyType::Troll;
        return EnemyType::Dragon;
    } else {
        // Deep floors: trolls, dragons, liches
        if (r < 30) return EnemyType::Troll;
        if (r < 60) return EnemyType::Dragon;
        return EnemyType::Lich;
    }
}

// Spawn a boss enemy for the current floor
static void spawn_boss(std::vector<Enemy>& enemies, const Dungeon& dungeon, MessageLog& log, 
                       int depth, std::mt19937& /* rng */, const DifficultyParams& params) {
    EnemyType bossType = get_boss_for_depth(depth);
    Enemy boss(bossType);
    
    // Find a good position for the boss (preferably in a boss chamber)
    const auto& rooms = dungeon.rooms();
    for (const auto& room : rooms) {
        if (room.type == RoomType::BOSS_CHAMBER) {
            boss.set_position(room.center_x(), room.center_y());
            break;
        }
    }
    
    // If no boss chamber found, spawn near stairs
    if (boss.get_position().x == 0 && boss.get_position().y == 0) {
        // Find stairs down and spawn nearby
        for (int y = 0; y < dungeon.height(); ++y) {
            for (int x = 0; x < dungeon.width(); ++x) {
                if (dungeon.get_tile(x, y) == TileType::StairsDown) {
                    boss.set_position(x, y);
                    break;
                }
            }
        }
    }
    
    // Scale boss stats with difficulty (heavily nerfed scaling)
    // IMPROVED: Use named constants for boss scaling
    // Reduced HP scaling: only +1 per depth instead of +5
    int baseHp = boss.stats().maxHp + depth;  // Much less HP scaling
    // Reduced attack scaling: only +0.5 per depth (rounded down)
    int baseAtk = boss.stats().attack + (depth / 2);  // Much less attack scaling
    boss.stats().maxHp = static_cast<int>(baseHp * params.enemyHpMultiplier);
    boss.stats().hp = boss.stats().maxHp;
    boss.stats().attack = static_cast<int>(baseAtk * params.enemyDamageMultiplier);
    
    enemies.push_back(boss);
    
    // Announce boss
    log.add(MessageType::Warning, "\033[1;91m" + std::string(glyphs::warning()) + " " + boss.name() + " awaits!\033[0m");
    LOG_INFO("Spawned boss " + boss.name() + " on floor " + std::to_string(depth));
}

// Spawn a new enemy near the player with difficulty scaling
static void spawn_enemy_near_player(std::vector<Enemy>& enemies, const Player& player, const Dungeon& dungeon, MessageLog& log, int depth, std::mt19937& rng, const DifficultyParams& params) {
    Position pp = player.get_position();
    // IMPROVED: Use named constant for spawn search radius
    // Try to find a walkable spot within search radius
    for (int dy = -game_constants::BOSS_SPAWN_SEARCH_RADIUS; dy <= game_constants::BOSS_SPAWN_SEARCH_RADIUS; ++dy) {
        for (int dx = -game_constants::BOSS_SPAWN_SEARCH_RADIUS; dx <= game_constants::BOSS_SPAWN_SEARCH_RADIUS; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int nx = pp.x + dx;
            int ny = pp.y + dy;
            if (dungeon.in_bounds(nx, ny) && dungeon.is_walkable(nx, ny)) {
                EnemyType type = get_enemy_type_for_depth(depth, rng);
                Enemy e(type);
                e.set_position(nx, ny);
                // IMPROVED: Use named constants for enemy scaling
                // Scale stats with depth AND difficulty
                int baseHp = e.stats().maxHp + depth * game_constants::ENEMY_HP_SCALING_PER_DEPTH;
                int baseAtk = e.stats().attack + depth / game_constants::ENEMY_ATK_SCALING_DIVISOR;
                e.stats().maxHp = static_cast<int>(baseHp * params.enemyHpMultiplier);
                e.stats().hp = e.stats().maxHp;
                e.stats().attack = static_cast<int>(baseAtk * params.enemyDamageMultiplier);
                enemies.push_back(e);
                log.add(MessageType::Warning, "A " + e.name() + " appears!");
                return;
            }
        }
    }
    log.add(MessageType::Info, "The dungeon remains quiet...");
}

// in_simple_fov moved to viewport.cpp

// Find enemy at position, returns nullptr if none
static Enemy* find_enemy_at(std::vector<Enemy>& enemies, int x, int y) {
    for (auto& e : enemies) {
        if (e.get_position().x == x && e.get_position().y == y) {
            return &e;
        }
    }
    return nullptr;
}

// Global trap tracking for the current floor
static std::vector<traps::Trap> floorTraps;

// Initialize traps for a floor based on dungeon tiles
static void initialize_floor_traps(const Dungeon& dungeon, std::mt19937& rng) {
    floorTraps.clear();
    for (int y = 0; y < dungeon.height(); ++y) {
        for (int x = 0; x < dungeon.width(); ++x) {
            if (dungeon.get_tile(x, y) == TileType::Trap) {
                TrapType type = traps::get_random_trap_type(rng);
                floorTraps.push_back(traps::create_trap(x, y, type));
            }
        }
    }
    LOG_DEBUG("Initialized " + std::to_string(floorTraps.size()) + " traps on floor");
}

// Check if player steps on a trap and trigger it
static void check_trap_at_player(Player& player, Dungeon& dungeon, MessageLog& log, std::mt19937& rng) {
    Position pos = player.get_position();
    
    for (auto& trap : floorTraps) {
        if (trap.position.x == pos.x && trap.position.y == pos.y && !trap.triggered) {
            // Check if player detects it first
            if (!trap.detected && traps::player_detects_trap(player, trap, rng)) {
                trap.detected = true;
                log.add(MessageType::Warning, "\033[93m" + std::string(glyphs::warning()) + " You spot a " + 
                        traps::get_trap_description(trap.type) + "!\033[0m");
                return;  // Don't trigger if just detected
            }
            
            // Trigger the trap
            traps::trigger_trap(trap, player, dungeon, log, rng);
            return;
        }
    }
}

// Try to move player, or attack if enemy is in the way (bump-to-attack)
// Returns true if the player took an action (moved or attacked)
static bool try_move_or_attack(Player& player, std::vector<Enemy>& enemies, 
                                Dungeon& dungeon, MessageLog& log,
                                int dx, int dy, std::mt19937& rng) {
    Position p = player.get_position();
    int newX = p.x + dx;
    int newY = p.y + dy;
    
    // Check for enemy at target position (bump-to-attack)
    Enemy* target = find_enemy_at(enemies, newX, newY);
    if (target) {
        LOG_DEBUG("Player bumping into enemy " + target->name() + " at (" + 
                  std::to_string(newX) + "," + std::to_string(newY) + ")");
        
        // Enter tactical combat mode with full 3D combat system
        LOG_OP_START("enter_combat_mode");
        bool playerWon = combat::enter_combat_mode(player, *target, dungeon, log);
        LOG_OP_END("enter_combat_mode");
        (void)playerWon;  // Result handled by main loop (death check)
        
        // If player died in combat, the main loop will handle it
        // If player won or retreated, continue normally
        return true;
    }
    
    // No enemy, try to move
    if (dungeon.is_walkable(newX, newY)) {
        player.move_by(dx, dy);
        
        // Check for traps at new position
        check_trap_at_player(player, dungeon, log, rng);
        
        return true;
    }
    
    return false;
}

static void show_ascii_screen(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return;
    }
    ui::clear();
    std::string line;
    int row = 2;
    while (std::getline(in, line)) {
        ui::move_cursor(row++, 2);
        std::cout << line;
    }
    std::cout.flush();
    input::read_key_blocking();
}

// Forward declaration
static void show_congratulations_page();

// Victory screen with stats (centered)
static void show_victory_screen(int floorsCleared, int enemiesKilled, const Player& player, unsigned int seed, const Leaderboard& leaderboard) {
    auto termSize = input::get_terminal_size();
    
    const int boxWidth = 60;
    const int boxHeight = 24;  // Increased for seed display
    int boxRow = std::max(2, (termSize.height - boxHeight) / 2);
    int boxCol = std::max(2, (termSize.width - boxWidth) / 2);
    
    ui::clear();
    
    // Play victory sound
    ui::play_victory_sound();
    
    // Draw frame
    ui::draw_box_double(boxRow, boxCol, boxWidth, boxHeight, "\033[38;5;226m"); // Gold
    
    // Victory title with color cycling
    ui::move_cursor(boxRow + 2, boxCol + 15);
    std::cout << "\033[1;33m" << glyphs::artifact() << " VICTORY! " << glyphs::artifact() << "\033[0m";
    
    ui::move_cursor(boxRow + 4, boxCol + 8);
    std::cout << "\033[38;5;226mYou have conquered the Rogue Depths!\033[0m";
    
    ui::move_cursor(boxRow + 5, boxCol + 10);
    std::cout << "\033[38;5;226mThe artifact is yours!\033[0m";
    
    // Stats frame
    ui::move_cursor(boxRow + 7, boxCol + 5);
    ui::set_color(constants::color_ui);
    for (int i = 0; i < boxWidth - 10; ++i) std::cout << glyphs::box_sgl_h();
    ui::reset_color();
    
    ui::move_cursor(boxRow + 8, boxCol + 5);
    std::cout << "\033[1;37m ADVENTURE STATISTICS \033[0m";
    
    ui::move_cursor(boxRow + 10, boxCol + 8);
    std::cout << glyphs::stairs_down() << " Floors Conquered: \033[1;32m" << floorsCleared << "\033[0m";
    
    ui::move_cursor(boxRow + 11, boxCol + 8);
    std::cout << glyphs::corpse() << " Enemies Slain:    \033[1;31m" << enemiesKilled << "\033[0m";
    
    ui::move_cursor(boxRow + 12, boxCol + 8);
    std::cout << glyphs::stat_hp() << " Final HP:         \033[1;32m" << player.get_stats().maxHp << "\033[0m";
    
    ui::move_cursor(boxRow + 13, boxCol + 8);
    std::cout << glyphs::stat_attack() << " Final ATK:        \033[1;33m" << player.get_stats().attack << "\033[0m";
    
    ui::move_cursor(boxRow + 14, boxCol + 8);
    std::cout << glyphs::stat_defense() << " Final DEF:        \033[1;36m" << player.get_stats().defense << "\033[0m";
    
    ui::move_cursor(boxRow + 15, boxCol + 8);
    std::cout << glyphs::stat_level() << " Class:            \033[1;37m" << Player::class_name(player.player_class()) << "\033[0m";

    ui::move_cursor(boxRow + 16, boxCol + 8);
    std::cout << glyphs::stat_speed() << " Final SPD:        \033[1;35m" << player.get_stats().speed << "\033[0m";

    ui::move_cursor(boxRow + 18, boxCol + 5);
    ui::set_color(constants::color_ui);
    for (int i = 0; i < boxWidth - 10; ++i) std::cout << glyphs::box_sgl_h();
    ui::reset_color();
    
    // Seed display for sharing/speedrunning
    ui::move_cursor(boxRow + 19, boxCol + 8);
    ui::set_color(constants::color_floor);
    std::cout << "Seed: " << seed << " (share this to replay!)";
    ui::reset_color();
    
    ui::move_cursor(boxRow + 20, boxCol + 8);
    std::cout << glyphs::msg_info() << " TAB to review stats, ENTER to start anew";
    
    // Show leaderboard below
    int leaderboardRow = boxRow + boxHeight + 2;
    int leaderboardCol = std::max(2, (termSize.width - 60) / 2);
    leaderboard.display(leaderboardRow, leaderboardCol, 60);
    
    ui::move_cursor(leaderboardRow + 12, boxCol + 12);
    ui::set_color(constants::color_floor);
    std::cout << "Press any key to continue...";
    ui::reset_color();
    
    std::cout.flush();
    input::read_key_blocking();
    
    // Show congratulations page
    show_congratulations_page();
}

// Congratulations page after victory
static void show_congratulations_page() {
    auto termSize = input::get_terminal_size();
    
    const int boxWidth = 70;
    const int boxHeight = 15;
    int boxRow = std::max(2, (termSize.height - boxHeight) / 2);
    int boxCol = std::max(2, (termSize.width - boxWidth) / 2);
    
    ui::clear();
    
    // Draw frame with celebration colors
    ui::draw_box_double(boxRow, boxCol, boxWidth, boxHeight, "\033[38;5;226m"); // Gold
    
    // Congratulations title
    ui::move_cursor(boxRow + 2, boxCol + 15);
    std::cout << "\033[1;33m" << glyphs::artifact() << " CONGRATULATIONS! " << glyphs::artifact() << "\033[0m";
    
    ui::move_cursor(boxRow + 4, boxCol + 12);
    std::cout << "\033[38;5;226mYou have successfully completed";
    
    ui::move_cursor(boxRow + 5, boxCol + 18);
    std::cout << "\033[38;5;226mRogue Depths!\033[0m";
    
    ui::move_cursor(boxRow + 7, boxCol + 10);
    std::cout << "\033[1;37mThank you for playing!\033[0m";
    
    ui::move_cursor(boxRow + 9, boxCol + 8);
    std::cout << "\033[38;5;250mYour journey through the depths";
    
    ui::move_cursor(boxRow + 10, boxCol + 10);
    std::cout << "\033[38;5;250mhas been legendary!\033[0m";
    
    ui::move_cursor(boxRow + 12, boxCol + 20);
    ui::set_color(constants::color_floor);
    std::cout << "Returning to main menu...";
    ui::reset_color();
    
    std::cout.flush();
    
    // Wait 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

// Game over screen with stats (centered)
static void show_gameover_screen(int floorReached, int enemiesKilled, const std::string& causeOfDeath, unsigned int seed, const Leaderboard& leaderboard) {
    auto termSize = input::get_terminal_size();
    
    const int boxWidth = 60;
    const int boxHeight = 22;  // Increased for seed display
    int boxRow = std::max(2, (termSize.height - boxHeight) / 2);
    int boxCol = std::max(2, (termSize.width - boxWidth) / 2);
    
    ui::clear();
    
    // Play death sound
    ui::play_death_sound();
    
    // Draw frame
    ui::draw_box_double(boxRow, boxCol, boxWidth, boxHeight, "\033[38;5;196m"); // Red
    
    // Game over title
    ui::move_cursor(boxRow + 2, boxCol + 18);
    std::cout << "\033[1;31m" << glyphs::corpse() << " GAME OVER " << glyphs::corpse() << "\033[0m";
    
    ui::move_cursor(boxRow + 4, boxCol + 12);
    std::cout << "\033[38;5;196mYou have perished in the depths.\033[0m";
    
    // Stats frame
    ui::move_cursor(boxRow + 6, boxCol + 5);
    ui::set_color(constants::color_ui);
    for (int i = 0; i < boxWidth - 10; ++i) std::cout << glyphs::box_sgl_h();
    ui::reset_color();
    
    ui::move_cursor(boxRow + 7, boxCol + 5);
    std::cout << "\033[1;37m FINAL STATISTICS \033[0m";
    
    ui::move_cursor(boxRow + 9, boxCol + 8);
    std::cout << glyphs::stairs_down() << " Floor Reached:    \033[1;33m" << floorReached << "\033[0m";
    
    ui::move_cursor(boxRow + 10, boxCol + 8);
    std::cout << glyphs::corpse() << " Enemies Slain:    \033[1;31m" << enemiesKilled << "\033[0m";
    
    ui::move_cursor(boxRow + 12, boxCol + 8);
    std::cout << glyphs::msg_death() << " Cause of Death:";
    ui::move_cursor(boxRow + 13, boxCol + 10);
    std::cout << "\033[38;5;196m" << causeOfDeath << "\033[0m";
    
    ui::move_cursor(boxRow + 15, boxCol + 5);
    ui::set_color(constants::color_ui);
    for (int i = 0; i < boxWidth - 10; ++i) std::cout << glyphs::box_sgl_h();
    ui::reset_color();
    
    // Seed display for retry
    ui::move_cursor(boxRow + 16, boxCol + 8);
    ui::set_color(constants::color_floor);
    std::cout << "Seed: " << seed << " (use --seed to retry)";
    ui::reset_color();
    
    ui::move_cursor(boxRow + 17, boxCol + 8);
    std::cout << glyphs::msg_info() << " Press R in-game to reset run from title";
    
    // Show leaderboard below
    int leaderboardRow = boxRow + boxHeight + 2;
    int leaderboardCol = std::max(2, (termSize.width - 60) / 2);
    leaderboard.display(leaderboardRow, leaderboardCol, 60);
    
    ui::move_cursor(leaderboardRow + 12, boxCol + 12);
    ui::set_color(constants::color_floor);
    std::cout << "Press any key to continue...";
    ui::reset_color();
    
    std::cout.flush();
    input::read_key_blocking();
}

// Color cycling colors for title animation
static const std::vector<std::string> title_colors = {
    "\033[38;5;196m",  // Red
    "\033[38;5;208m",  // Orange
    "\033[38;5;226m",  // Yellow
    "\033[38;5;46m",   // Green
    "\033[38;5;51m",   // Cyan
    "\033[38;5;33m",   // Blue
    "\033[38;5;129m",  // Magenta
};

// Animated title screen with menu (centered on terminal)
// Returns: 0 = New Game, 1 = Load Game, 2 = Tutorial, 3 = Help, 4 = Leaderboard, 5 = Quit
static int show_animated_title(unsigned int seed = 0) {
    // Load title ASCII art
    std::vector<std::string> titleLines;
    std::ifstream in("assets/ascii/title.txt");
    if (in) {
        std::string line;
        while (std::getline(in, line)) {
            titleLines.push_back(line);
        }
    }
    if (titleLines.empty()) {
        titleLines = {
            "  _____                         ____              _        ",
            " |  __ \\                       |  _ \\            | |       ",
            " | |__) | ___   __ _  ___ ___  | | | | ___   ___ | | _____ ",
            " |  _  / / _ \\ / _` |/ __/ _ \\ | | | |/ _ \\ / _ \\| |/ / _ \\",
            " | | \\ \\| (_) | (_| | (_|  __/ | |_| | (_) | (_) |   <  __/",
            " |_|  \\_\\\\___/ \\__,_|\\___\\___| |____/ \\___/ \\___/|_|\\_\\___|"
        };
    }
    
    int selected = 0;
    int colorIndex = 0;
    auto lastColorChange = std::chrono::steady_clock::now();
    const auto colorInterval = std::chrono::milliseconds(400);
    
    // Get terminal size for centering
    auto termSize = input::get_terminal_size();
    
    // Frame dimensions
    const int frameWidth = 70;
    const int frameHeight = 35;
    int frameRow = std::max(1, (termSize.height - frameHeight) / 2);
    int frameCol = std::max(1, (termSize.width - frameWidth) / 2);
    
    while (true) {
        ui::clear();
        
        // Draw outer frame (centered)
        ui::draw_box_double(frameRow, frameCol, frameWidth, frameHeight, constants::color_frame_main);
        
        // Draw title with cycling color
        auto now = std::chrono::steady_clock::now();
        if (now - lastColorChange >= colorInterval) {
            colorIndex = (colorIndex + 1) % static_cast<int>(title_colors.size());
            lastColorChange = now;
        }
        
        ui::set_color(title_colors[colorIndex]);
        int row = frameRow + 2;
        for (const auto& line : titleLines) {
            ui::move_cursor(row++, frameCol + 4);
            std::cout << line;
        }
        ui::reset_color();
        
        // Tagline
        ui::move_cursor(row + 1, frameCol + 14);
        ui::set_color(constants::color_floor);
        std::cout << "~ Descend into the Abyss ~";
        ui::reset_color();
        
        // Menu frame (centered within main frame)
        int menuRow = frameRow + 19;
        int menuCol = frameCol + 19;
        ui::draw_box_single(menuRow, menuCol, 30, 10, constants::color_ui);
        ui::move_cursor(menuRow, menuCol + 2);
        ui::set_color(constants::color_ui);
        std::cout << " MAIN MENU ";
        ui::reset_color();
        
        // Menu options (only New Game, Help, Quit)
        const char* options[] = {"[N] New Game", "[H] Help", "[Q] Quit"};
        const int numOptions = 3;
        for (int i = 0; i < numOptions; ++i) {
            ui::move_cursor(menuRow + 2 + i, menuCol + 2);
            if (i == selected) {
                ui::set_color(constants::ansi_bold);
                ui::set_color(constants::color_player);
                std::cout << glyphs::arrow_right() << " " << options[i];
            } else {
                std::cout << "  " << options[i];
            }
            ui::reset_color();
        }
        
        // Navigation hint
        ui::move_cursor(menuRow + 7, menuCol + 2);
        ui::set_color(constants::color_floor);
        std::cout << glyphs::arrow_up() << "/" << glyphs::arrow_down() << " Select  Enter Confirm";
        ui::reset_color();
        
        // Seed + controls info strip
        ui::move_cursor(frameRow + frameHeight - 6, frameCol + 4);
        ui::set_color(constants::color_floor);
        std::cout << glyphs::artifact() << " Seed: " << seed << "   (pass --seed to replay)";
        ui::reset_color();

        ui::move_cursor(frameRow + frameHeight - 5, frameCol + 4);
        ui::set_color(constants::color_floor);
        std::cout << glyphs::msg_info() << " Controls: W/S or " << glyphs::arrow_up() << "/" << glyphs::arrow_down()
                  << " to navigate, Enter to confirm";
        ui::reset_color();

        // Options hint
        ui::move_cursor(frameRow + frameHeight - 4, frameCol + 4);
        ui::set_color(constants::color_floor);
        std::cout << glyphs::msg_info() << " Options: TAB cycles HUD, R resets run";
        ui::reset_color();

        // Unicode/ASCII mode hint on its own line for clarity
        ui::move_cursor(frameRow + frameHeight - 3, frameCol + 4);
        ui::set_color(constants::color_floor);
        std::cout << glyphs::msg_info() << " "
                  << (glyphs::use_unicode ? "Tip: use --no-unicode/--no-color for ASCII-safe mode"
                                          : "ASCII-safe mode enabled (use --no-unicode/--no-color)");
        ui::reset_color();

        // Version info and seed display
        ui::move_cursor(frameRow + frameHeight - 2, frameCol + 2);
        ui::set_color(constants::color_floor);
        std::cout << "v1.0 | Rogue Depths";
        if (seed != 0) {
            std::cout << " | Seed: " << seed;
        }
        ui::reset_color();
        
        std::cout.flush();
        
        // Non-blocking input check for animation
        int key = input::read_key_nonblocking();
        if (key == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        
        switch (key) {
            case 'w':
            case 'W':
            case input::KEY_UP:
                selected = (selected - 1 + numOptions) % numOptions;
                break;
            case 's':
            case 'S':
            case input::KEY_DOWN:
                selected = (selected + 1) % numOptions;
                break;
            case 'n':
            case 'N':
                return 0; // New Game
            case 'h':
            case 'H':
            case '?':
                return 3; // Help
            case 'q':
            case 'Q':
                return 5; // Quit
            case '\n':
            case '\r':
            case ' ':
                // Map selection index to internal menu codes:
                // 0 -> New Game (0), 1 -> Help (3), 2 -> Quit (5)
                if (selected == 0) return 0;
                if (selected == 1) return 3;
                return 5;
        }
    }
}

// Class selection menu - returns chosen class (centered, framed design)
static PlayerClass show_class_selection() {
    int selected = 0;
    const int numClasses = 2;  // Warrior and Mage only (Rogue hidden)
    
    // Get terminal size for centering
    auto termSize = input::get_terminal_size();
    
    // Box dimensions
    const int boxWidth = 60;
    const int boxHeight = 20;
    int boxRow = std::max(2, (termSize.height - boxHeight) / 2);
    int boxCol = std::max(2, (termSize.width - boxWidth) / 2);
    
    while (true) {
        ui::clear();
        
        // Draw outer frame
        ui::draw_box_double(boxRow, boxCol, boxWidth, boxHeight, constants::color_frame_main);
        
        // Title
        ui::move_cursor(boxRow, boxCol + 2);
        ui::set_color(constants::color_frame_main);
        std::cout << glyphs::box_dbl_v();
        ui::set_color(constants::ansi_bold);
        ui::set_color(constants::color_player);
        std::cout << " CHOOSE YOUR CLASS ";
        ui::set_color(constants::color_frame_main);
        std::cout << glyphs::box_dbl_v();
        ui::reset_color();
        
        // Class descriptions with stats preview
        struct ClassInfo {
            const char* name;
            const char* desc;
            const char* stats;
            const char* color;
        };
        
        ClassInfo classes[] = {
            {"WARRIOR", "A sturdy fighter who excels in close combat.",
             "HP: 13  ATK: 5  DEF: 2  SPD: 10", "\033[38;5;196m"},  // Red
            {"MAGE", "A defensive caster with magical protection.",
             "HP: 9   ATK: 4  DEF: 4  SPD: 10", "\033[38;5;51m"}    // Cyan
        };
        
        int classRow = boxRow + 3;
        for (int i = 0; i < numClasses; ++i) {
            // Selection indicator
            ui::move_cursor(classRow, boxCol + 3);
            if (i == selected) {
                ui::set_color(constants::color_player);
                std::cout << glyphs::arrow_right() << " ";
            } else {
                std::cout << "  ";
            }
            
            // Class name
            if (i == selected) {
                std::cout << classes[i].color;
                ui::set_color(constants::ansi_bold);
            }
            std::cout << "[" << (i + 1) << "] " << classes[i].name;
            ui::reset_color();
            
            // Description
            ui::move_cursor(classRow + 1, boxCol + 7);
            ui::set_color(constants::color_floor);
            std::cout << classes[i].desc;
            ui::reset_color();
            
            // Stats preview (only for selected)
            if (i == selected) {
                ui::move_cursor(classRow + 2, boxCol + 7);
                ui::set_color(constants::color_ui);
                std::cout << glyphs::stat_hp() << " " << classes[i].stats;
                ui::reset_color();
            }
            
            classRow += 4;
        }
        
        // Separator line
        ui::move_cursor(boxRow + boxHeight - 4, boxCol + 2);
        ui::set_color(constants::color_frame_main);
        for (int i = 0; i < boxWidth - 4; ++i) std::cout << glyphs::box_sgl_h();
        ui::reset_color();
        
        // Navigation hint (no numeric quick-select text)
        ui::move_cursor(boxRow + boxHeight - 2, boxCol + 5);
        ui::set_color(constants::color_floor);
        std::cout << glyphs::arrow_up() << "/" << glyphs::arrow_down() 
                  << " or W/S: Select    Enter/Space: Confirm";
        ui::reset_color();
        
        std::cout.flush();
        
        int key = input::read_key_blocking();
        switch (key) {
            case 'w':
            case 'W':
            case input::KEY_UP:
                selected = (selected - 1 + numClasses) % numClasses;
                break;
            case 's':
            case 'S':
            case input::KEY_DOWN:
                selected = (selected + 1) % numClasses;
                break;
            case '1':
                return PlayerClass::Warrior;
            case '2':
                return PlayerClass::Mage;
            case '\n':
            case '\r':
            case ' ':
                switch (selected) {
                    case 0: return PlayerClass::Warrior;
                    case 1: return PlayerClass::Mage;
                }
                break;
        }
    }
}

static Item generate_loot(std::mt19937& rng, const DifficultyParams& params) {
    std::uniform_int_distribution<int> rarityDist(0, 100);
    int roll = rarityDist(rng);
    float lootBias = std::clamp(params.lootMultiplier, 0.5f, 2.0f);
    int adjustedRoll = std::clamp(static_cast<int>(roll * lootBias), 0, 100);
    Item item;
    if (adjustedRoll > 95) {
        item.name = "Time Knife";
        item.type = ItemType::Weapon;
        item.rarity = Rarity::Legendary;
        item.attackBonus = 5;
        item.isEquippable = true;
        item.slot = EquipmentSlot::Weapon;
    } else if (adjustedRoll > 75) {
        item.name = "Soul Drinker";
        item.type = ItemType::Weapon;
        item.rarity = Rarity::Epic;
        item.attackBonus = 3;
        item.isEquippable = true;
        item.slot = EquipmentSlot::Weapon;
    } else if (adjustedRoll > 50) {
        item.name = "Sturdy Vest";
        item.type = ItemType::Armor;
        item.rarity = Rarity::Rare;
        item.defenseBonus = 2;
        item.isEquippable = true;
        item.slot = EquipmentSlot::Chest;
    } else {
        item.name = "Minor Tonic";
        item.type = ItemType::Consumable;
        item.rarity = Rarity::Common;
        item.isConsumable = true;
        item.healAmount = 5;
    }
    return item;
}

// Get item glyph based on type (returns first char of glyph string)
static char get_item_glyph(ItemType type) {
    switch (type) {
        case ItemType::Weapon:     return glyphs::weapon()[0];
        case ItemType::Armor:      return glyphs::armor()[0];
        case ItemType::Consumable: return glyphs::potion()[0];
        case ItemType::Quest:      return glyphs::artifact()[0];
        default:                   return glyphs::gold()[0];
    }
}

// Get item color based on rarity
static std::string get_item_color(Rarity rarity) {
    switch (rarity) {
        case Rarity::Common:    return constants::color_item_common;
        case Rarity::Uncommon:  return constants::color_item_uncommon;
        case Rarity::Rare:      return constants::color_item_rare;
        case Rarity::Epic:      return constants::color_item_epic;
        case Rarity::Legendary: return constants::color_item_legendary;
        default:                return constants::color_item_common;
    }
}

// Viewport functions moved to viewport.cpp

// Legacy draw_map for compatibility (not used with new viewport)
static void draw_map(const Dungeon& dungeon, const Player& player, const std::vector<Enemy>& enemies) {
    draw_map_viewport(dungeon, player, enemies, 1, 1, constants::viewport_width, constants::viewport_height);
}

// Check terminal size and ensure it's large enough for the game
static void check_terminal_size() {
    const int MIN_WIDTH = 260;  // Minimum terminal width required
    
    while (true) {
        auto termSize = input::get_terminal_size();
        
        if (termSize.width >= MIN_WIDTH) {
            // Terminal is large enough, proceed
            break;
        }
        
        // Terminal too small - show warning
        ui::clear();
        
        // Calculate box position (centered)
        const int boxWidth = MIN_WIDTH;
        const int boxHeight = 13;
        auto currentSize = input::get_terminal_size();
        int boxRow = std::max(2, (currentSize.height - boxHeight) / 2);
        int boxCol = std::max(2, (currentSize.width - boxWidth) / 2);
        
        // Draw box
        ui::draw_box_double(boxRow, boxCol, boxWidth, boxHeight, constants::color_frame_main);
        
        // Title
        ui::move_cursor(boxRow + 1, boxCol + (boxWidth - 20) / 2);
        ui::set_color(constants::color_player);
        std::cout << "⚠ Terminal Too Small ⚠";
        ui::reset_color();
        
        // Message
        ui::move_cursor(boxRow + 3, boxCol + 2);
        ui::set_color(constants::color_floor);
        std::cout << "Your terminal width is " << termSize.width << " characters.";
        ui::move_cursor(boxRow + 4, boxCol + 2);
        std::cout << "Rogue Depths requires at least " << MIN_WIDTH << " characters.";
        ui::reset_color();
        
        // Bar of equal size
        ui::move_cursor(boxRow + 6, boxCol + 1);
        ui::set_color(constants::color_ui);
        for (int i = 0; i < boxWidth; ++i) {
            std::cout << "~";
        }
        ui::reset_color();
        
        // Instructions
        ui::move_cursor(boxRow + 8, boxCol + 2);
        ui::set_color(constants::color_floor);
        std::cout << "Please resize your terminal window:";
        ui::move_cursor(boxRow + 9, boxCol + 2);
        std::cout << "• Go full screen (F11 or maximize window)";
        ui::move_cursor(boxRow + 10, boxCol + 2);
        std::cout << "• Press Ctrl + Scroll Down to zoom out";
        ui::move_cursor(boxRow + 11, boxCol + 2);
        std::cout << "• Or manually resize the window";
        ui::move_cursor(boxRow + 12, boxCol + (boxWidth - 25) / 2);
        ui::set_color(constants::ansi_bold);
        std::cout << "Press ENTER when ready...";
        ui::reset_color();
        
        std::cout.flush();
        
        // Wait for Enter key
        int key = input::read_key_blocking();
        if (key == '\n' || key == '\r') {
            // Check again after Enter
            continue;
        }
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Windows console setup for UTF-8 support
    SetConsoleOutputCP(65001); // UTF-8 code page
    SetConsoleCP(65001);       // UTF-8 input code page
    
    // Enable virtual terminal processing for ANSI escape sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hIn, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
            SetConsoleMode(hIn, dwMode);
}
    }
#endif

    // Initialize UI and input before checking terminal size
    ui::init();
    input::enable_raw_mode();
    
    // Check terminal size before proceeding
    check_terminal_size();

    // Parse command-line arguments
    CLIConfig cliConfig = cli::parse(argc, argv);
    cli::set_config(cliConfig);
    
    // Handle help/version/errors
    if (cliConfig.showHelp) {
        cli::print_help(argv[0]);
        return cliConfig.exitCode;
    }
    if (cliConfig.showVersion) {
        cli::print_version();
        return cliConfig.exitCode;
    }
    if (cliConfig.exitRequested) {
        return cliConfig.exitCode;
    }
    
    // Initialize logger if log file specified
    if (!cliConfig.logFile.empty()) {
        Logger::instance().init(cliConfig.logFile);
        LOG_INFO("Command-line arguments parsed successfully");
        if (cliConfig.debug) LOG_INFO("Debug mode enabled");
        if (cliConfig.noColor) LOG_INFO("Color output disabled");
        if (cliConfig.noUnicode) LOG_INFO("Unicode output disabled");
    }
    
    // Initialize glyph system based on CLI settings
    glyphs::init(!cliConfig.noUnicode, !cliConfig.noColor);
    
    // Initialize keybindings from config file
    keybinds::init("config/controls.json");
    
    // Seed - use CLI seed if provided, otherwise random
    std::random_device rd;
    unsigned int seed = (cliConfig.seed != 0) ? cliConfig.seed : rd();
    std::mt19937 rng(seed);
    
    LOG_INFO("Random seed: " + std::to_string(seed));

    // UI and input already initialized earlier for terminal size check
    // Animated title screen with menu (pass seed for display)
    int menuChoice = show_animated_title(seed);
    
    while (true) {
        if (menuChoice == 5) {
            // Quit selected
            LOG_INFO("User quit from title screen");
            Logger::instance().shutdown();
            input::disable_raw_mode();
            ui::shutdown();
            return 0;
        }
        // Tutorial temporarily disabled: hide option by treating choice 2 as "Start Game"
        if (menuChoice == 2) {
            // Fall through to normal game start (same as choice 1)
            menuChoice = 1;
        }
        if (menuChoice == 4) {
            // View Leaderboard selected
            Leaderboard lb;
            lb.load();
            // Center leaderboard on screen
            auto termSize = input::get_terminal_size();
            int width = 60;
            int startRow = std::max(2, (termSize.height - 15) / 2);
            int startCol = std::max(2, (termSize.width - width) / 2);
            lb.display(startRow, startCol, width);
            ui::move_cursor(startRow + 15, startCol + 2);
            std::cout << "Press any key to return to menu...";
            std::cout.flush();
            input::read_key_blocking();
            menuChoice = show_animated_title(seed);
            continue;
        }
        if (menuChoice == 3) {
            // Help selected - show help then return to title
            int helpPage = 0;
            while (true) {
                ui::draw_help_screen(helpPage);
                int key = input::read_key_blocking();
                if (key == input::KEY_LEFT || key == 'a' || key == 'A') {
                    helpPage = (helpPage - 1 + ui::help_page_count) % ui::help_page_count;
                } else if (key == input::KEY_RIGHT || key == 'd' || key == 'D') {
                    helpPage = (helpPage + 1) % ui::help_page_count;
                } else {
                    break; // Any other key exits help
                }
            }
            // After help, restart title screen
            menuChoice = show_animated_title(seed);
            continue;
        }
        break;
    }

    // Try to load existing game (slot 1)
    GameState loaded{};
    bool hasSave = fileio::load_from_slot(loaded, 1);
    
    // If Load Game selected but no save exists, treat as new game
    if (menuChoice == 1 && !hasSave) {
        menuChoice = 0; // Fall back to new game
    }
    
    // Force new game if selected
    if (menuChoice == 0) {
        hasSave = false;
    }

    Difficulty difficulty = hasSave ? loaded.difficulty : Difficulty::Adventurer;
    DifficultyParams params = get_difficulty_params(difficulty);

    // Current floor depth (needed for dungeon generation)
    int currentDepth = hasSave ? loaded.depth : 1;

    // Build dungeon and player
    // Dynamic map sizing based on depth: width = 30 + depth*10, height = 15 + depth*5
    int mapWidth = 30 + currentDepth * 10;
    int mapHeight = 15 + currentDepth * 5;
    Dungeon dungeon(mapWidth, mapHeight);
    Position start{};
    Position stairsDown{};
    if (hasSave) {
        seed = loaded.seed;
        dungeon.generate(seed, start, stairsDown, currentDepth);
        // Use saved positions
        stairsDown = loaded.stairsDown;
        LOG_INFO("Loaded save from slot 1");
    } else {
        dungeon.generate(seed, start, stairsDown, currentDepth);
        LOG_INFO("Generated new dungeon floor 1");
    }
    
    // Initialize traps for the floor
    initialize_floor_traps(dungeon, rng);

    // Player creation: load from save or create new with class selection
    Player player;
    std::string selectedClassName;
    if (hasSave) {
        player = loaded.player;
    } else {
        // Show class selection for new game
        PlayerClass chosenClass = show_class_selection();
        selectedClassName = Player::class_name(chosenClass);
        player = Player(chosenClass);
        player.set_position(start.x, start.y);
        player.get_stats().maxHp += params.playerHpBoost;
        player.get_stats().hp = player.get_stats().maxHp;
        player.set_depth(currentDepth);  // Set initial depth for attack bonus
        LOG_INFO("Class selected: " + selectedClassName);
        
        // Add starter weapon based on class
        Item starterWeapon;
        starterWeapon.type = ItemType::Weapon;
        starterWeapon.isEquippable = true;
        starterWeapon.slot = EquipmentSlot::Weapon;
        starterWeapon.rarity = Rarity::Common;
        starterWeapon.attackBonus = 2;  // Small starter bonus
        
        // All classes get a starter sword
        starterWeapon.name = "Starter Sword";
        player.inventory().push_back(starterWeapon);
        LOG_INFO("Added starter weapon: " + starterWeapon.name);
        
        // Add 5 healing potions to starting inventory
        for (int i = 0; i < 5; ++i) {
            Item healingPotion;
            healingPotion.name = "Healing Potion";
            healingPotion.type = ItemType::Consumable;
            healingPotion.isConsumable = true;
            healingPotion.healAmount = 20;  // Heals 20 HP
            healingPotion.rarity = Rarity::Common;
            player.inventory().push_back(healingPotion);
        }
        LOG_INFO("Added 5 healing potions to starting inventory");
    }

    MessageLog log;
    if (!hasSave && !selectedClassName.empty()) {
        log.add(MessageType::Info, "You embark as the " + selectedClassName + ".");
    }
    log.add(MessageType::Info, "Welcome to Rogue Depths. Press 'q' to quit.");

    // Spawn enemies (with difficulty scaling)
    std::vector<Enemy> enemies;
    if (hasSave) {
        enemies = loaded.enemies;
    } else {
        // Spawn initial enemy based on depth with difficulty scaling
        EnemyType initialType = get_enemy_type_for_depth(currentDepth, rng);
        Enemy e(initialType);
        e.set_position(stairsDown.x, stairsDown.y);
        // Apply difficulty multipliers
                        // IMPROVED: Use named constants for enemy scaling
                        int baseHp = e.stats().maxHp + currentDepth * game_constants::ENEMY_HP_SCALING_PER_DEPTH;
                        int baseAtk = e.stats().attack + currentDepth / game_constants::ENEMY_ATK_SCALING_DIVISOR;
        e.stats().maxHp = static_cast<int>(baseHp * params.enemyHpMultiplier);
        e.stats().hp = e.stats().maxHp;
        e.stats().attack = static_cast<int>(baseAtk * params.enemyDamageMultiplier);
        enemies.push_back(e);

        // Corpse run: if previous save exists in slot 2, spawn a tougher enemy at its former position
        GameState corpse{};
        if (fileio::load_from_slot(corpse, 2)) {
            Enemy c(EnemyType::CorpseEnemy);
            c.set_position(corpse.player.get_position().x, corpse.player.get_position().y);
            c.stats().maxHp = std::max(8, corpse.player.get_stats().maxHp / 2);
            c.stats().hp = c.stats().maxHp;
            c.stats().attack = std::max(4, corpse.player.get_stats().attack);
            enemies.push_back(c);
            log.add(MessageType::Warning, "You sense the presence of your past demise...");
        }
    }

    // Main loop
    bool running = true;
    bool corpseSaved = false;
    bool shrinePromptActive = false;  // Track if we're waiting for Y/N at a shrine
    int shrineMessageStage = -1;      // 0 = intro, 1 = prompt, 2 = warning, 3 = done
    auto shrineStageTime = std::chrono::steady_clock::now();
    Position prevPlayerPos = player.get_position();
    
    // Dynamic viewport sizing based on terminal
    auto termSize = input::get_terminal_size();
    auto vpSize = input::calculate_viewport(termSize.width, termSize.height);
    int viewport_w = vpSize.width;
    int viewport_h = vpSize.height;
    
    LOG_INFO("Entering main game loop");
    int frameCount = 0;
    int totalKillCount = 0;  // Track total enemies killed for stats
    std::string lastEnemyAttacker = "Unknown";  // Track what killed the player
    
    auto lastHeartbeat = std::chrono::steady_clock::now();
    
    while (running) {
        LOG_OP_START("game_loop_iteration");
        auto frameStart = std::chrono::steady_clock::now();
        
        // IMPROVED: Use named constant for heartbeat interval
        // Heartbeat: Log every N seconds to detect freezes
        auto now = std::chrono::steady_clock::now();
        auto timeSinceHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat);
        if (timeSinceHeartbeat.count() >= game_constants::UI_HEARTBEAT_INTERVAL_SECONDS) {
            // IMPROVED: Optimize string building for heartbeat
            std::string heartbeatMsg;
            heartbeatMsg.reserve(100);
            heartbeatMsg = "HEARTBEAT: Game loop still running (frame ";
            heartbeatMsg += std::to_string(frameCount);
            heartbeatMsg += ", player at ";
            heartbeatMsg += std::to_string(player.get_position().x);
            heartbeatMsg += ",";
            heartbeatMsg += std::to_string(player.get_position().y);
            heartbeatMsg += ")";
            LOG_DEBUG(heartbeatMsg);
            // stderr output disabled - heartbeat still logged to file
            // std::cerr << "[HEARTBEAT] Frame " << frameCount << std::endl;
            // std::cerr.flush();
            lastHeartbeat = now;
        }

        // Timed shrine prompt messages: show them one by one so player can read
        if (shrinePromptActive) {
            using namespace std::chrono;
            if (shrineMessageStage == 0) {
                // First message appears immediately when shrine prompt becomes active
                log.add(MessageType::Info, std::string(glyphs::shrine()) + " A mystical shrine pulses with energy.");
                shrineMessageStage = 1;
                shrineStageTime = now;
            } else if (shrineMessageStage == 1 &&
                       duration_cast<milliseconds>(now - shrineStageTime).count() >= 1000) {
                // Second message (Y/N prompt) after ~1 second
                log.add(MessageType::Info, "Pray at the shrine? (Y/N)");
                shrineMessageStage = 2;
                shrineStageTime = now;
            } else if (shrineMessageStage == 2 &&
                       duration_cast<milliseconds>(now - shrineStageTime).count() >= 1000) {
                // Third message (warning about negative results) after another ~1 second
                log.add(MessageType::Warning,
                        "Tip: Shrines can bless or curse you. There's a chance of a negative effect.");
                shrineMessageStage = 3; // done
            }
        } else {
            // Reset shrine staged messages when prompt is not active
            shrineMessageStage = -1;
        }
        
        frameCount++;
        // IMPROVED: Use named constant for frame log interval
        if (frameCount % game_constants::UI_FRAME_LOG_INTERVAL == 0) {
            LOG_DEBUG("Frame " + std::to_string(frameCount) + " - Player at (" + 
                      std::to_string(player.get_position().x) + "," + 
                      std::to_string(player.get_position().y) + ") HP: " +
                      std::to_string(player.get_stats().hp) + "/" +
                      std::to_string(player.get_stats().maxHp));
        }
        
        // Recalculate viewport on each frame (handles terminal resize)
        LOG_OP_START("get_terminal_size");
        termSize = input::get_terminal_size();
        LOG_OP_END("get_terminal_size");
        
        LOG_OP_START("calculate_viewport");
        vpSize = input::calculate_viewport(termSize.width, termSize.height);
        LOG_OP_END("calculate_viewport");
        viewport_w = vpSize.width;
        viewport_h = vpSize.height;
        
        // IMPROVED: Use named constants for UI frame dimensions
        // Calculate centered UI layout (professional approach)
        LOG_OP_START("ui_layout_calculation");
        int mapFrameHeight = viewport_h + game_constants::UI_BORDER_WIDTH;      // map + top/bottom borders
        int statusFrameHeight = game_constants::UI_STATUS_FRAME_HEIGHT;                 // status bar frame
        int messageFrameHeight = game_constants::UI_MESSAGE_FRAME_HEIGHT;                // message log frame
        int totalHeight = mapFrameHeight + statusFrameHeight + messageFrameHeight + game_constants::UI_BORDER_WIDTH; // +spacing
        int totalWidth = viewport_w + game_constants::UI_BORDER_WIDTH;           // map + left/right borders
        
        // Center on terminal
        int mapStartRow = std::max(1, (termSize.height - totalHeight) / 2);
        int mapStartCol = std::max(1, (termSize.width - totalWidth) / 2);
        
        int statusRow = mapStartRow + mapFrameHeight + 1;
        int msgRow = statusRow + statusFrameHeight + 1;
        LOG_OP_END("ui_layout_calculation");
        
        // Tab-based UI view system (declare before rendering)
        static UIView currentView = UIView::MAP;
        static int viewScrollOffset = 0;
        static int invSel = 0;
        
        // Legacy inventory open state (for backward compatibility)
        bool invOpen = (currentView == UIView::INVENTORY);

        LOG_OP_START("ui_clear");
        ui::clear();
        LOG_OP_END("ui_clear");
        
        // Only draw map when in MAP view (prevents flicker)
        if (currentView == UIView::MAP) {
            LOG_OP_START("draw_map_viewport");
            // Draw main game viewport (camera-centered, dynamic size)
            draw_map_viewport(dungeon, player, enemies, mapStartRow, mapStartCol, viewport_w, viewport_h);
            LOG_OP_END("draw_map_viewport");
            
            LOG_OP_START("draw_status_bar");
            // Draw framed status bar below map
            ui::draw_status_bar_framed(statusRow, mapStartCol, viewport_w + 2, player, currentDepth);
            LOG_OP_END("draw_status_bar");
            
            LOG_OP_START("draw_message_log");
            // Draw framed message log (increased from 4 to 8 lines)
            log.render_framed(msgRow, mapStartCol, viewport_w + 2, 8);
            LOG_OP_END("draw_message_log");
            
        } else {
            LOG_OP_START("draw_menu_view");
            // Draw menu view overlay (full screen, no map behind)
            int viewWidth = std::min(70, termSize.width - 4);
            int viewHeight = std::min(25, termSize.height - 4);
            int viewRow = std::max(2, (termSize.height - viewHeight) / 2);
            int viewCol = std::max(2, (termSize.width - viewWidth) / 2);
            
            switch (currentView) {
                case UIView::INVENTORY:
                    ui::draw_full_inventory_view(viewRow, viewCol, viewWidth, viewHeight, 
                                                  player, invSel, viewScrollOffset);
                    break;
                case UIView::STATS:
                    ui::draw_stats_view(viewRow, viewCol, viewWidth, viewHeight, 
                                        player, currentDepth, totalKillCount);
                    break;
                case UIView::EQUIPMENT:
                    ui::draw_equipment_view(viewRow, viewCol, viewWidth, viewHeight, player);
                    break;
                case UIView::MESSAGE_LOG:
                    ui::draw_message_log_view(viewRow, viewCol, viewWidth, viewHeight,
                                              log, viewScrollOffset);
                    break;
                default:
                    break;
            }
            LOG_OP_END("draw_menu_view");
        }
        
        // Draw contextual tips box (always visible, based on current view and context)
        {
            std::string tipText;
            
            if (currentView == UIView::MAP) {
                // Check if player has low HP and healing potions - show reminder first
                int currentHp = player.get_stats().hp;
                int maxHp = player.get_stats().maxHp;
                float hpPercent = (maxHp > 0) ? (static_cast<float>(currentHp) / static_cast<float>(maxHp)) : 0.0f;
                
                // Check if player has healing potions
                bool hasHealingPotion = false;
                for (const auto& item : player.inventory()) {
                    if (item.isConsumable && item.healAmount > 0) {
                        hasHealingPotion = true;
                        break;
                    }
                }
                
                // Show low HP reminder if HP is below 50% and has potions
                if (hpPercent < 0.5f && hasHealingPotion) {
                    tipText = "⚠ Low HP! Press 'i' to open inventory, then 'U' to use a healing potion.";
                } else {
                    // Check for better unequipped items
                    bool hasBetterItem = false;
                    std::string betterItemName;
                    std::string betterItemType;
                    
                    const auto& equipment = player.get_equipment();
                    
                    // Check inventory for better items
                    for (const auto& invItem : player.inventory()) {
                        if (!invItem.isEquippable) continue;
                        
                        bool isBetter = false;
                        
                        // For weapons, check both Weapon and Offhand slots (dual wielding)
                        if (invItem.type == ItemType::Weapon) {
                            // Check main weapon slot
                            auto weaponIt = equipment.find(EquipmentSlot::Weapon);
                            if (weaponIt != equipment.end()) {
                                const Item& equippedWeapon = weaponIt->second;
                                if (invItem.attackBonus > equippedWeapon.attackBonus) {
                                    isBetter = true;
                                } else if (invItem.attackBonus == equippedWeapon.attackBonus &&
                                          static_cast<int>(invItem.rarity) > static_cast<int>(equippedWeapon.rarity)) {
                                    isBetter = true;
                                }
                            } else {
                                // No weapon equipped, this is better
                                isBetter = true;
                            }
                            
                            // Also check offhand if main hand is occupied
                            if (!isBetter) {
                                auto offhandIt = equipment.find(EquipmentSlot::Offhand);
                                if (offhandIt != equipment.end()) {
                                    const Item& equippedOffhand = offhandIt->second;
                                    if (invItem.attackBonus > equippedOffhand.attackBonus) {
                                        isBetter = true;
                                    } else if (invItem.attackBonus == equippedOffhand.attackBonus &&
                                              static_cast<int>(invItem.rarity) > static_cast<int>(equippedOffhand.rarity)) {
                                        isBetter = true;
                                    }
                                } else if (weaponIt != equipment.end()) {
                                    // Main hand has weapon, offhand is empty - this weapon is better than nothing
                                    isBetter = true;
                                }
                            }
                        } else {
                            // For armor, check the specific slot
                            auto equippedIt = equipment.find(invItem.slot);
                            if (equippedIt != equipment.end()) {
                                const Item& equippedItem = equippedIt->second;
                                if (invItem.defenseBonus > equippedItem.defenseBonus) {
                                    isBetter = true;
                                } else if (invItem.defenseBonus == equippedItem.defenseBonus &&
                                          static_cast<int>(invItem.rarity) > static_cast<int>(equippedItem.rarity)) {
                                    isBetter = true;
                                }
                            } else {
                                // Slot is empty, any equippable item is better than nothing
                                isBetter = true;
                            }
                        }
                        
                        if (isBetter) {
                            hasBetterItem = true;
                            betterItemName = invItem.name;
                            betterItemType = (invItem.type == ItemType::Weapon) ? "weapon" : "armor";
                            break;  // Show first better item found
                        }
                    }
                    
                    if (hasBetterItem) {
                        tipText = "💡 Better " + betterItemType + " available: " + betterItemName + "! Press 'i' to equip.";
                    } else {
                        // Map view: movement / basic interaction tips
                        TileType tile = dungeon.get_tile(player.get_position().x, player.get_position().y);
                        if (tile == TileType::StairsDown) {
                            tipText = "Standing on stairs (>): Press '>' or Shift+'.' to descend.";
                        } else if (tile == TileType::Shrine) {
                            tipText = "At a shrine (_): Press 'e' to interact, then Y/N (Shift+'.' works for '>').";
                        } else {
                            tipText = "Move: WASD / Arrows | Attack: walk into enemy | Inventory: i | Views: TAB | Quit: q";
                        }
                    }
                }
            } else if (currentView == UIView::INVENTORY) {
                tipText = "Inventory: W/S to navigate, E equip, U use, D drop, ESC/q or i to return to map.";
            } else if (currentView == UIView::STATS) {
                tipText = "Stats: Review your character. Use TAB for next view, ESC/q to return to map.";
            } else if (currentView == UIView::EQUIPMENT) {
                tipText = "Equipment: See what you have equipped. TAB for next view, ESC/q to return to map.";
            } else if (currentView == UIView::MESSAGE_LOG) {
                tipText = "Messages: Review history. W/S to scroll, TAB for next view, ESC/q to return to map.";
            }

            if (!tipText.empty()) {
                // Place tips further below the message log frame when possible
                int tipRow = msgRow + messageFrameHeight + 10;
                // Fallback to bottom of screen if there isn't enough space
                if (tipRow + 2 >= termSize.height) {
                    tipRow = std::max(1, termSize.height - 3);
                }

                int tipCol = mapStartCol;
                // Align tips box width exactly with the message log frame when possible
                int maxBoxWidth = viewport_w + 2;  // message log width
                // Ensure we don't overflow terminal width
                maxBoxWidth = std::min(maxBoxWidth, termSize.width - tipCol - 1);

                int boxWidth = maxBoxWidth;
                if (boxWidth < 10) boxWidth = std::min(10, maxBoxWidth);
                
                ui::draw_box_single(tipRow, tipCol, boxWidth, 3, constants::color_frame_main);
                ui::move_cursor(tipRow + 1, tipCol + 2);
                // Truncate tipText if it does not fit in the box
                int maxTextWidth = boxWidth - 4;
                if (maxTextWidth > 0) {
                    std::cout << tipText.substr(0, static_cast<size_t>(maxTextWidth));
                }
            }
        }

        LOG_OP_START("cout_flush");
        // PHASE 3: Check output stream health before flush
        if (!std::cout.good()) {
            LOG_WARN("Main loop: std::cout is in bad state before flush - attempting to clear");
            std::cout.clear();
            if (!std::cout.good()) {
                LOG_ERROR("Main loop: Failed to recover std::cout state");
            }
        }
        std::cout.flush();
        LOG_OP_END("cout_flush");

        LOG_DEBUG("Waiting for input...");
        LOG_OP_START("read_key_nonblocking");
        auto inputStart = std::chrono::steady_clock::now();
        int key = input::read_key_nonblocking();
        auto inputEnd = std::chrono::steady_clock::now();
        auto inputDuration = std::chrono::duration_cast<std::chrono::milliseconds>(inputEnd - inputStart);
        LOG_OP_END("read_key_nonblocking");
        
        // Warn if input reading takes suspiciously long (non-blocking should be fast)
        if (inputDuration.count() > 50) {
            LOG_WARN("read_key_nonblocking took " + std::to_string(inputDuration.count()) + "ms - should be < 1ms for non-blocking");
        }
        
        if (key == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        LOG_DEBUG("Key pressed: " + std::to_string(key) + " ('" + (key >= 32 && key < 127 ? std::string(1, static_cast<char>(key)) : "ctrl") + "')");
        
        // Restore energy at start of player turn (for combat actions)
        // Energy system removed
        
        bool playerIncapacitated = (currentView == UIView::MAP) &&
            (player.has_status(StatusType::Freeze) || player.has_status(StatusType::Stun));
        if (playerIncapacitated) {
            log.add(MessageType::Warning, glyphs::warning() + std::string(" You are incapacitated and cannot act!"));
        }
        
        if (!playerIncapacitated) {
        switch (key) {
            case 'q':
                if (currentView != UIView::MAP) {
                    currentView = UIView::MAP;  // ESC-like behavior: return to map
                } else {
                    running = false;
                }
                break;
            case '\t': {  // TAB key - cycle through views
                int v = static_cast<int>(currentView);
                v = (v + 1) % 5;  // 5 views: MAP, INVENTORY, STATS, EQUIPMENT, MESSAGE_LOG
                currentView = static_cast<UIView>(v);
                viewScrollOffset = 0;
                invSel = 0;
                log.add(MessageType::Info, "Tip: Use TAB to cycle views, ESC/q to return to the map.");
                break;
            }
            case 27: {  // ESC key - return to map view
                currentView = UIView::MAP;
                break;
            }
            case 'i':
            case 'I': {
                if (currentView == UIView::INVENTORY) {
                    currentView = UIView::MAP;
                    log.add(MessageType::Info, "Tip: You can reopen your inventory anytime with 'i'.");
                } else {
                    currentView = UIView::INVENTORY;
                    log.add(MessageType::Info, "Tip: Use W/S, E, U, D to manage items while inventory is open.");
                }
                invSel = 0;
                break;
            }
            case 'e':
            case 'E': {
                if (currentView == UIView::INVENTORY) {
                    if (!player.inventory().empty()) {
                        size_t idx = static_cast<size_t>(std::clamp(invSel, 0, static_cast<int>(player.inventory().size()) - 1));
                        player.equip_item(idx);
                        log.add(MessageType::Info, "Tip: Equipped gear boosts your stats. Press 'e' on another item to swap.");
                    }
                    break;
                }
                // Check for shrine interaction (non-blocking, use message log + tips)
                if (currentView == UIView::MAP) {
                    Position pos = player.get_position();
                    if (dungeon.get_tile(pos.x, pos.y) == TileType::Shrine) {
                        if (!shrinePromptActive) {
                            // Activate staged shrine prompt; messages are shown over time in the main loop
                            shrinePromptActive = true;
                            shrineMessageStage = 0;
                            shrineStageTime = std::chrono::steady_clock::now();
                        }
                    } else {
                        log.add(MessageType::Info, "Nothing to interact with here.");
                        log.add(MessageType::Info, "Tip: Stand on a shrine (_) or other special tiles before pressing 'e'.");
                    }
                }
                break;
            }
            case 'u':
            case 'U': {
                if (currentView == UIView::INVENTORY) {
                    if (!player.inventory().empty()) {
                        size_t idx = static_cast<size_t>(std::clamp(invSel, 0, static_cast<int>(player.inventory().size()) - 1));
                        player.use_consumable(idx);
                    }
                    break;
                }
                break;
            }
            case 'd':
            case 'D': {
                if (currentView == UIView::INVENTORY) {
                    if (!player.inventory().empty()) {
                        size_t idx = static_cast<size_t>(std::clamp(invSel, 0, static_cast<int>(player.inventory().size()) - 1));
                        auto& inv = player.inventory();
                        inv[idx] = inv.back();
                        inv.pop_back();
                        if (invSel >= static_cast<int>(inv.size())) invSel = static_cast<int>(inv.size()) - 1;
                        if (invSel < 0) invSel = 0;
                    }
                } else if (currentView == UIView::MAP) {
                    try_move_or_attack(player, enemies, dungeon, log, 1, 0, rng);
                }
                break;
            }
            case 'y':
            case 'Y': {
                if (shrinePromptActive) {
                    // Resolve shrine blessing via helper: get_random_blessing/apply_blessing
                    shrine::BlessingResult result = shrine::get_random_blessing(rng);
                    log.add(MessageType::Info, result.description);
                    shrine::apply_blessing(player, result.type, log);
                    shrinePromptActive = false;
                    // Turn shrine into floor after interaction
                    Position pos = player.get_position();
                    if (dungeon.get_tile(pos.x, pos.y) == TileType::Shrine) {
                        dungeon.set_tile(pos.x, pos.y, TileType::Floor);
                    }
                    break;
                }
                // Fall through to normal movement if no shrine prompt is active
            }
            case 'w':
            case 'W': {
                if (currentView == UIView::INVENTORY || currentView == UIView::MESSAGE_LOG) { 
                    invSel = std::max(0, invSel - 1); 
                    viewScrollOffset = std::max(0, viewScrollOffset - 1);
                    break; 
                }
                if (currentView != UIView::MAP) break;  // Ignore in other views
                try_move_or_attack(player, enemies, dungeon, log, 0, -1, rng);
                break;
            }
            case 's':
            case 'S': {
                if (currentView == UIView::INVENTORY || currentView == UIView::MESSAGE_LOG) { 
                    invSel = std::min(invSel + 1, static_cast<int>(player.inventory().size()) - 1); 
                    viewScrollOffset++;
                    break; 
                }
                if (currentView != UIView::MAP) break;  // Ignore in other views
                try_move_or_attack(player, enemies, dungeon, log, 0, 1, rng);
                break;
            }
            case 'a':
            case 'A':
            case input::KEY_LEFT: {
                if (currentView != UIView::MAP) break;
                try_move_or_attack(player, enemies, dungeon, log, -1, 0, rng);
                break;
            }
            case input::KEY_UP: {
                if (currentView == UIView::INVENTORY || currentView == UIView::MESSAGE_LOG) { 
                    invSel = std::max(0, invSel - 1); 
                    viewScrollOffset = std::max(0, viewScrollOffset - 1);
                    break; 
                }
                if (currentView != UIView::MAP) break;
                try_move_or_attack(player, enemies, dungeon, log, 0, -1, rng);
                break;
            }
            case input::KEY_DOWN: {
                if (currentView == UIView::INVENTORY || currentView == UIView::MESSAGE_LOG) { 
                    invSel = std::min(invSel + 1, static_cast<int>(player.inventory().size()) - 1); 
                    viewScrollOffset++;
                    break; 
                }
                if (currentView != UIView::MAP) break;
                try_move_or_attack(player, enemies, dungeon, log, 0, 1, rng);
                break;
            }
            case input::KEY_RIGHT: {
                if (currentView != UIView::MAP) break;
                try_move_or_attack(player, enemies, dungeon, log, 1, 0, rng);
                break;
            }
            case 'r':
            case 'R': {
                // Reset: delete saves and restart - show confirmation prompt
                // Get terminal size for centering
                auto confirmTermSize = input::get_terminal_size();
                int confirmTermW = confirmTermSize.width;
                int confirmTermH = confirmTermSize.height;
                
                // Clear and redraw with the prompt visible
                std::cout << "\033[2J\033[H";  // Clear screen
                
                // Draw a centered confirmation box
                int confirmBoxRow = confirmTermH / 2 - 2;
                int confirmBoxCol = confirmTermW / 2 - 20;
                
                std::cout << "\033[" << confirmBoxRow << ";" << confirmBoxCol << "H";
                std::cout << "\033[1;31m" << glyphs::box_dbl_tl();
                for (int i = 0; i < 38; i++) std::cout << glyphs::box_dbl_h();
                std::cout << glyphs::box_dbl_tr() << "\033[0m";
                
                std::cout << "\033[" << (confirmBoxRow + 1) << ";" << confirmBoxCol << "H";
                std::cout << "\033[1;31m" << glyphs::box_dbl_v() << "\033[0m";
                std::cout << "\033[1;33m" << "  " << glyphs::warning() << " DELETE ALL SAVES AND RESTART?  " << "\033[0m";
                std::cout << "\033[1;31m" << glyphs::box_dbl_v() << "\033[0m";
                
                std::cout << "\033[" << (confirmBoxRow + 2) << ";" << confirmBoxCol << "H";
                std::cout << "\033[1;31m" << glyphs::box_dbl_v() << "\033[0m";
                std::cout << "                                      ";
                std::cout << "\033[1;31m" << glyphs::box_dbl_v() << "\033[0m";
                
                std::cout << "\033[" << (confirmBoxRow + 3) << ";" << confirmBoxCol << "H";
                std::cout << "\033[1;31m" << glyphs::box_dbl_v() << "\033[0m";
                std::cout << "      Press [Y] to confirm            ";
                std::cout << "\033[1;31m" << glyphs::box_dbl_v() << "\033[0m";
                
                std::cout << "\033[" << (confirmBoxRow + 4) << ";" << confirmBoxCol << "H";
                std::cout << "\033[1;31m" << glyphs::box_dbl_v() << "\033[0m";
                std::cout << "      Press any other key to cancel   ";
                std::cout << "\033[1;31m" << glyphs::box_dbl_v() << "\033[0m";
                
                std::cout << "\033[" << (confirmBoxRow + 5) << ";" << confirmBoxCol << "H";
                std::cout << "\033[1;31m" << glyphs::box_dbl_bl();
                for (int i = 0; i < 38; i++) std::cout << glyphs::box_dbl_h();
                std::cout << glyphs::box_dbl_br() << "\033[0m";
                
                std::cout.flush();
                
                // Wait for confirmation input
                int confirm = input::read_key_blocking();
                if (confirm == 'y' || confirm == 'Y') {
                    delete_all_saves();
                    // Show success message
                    std::cout << "\033[" << (confirmBoxRow + 3) << ";" << (confirmBoxCol + 2) << "H";
                    std::cout << "\033[1;32m" << "  OK Saves deleted! Restart to begin." << "\033[0m";
                    std::cout.flush();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                    running = false;
                } else {
                    log.add(MessageType::Info, "Reset cancelled.");
                }
                break;
            }
            case 'g':
            case 'G': {
                // Debug: spawn test items - DISABLED
                // spawn_test_items(player, log);
                break;
            }
            case 'n':
            case 'N': {
                // Debug: spawn goblin archer near player
                Position pp = player.get_position();
                bool spawned = false;
                // Try to find a walkable spot within search radius
                for (int dy = -game_constants::BOSS_SPAWN_SEARCH_RADIUS; dy <= game_constants::BOSS_SPAWN_SEARCH_RADIUS && !spawned; ++dy) {
                    for (int dx = -game_constants::BOSS_SPAWN_SEARCH_RADIUS; dx <= game_constants::BOSS_SPAWN_SEARCH_RADIUS && !spawned; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = pp.x + dx;
                        int ny = pp.y + dy;
                        if (dungeon.in_bounds(nx, ny) && dungeon.is_walkable(nx, ny)) {
                            Enemy e(EnemyType::Archer);  // Spawn goblin archer
                            e.set_position(nx, ny);
                            // Scale stats with depth AND difficulty
                            int baseHp = e.stats().maxHp + currentDepth * game_constants::ENEMY_HP_SCALING_PER_DEPTH;
                            int baseAtk = e.stats().attack + currentDepth / game_constants::ENEMY_ATK_SCALING_DIVISOR;
                            e.stats().maxHp = static_cast<int>(baseHp * params.enemyHpMultiplier);
                            e.stats().hp = e.stats().maxHp;
                            e.stats().attack = static_cast<int>(baseAtk * params.enemyDamageMultiplier);
                            enemies.push_back(e);
                            log.add(MessageType::Warning, "A " + e.name() + " appears!");
                            spawned = true;
                        }
                    }
                }
                if (!spawned) {
                    log.add(MessageType::Info, "The dungeon remains quiet...");
                }
                break;
            }
            case '?': {
                // In-game help system
                int helpPage = 0;
                while (true) {
                    ui::draw_help_screen(helpPage);
                    int hkey = input::read_key_blocking();
                    if (hkey == input::KEY_LEFT || hkey == 'a' || hkey == 'A') {
                        helpPage = (helpPage - 1 + ui::help_page_count) % ui::help_page_count;
                    } else if (hkey == input::KEY_RIGHT || hkey == 'd' || hkey == 'D') {
                        helpPage = (helpPage + 1) % ui::help_page_count;
                    } else {
                        break; // Any other key exits help
                    }
                }
                break;
            }
            case '>': {
                // Descend stairs
                Position p = player.get_position();
                if (dungeon.get_tile(p.x, p.y) == TileType::StairsDown) {
                    // IMPROVED: Use named constant for final boss floor
                if (currentDepth >= game_constants::BOSS_FLOOR_3) {
                        // Victory! Player has reached the deepest floor
                        log.add(MessageType::Level, "You have conquered the depths!");
                        log.add(MessageType::Info, "Tip: You can always go down stairs by standing on '>' and pressing '>'.");
                        // Will show victory screen after loop
                        player.get_stats().hp = -999; // Signal victory (handled specially)
                        running = false;
                    } else {
                        // Generate new floor
                        currentDepth++;
                        player.set_depth(currentDepth);  // Update player depth for attack bonus
                        log.add(MessageType::Level, "You descend to depth " + std::to_string(currentDepth) + "...");
                        log.add(MessageType::Info, "Tip: Explore each floor for loot and shrines before going deeper.");
                        
                        // Play level transition sound and animation
                        ui::play_level_up_sound();
                        ui::wipe_transition_down();
                        
                        // Dynamic map sizing based on depth: width = 30 + depth*10, height = 15 + depth*5
                        int newMapWidth = 30 + currentDepth * 10;
                        int newMapHeight = 15 + currentDepth * 5;
                        // Recreate dungeon with new size
                        dungeon = Dungeon(newMapWidth, newMapHeight);
                        
                        // New seed based on base seed + depth
                        unsigned int newSeed = seed + static_cast<unsigned int>(currentDepth);
                        dungeon.generate(newSeed, start, stairsDown, currentDepth);
                        
                        // Initialize traps for the new floor
                        initialize_floor_traps(dungeon, rng);
                        
                        // Tick shrine blessings when descending
                        shrine::tick_blessings(player, log);
                        
                        // Place player at new start
                        player.set_position(start.x, start.y);
                        
                        // Clear old enemies and spawn new ones (with difficulty scaling)
                        enemies.clear();
                        
                        // Check if this is a boss floor
                        if (is_boss_floor(currentDepth)) {
                            spawn_boss(enemies, dungeon, log, currentDepth, rng, params);
                            // Spawn fewer regular enemies on boss floors
                            int numEnemies = 1 + currentDepth / 3;
                            for (int i = 0; i < numEnemies; ++i) {
                                EnemyType type = get_enemy_type_for_depth(currentDepth, rng);
                                Enemy e(type);
                                // IMPROVED: Use named constant for max spawn attempts
                                for (int attempts = 0; attempts < game_constants::MAX_SPAWN_ATTEMPTS; ++attempts) {
                                    int ex = std::uniform_int_distribution<int>(1, dungeon.width() - 2)(rng);
                                    int ey = std::uniform_int_distribution<int>(1, dungeon.height() - 2)(rng);
                                    if (dungeon.is_walkable(ex, ey) && (ex != start.x || ey != start.y)) {
                                        e.set_position(ex, ey);
                        // IMPROVED: Use named constants for enemy scaling
                        int baseHp = e.stats().maxHp + currentDepth * game_constants::ENEMY_HP_SCALING_PER_DEPTH;
                        int baseAtk = e.stats().attack + currentDepth / game_constants::ENEMY_ATK_SCALING_DIVISOR;
                                        e.stats().maxHp = static_cast<int>(baseHp * params.enemyHpMultiplier);
                                        e.stats().hp = e.stats().maxHp;
                                        e.stats().attack = static_cast<int>(baseAtk * params.enemyDamageMultiplier);
                                        enemies.push_back(e);
                                        break;
                                    }
                                }
                            }
                        } else {
                            // Regular floor spawning
                            int numEnemies = 1 + currentDepth / 3;
                            for (int i = 0; i < numEnemies; ++i) {
                                EnemyType type = get_enemy_type_for_depth(currentDepth, rng);
                                Enemy e(type);
                                // IMPROVED: Use named constant for max spawn attempts
                                for (int attempts = 0; attempts < game_constants::MAX_SPAWN_ATTEMPTS; ++attempts) {
                                    int ex = std::uniform_int_distribution<int>(1, dungeon.width() - 2)(rng);
                                    int ey = std::uniform_int_distribution<int>(1, dungeon.height() - 2)(rng);
                                    if (dungeon.is_walkable(ex, ey) && (ex != start.x || ey != start.y)) {
                                        e.set_position(ex, ey);
                        // IMPROVED: Use named constants for enemy scaling
                        int baseHp = e.stats().maxHp + currentDepth * game_constants::ENEMY_HP_SCALING_PER_DEPTH;
                        int baseAtk = e.stats().attack + currentDepth / game_constants::ENEMY_ATK_SCALING_DIVISOR;
                                        e.stats().maxHp = static_cast<int>(baseHp * params.enemyHpMultiplier);
                                        e.stats().hp = e.stats().maxHp;
                                        e.stats().attack = static_cast<int>(baseAtk * params.enemyDamageMultiplier);
                                        enemies.push_back(e);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    log.add(MessageType::Info, "There are no stairs here.");
                }
                break;
            }
            default:
                break;
        }
        } // end !playerIncapacitated
        if (invOpen) {
            // While inventory open, skip enemy turns but still tick statuses
            player.tick_statuses();
            player.tick_cooldowns();
            continue;
        }
        
        // Check for special tile events after movement
        Position playerPos = player.get_position();
        TileType currentTile = dungeon.get_tile(playerPos.x, playerPos.y);
        if (currentTile == TileType::Trap) {
            // Trap: deal damage and convert to floor
            int trapDamage = 2 + currentDepth;
            player.get_stats().hp -= trapDamage;
            // Clamp HP to 0 minimum (player dies if HP <= 0, checked elsewhere)
            if (player.get_stats().hp < 0) {
                player.get_stats().hp = 0;
            }
            log.add(MessageType::Damage, "You triggered a trap! (-" + std::to_string(trapDamage) + " HP)");
            dungeon.set_tile(playerPos.x, playerPos.y, TileType::Floor);
        } else if (currentTile == TileType::Shrine) {
            // Shrine: heal or buff
            std::uniform_int_distribution<int> shrineRoll(0, 100);
            int roll = shrineRoll(rng);
            if (roll < 50) {
                // Heal
                int healAmt = 5 + currentDepth;
                player.get_stats().hp = std::min(player.get_stats().hp + healAmt, player.get_stats().maxHp);
                log.add(MessageType::Heal, "The shrine heals you! (+" + std::to_string(healAmt) + " HP)");
            } else {
                // Haste buff
                player.apply_status(StatusEffect{StatusType::Haste, 10, 3});
                log.add(MessageType::Heal, "The shrine hastens you! (+3 SPD for 10 turns)");
            }
            dungeon.set_tile(playerPos.x, playerPos.y, TileType::Floor);
        } else if (currentTile == TileType::Water) {
            // Water: slows movement (message only, actual slow would need turn system)
            static bool waterMsgShown = false;
            if (!waterMsgShown) {
                log.add(MessageType::Info, "You wade through the water...");
                waterMsgShown = true;
            }
        } else if (currentTile == TileType::Lava) {
            // Lava: massive damage (shouldn't be walkable, but just in case)
            int lavaDamage = 20 + currentDepth * 2;
            player.get_stats().hp -= lavaDamage;
            // Clamp HP to 0 minimum (player dies if HP <= 0, checked elsewhere)
            if (player.get_stats().hp < 0) {
                player.get_stats().hp = 0;
            }
            log.add(MessageType::Damage, "You step into LAVA! (-" + std::to_string(lavaDamage) + " HP)");
        } else if (currentTile == TileType::Chasm) {
            // Chasm: instant death (shouldn't be walkable)
            player.get_stats().hp = 0;
            log.add(MessageType::Death, "You fall into the endless chasm!");
        }
        
        // Skip game logic when in menu views (only process when on MAP)
        if (currentView != UIView::MAP) {
            continue;  // Don't process enemy turns or game logic in menu views
        }

        // Track player kiting behavior
        Position newPlayerPos = player.get_position();
        for (auto& en : enemies) {
            int oldDist = std::abs(prevPlayerPos.x - en.get_position().x) + std::abs(prevPlayerPos.y - en.get_position().y);
            int newDist = std::abs(newPlayerPos.x - en.get_position().x) + std::abs(newPlayerPos.y - en.get_position().y);
            if (newDist > oldDist) {
                en.knowledge().timesPlayerKited += 1;
                en.knowledge().record_action(4); // Kite action
            } else if (newDist < oldDist) {
                en.knowledge().record_action(1); // Approach/melee action
            }
        }
        prevPlayerPos = newPlayerPos;

        // Enemies take a turn (movement only - combat handled by tactical combat mode)
        LOG_DEBUG("Processing " + std::to_string(enemies.size()) + " enemy turns");
        // IMPROVED: Use range-based for loop with index tracking where needed
        size_t enemyIndex = 0;
        for (auto& en : enemies) {
            LOG_DEBUG("Enemy " + std::to_string(enemyIndex) + " (" + en.name() + ") at (" + 
                      std::to_string(en.get_position().x) + "," + 
                      std::to_string(en.get_position().y) + ") taking turn");
            LOG_OP_START("ai_take_turn_" + std::to_string(enemyIndex));
            ai::take_turn(en, player, dungeon, log);
            LOG_OP_END("ai_take_turn_" + std::to_string(enemyIndex));
            
            // Check if enemy moved adjacent to player - enter tactical combat mode
            const Position ep = en.get_position(); // IMPROVED: Use const for read-only position
            const Position pp = player.get_position(); // IMPROVED: Use const for read-only position
            if (std::abs(ep.x - pp.x) + std::abs(ep.y - pp.y) == 1) {
                LOG_DEBUG("Enemy " + en.name() + " is adjacent to player - entering tactical combat");
                lastEnemyAttacker = en.name();  // Track for death message
                
                // Enter tactical combat mode (replaces old automatic melee system)
                LOG_OP_START("enter_combat_mode_from_enemy_turn");
                bool playerWon = combat::enter_combat_mode(player, en, dungeon, log);
                LOG_OP_END("enter_combat_mode_from_enemy_turn");
                (void)playerWon;  // Result handled by main loop (death check)
            }
            enemyIndex++; // IMPROVED: Increment index after processing each enemy
        }

        // Remove dead enemies
        LOG_DEBUG("Checking for dead enemies");
        for (auto it = enemies.begin(); it != enemies.end();) {
            if (it->stats().hp <= 0) {
                LOG_INFO("Enemy " + it->name() + " died, dropping loot");
                
                // Special handling for Vengeful Spirit (corpse run)
                if (it->enemy_type() == EnemyType::CorpseEnemy) {
                    // Recover items from previous death
                    // FIXED: Add timing to detect slow file I/O
                    LOG_OP_START("load_corpse_save");
                    GameState corpse{};
                    bool loadSuccess = fileio::load_from_slot(corpse, 2);
                    LOG_OP_END("load_corpse_save");
                    
                    if (loadSuccess) {
                        int recoveredCount = 0;
                        for (const auto& item : corpse.player.inventory()) {
                            player.inventory().push_back(item);
                            recoveredCount++;
                        }
                        if (recoveredCount > 0) {
                            log.add(MessageType::Loot, "Recovered " + std::to_string(recoveredCount) + " items from your past self!");
                        }
                        // Clear the corpse save after recovery
                        LOG_OP_START("delete_corpse_save");
                        fileio::delete_slot(2);
                        LOG_OP_END("delete_corpse_save");
                        log.add(MessageType::Info, "Your spirit is at peace.");
                    } else {
                        LOG_WARN("Failed to load corpse save - file may be missing or corrupted");
                    }
                } else {
                    // Normal enemy loot - drop 1-3 items per enemy
                    std::uniform_int_distribution<int> itemCountRoll(0, 100);
                    int roll = itemCountRoll(rng);
                    int itemCount = 1;  // 70% chance for 1 item
                    if (roll >= 70 && roll < 90) {
                        itemCount = 2;  // 20% chance for 2 items
                    } else if (roll >= 90) {
                        itemCount = 3;  // 10% chance for 3 items
                    }
                    
                    std::vector<Item> droppedItems;
                    std::string lootMessage = "Loot gained: ";
                    
                    for (int i = 0; i < itemCount; ++i) {
                        // Use loot:: functions for better item generation
                        std::uniform_int_distribution<int> itemTypeRoll(0, 100);
                        int typeRoll = itemTypeRoll(rng);
                        Item loot;
                        
                        if (typeRoll < 40) {
                            // 40% chance for weapon
                            loot = loot::generate_weapon(currentDepth, rng);
                        } else if (typeRoll < 70) {
                            // 30% chance for armor
                            loot = loot::generate_armor(currentDepth, rng);
                        } else {
                            // 30% chance for consumable
                            loot = loot::generate_consumable(currentDepth, rng);
                        }
                        
                    player.inventory().push_back(loot);
                        droppedItems.push_back(loot);
                        
                        if (i > 0) lootMessage += ", ";
                        lootMessage += loot.name;
                    }
                    
                    lootMessage += ".";
                    log.add(MessageType::Loot, lootMessage);
                    log.add(MessageType::Info, "Tip: Walk over items to pick them up, then press 'i' to see them in your inventory.");
                }
                
                totalKillCount++;  // Track kills for stats
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }

        // Auto-respawn: DISABLED - was used for testing, now removed
        // if (enemies.empty()) {
        //     LOG_INFO("All enemies dead, spawning new enemy");
        //     log.add(MessageType::Warning, "The dungeon stirs... a new foe approaches!");
        //     spawn_enemy_near_player(enemies, player, dungeon, log, currentDepth, rng, params);
        // }

        // Tick statuses at end of full turn
        LOG_DEBUG("Ticking player statuses");
        player.tick_statuses();
        player.tick_cooldowns();  // Decrement ability cooldowns

        if (player.get_stats().hp <= 0 && player.get_stats().hp != -999) {
            if (!corpseSaved) {
                save_corpse_state(player, difficulty, currentDepth, seed, stairsDown);
                corpseSaved = true;
                log.add(MessageType::Warning, "Your fallen gear lingers as a vengeful spirit!");
            }
            LOG_INFO("Player died - HP: " + std::to_string(player.get_stats().hp));
            log.add(MessageType::Death, "You died.");
            running = false;
        }
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        long long frameMs = frameDuration.count();
        
        if (frameMs > 100) {
            LOG_WARN("SLOW FRAME: Game loop iteration took " + std::to_string(frameMs) + "ms");
        }
        
        LOG_OP_END("game_loop_iteration");
        
        // Check for slow frames (after logging)
        auto frameEndCheck = std::chrono::steady_clock::now();
        auto frameDurationCheck = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndCheck - frameStart);
        if (frameDurationCheck.count() > 100) {
            LOG_WARN("Slow frame detected: " + std::to_string(frameDurationCheck.count()) + "ms (frame " + std::to_string(frameCount) + ")");
        }
        
        LOG_DEBUG("End of game loop iteration");
    }

    // Leaderboard integration
    Leaderboard leaderboard;
    leaderboard.load();
    
    // If victory (hp == -999 is our victory signal)
    if (player.get_stats().hp == -999) {
        // Add victory entry to leaderboard
        LeaderboardEntry entry;
        entry.playerName = Player::class_name(player.player_class());
        entry.floorsReached = currentDepth;
        entry.enemiesKilled = totalKillCount;
        entry.goldCollected = 0;  // TODO: Track gold if not already tracked
        entry.className = Player::class_name(player.player_class());
        entry.causeOfDeath = "Victory";
        entry.timestamp = std::time(nullptr);
        entry.seed = seed;
        leaderboard.add_entry(entry);
        
        show_victory_screen(currentDepth, totalKillCount, player, seed, leaderboard);
    }
    // If dead, show game over
    else if (player.get_stats().hp <= 0) {
        // Add death entry to leaderboard
        LeaderboardEntry entry;
        entry.playerName = Player::class_name(player.player_class());
        entry.floorsReached = currentDepth;
        entry.enemiesKilled = totalKillCount;
        entry.goldCollected = 0;  // TODO: Track gold if not already tracked
        entry.className = Player::class_name(player.player_class());
        entry.causeOfDeath = "Slain by " + lastEnemyAttacker;
        entry.timestamp = std::time(nullptr);
        entry.seed = seed;
        leaderboard.add_entry(entry);
        
        show_gameover_screen(currentDepth, totalKillCount, "Slain by " + lastEnemyAttacker, seed, leaderboard);
    }

    const bool playerAlive = player.get_stats().hp > 0 && player.get_stats().hp != -999;
    if (playerAlive) {
        // Rotate previous save to slot 3 (slot 2 reserved for corpse runs)
        GameState previous{};
        if (fileio::load_from_slot(previous, 1)) {
            fileio::save_to_slot(previous, 3);
        }

        GameState state;
        state.difficulty = difficulty;
        state.player = player;
        state.enemies = enemies;
        state.depth = currentDepth;
        state.seed = seed;
        state.stairsDown = stairsDown;
        fileio::save_to_slot(state, 1);
        LOG_INFO("Game saved to slot 1");
    } else {
        fileio::delete_slot(1);
        LOG_INFO("Cleared autosave after completed run");
    }

    LOG_INFO("Game ended - shutting down");
    Logger::instance().shutdown();

    input::disable_raw_mode();
    ui::shutdown();
    return 0;
}


