#pragma once

#include <string>

// ==========================================================
// GLYPH SYSTEM - Unicode/ASCII Dual Mode
// ==========================================================
// All game visuals should use this system for consistent rendering.
// Use --no-unicode flag for ASCII-only mode (compatibility).
// 
// Symbol Reference:
// - Terrain: █ · + > < ≈ ░ △ †
// - Items: 🧪 📜 ⚔ 🛡 💎 💰 ✨
// - Enemies: 👹 🐉 🧟 ☠ 🕷 🦇
// - UI: ❤ ⚔ ◈ » ↑↓←→
// ==========================================================

namespace glyphs {
    // Global settings (set from CLI)
    extern bool use_unicode;
    extern bool use_color;
    
    // Initialize glyph settings
    void init(bool unicode, bool color);
    
    // ==========================================================
    // TERRAIN GLYPHS
    // ==========================================================
    inline const char* wall() { return use_unicode ? "█" : "#"; }
    inline const char* wall_alt() { return use_unicode ? "▓" : "#"; }
    inline const char* floor_tile() { return use_unicode ? "·" : "."; }
    inline const char* corridor() { return use_unicode ? "·" : "."; }
    inline const char* door_closed() { return use_unicode ? "+" : "+"; }
    inline const char* door_open() { return use_unicode ? "'" : "'"; }
    inline const char* door_locked() { return use_unicode ? "⊞" : "+"; }
    inline const char* stairs_down() { return use_unicode ? "▼" : ">"; }
    inline const char* stairs_up() { return use_unicode ? "▲" : "<"; }
    inline const char* water() { return use_unicode ? "≈" : "~"; }
    inline const char* deep_water() { return use_unicode ? "≋" : "~"; }
    inline const char* lava() { return use_unicode ? "≈" : "~"; }
    inline const char* chasm() { return use_unicode ? "░" : " "; }
    inline const char* pit() { return use_unicode ? "○" : "o"; }
    inline const char* trap() { return use_unicode ? "△" : "^"; }
    inline const char* trap_triggered() { return use_unicode ? "▲" : "^"; }
    inline const char* shrine() { return use_unicode ? "†" : "_"; }
    inline const char* altar() { return use_unicode ? "π" : "_"; }
    inline const char* fog() { return use_unicode ? "▒" : "."; }
    inline const char* unexplored() { return use_unicode ? "▓" : " "; }
    inline const char* tree() { return use_unicode ? "♣" : "T"; }
    inline const char* grass() { return use_unicode ? "\"" : "\""; }
    inline const char* rock() { return use_unicode ? "●" : "o"; }
    inline const char* ice() { return use_unicode ? "❄" : "*"; }
    inline const char* fire() { return use_unicode ? "🔥" : "^"; }
    inline const char* fire_tile() { return use_unicode ? "▲" : "^"; }
    
    // ==========================================================
    // ENTITY GLYPHS
    // ==========================================================
    inline const char* player() { return use_unicode ? "@" : "@"; }
    inline const char* player_dead() { return use_unicode ? "☠" : "%"; }
    inline const char* corpse() { return use_unicode ? "☠" : "%"; }
    inline const char* corpse_old() { return use_unicode ? "✝" : "%"; }
    inline const char* npc() { return use_unicode ? "☺" : "@"; }
    inline const char* merchant() { return use_unicode ? "$" : "$"; }
    
    // ==========================================================
    // ENEMY GLYPHS
    // ==========================================================
    // Standard roguelike convention: lowercase = weak, uppercase = strong
    // Letters represent creature type for easy identification
    inline const char* rat() { return "r"; }
    inline const char* spider() { return use_unicode ? "s" : "s"; }
    inline const char* bat() { return "b"; }
    inline const char* goblin() { return "g"; }
    inline const char* kobold() { return "k"; }
    inline const char* orc() { return "o"; }
    inline const char* zombie() { return "z"; }
    inline const char* skeleton() { return "s"; }
    inline const char* ghost() { return use_unicode ? "G" : "G"; }
    inline const char* gnome() { return "G"; }
    inline const char* ogre() { return "O"; }
    inline const char* troll() { return "T"; }
    inline const char* dragon() { return use_unicode ? "D" : "D"; }
    inline const char* lich() { return use_unicode ? "L" : "L"; }
    inline const char* demon() { return use_unicode ? "&" : "&"; }
    inline const char* elemental() { return use_unicode ? "E" : "E"; }
    inline const char* golem() { return use_unicode ? "#" : "#"; }
    inline const char* vengeful_spirit() { return use_unicode ? "Ω" : "C"; }
    inline const char* boss() { return use_unicode ? "Ω" : "B"; }
    
    // ==========================================================
    // ITEM GLYPHS
    // ==========================================================
    // Weapons
    inline const char* weapon() { return use_unicode ? ")" : ")"; }
    inline const char* sword() { return use_unicode ? "†" : ")"; }
    inline const char* dagger() { return use_unicode ? "¡" : ")"; }
    inline const char* axe() { return use_unicode ? "P" : ")"; }
    inline const char* bow() { return use_unicode ? "}" : ")"; }
    inline const char* staff() { return use_unicode ? "/" : "/"; }
    inline const char* spear() { return use_unicode ? "|" : "|"; }
    
    // Armor
    inline const char* armor() { return use_unicode ? "[" : "["; }
    inline const char* helmet() { return use_unicode ? "^" : "["; }
    inline const char* shield() { return use_unicode ? ")" : ")"; }
    inline const char* boots() { return use_unicode ? ";" : "["; }
    inline const char* gloves() { return use_unicode ? "," : "["; }
    inline const char* cloak() { return use_unicode ? "(" : "("; }
    
    // Consumables
    inline const char* potion() { return use_unicode ? "!" : "!"; }
    inline const char* scroll() { return use_unicode ? "?" : "?"; }
    inline const char* food() { return use_unicode ? "%" : "%"; }
    inline const char* herb() { return use_unicode ? "\"" : "\""; }
    
    // Magical
    inline const char* wand() { return use_unicode ? "/" : "/"; }
    inline const char* ring() { return use_unicode ? "=" : "="; }
    inline const char* amulet() { return use_unicode ? "\"" : "\""; }
    inline const char* orb() { return use_unicode ? "0" : "0"; }
    inline const char* gem() { return use_unicode ? "◆" : "*"; }
    
    // Treasure
    inline const char* gold() { return use_unicode ? "$" : "$"; }
    inline const char* gold_pile() { return use_unicode ? "◆" : "$"; }
    inline const char* chest() { return use_unicode ? "=" : "="; }
    inline const char* artifact() { return use_unicode ? "★" : "*"; }
    inline const char* key() { return use_unicode ? "k" : "k"; }
    inline const char* key_ornate() { return use_unicode ? "⚷" : "k"; }
    
    // ==========================================================
    // UI BOX-DRAWING CHARACTERS
    // ==========================================================
    // Double-line box (main frame)
    inline const char* box_dbl_tl() { return use_unicode ? "╔" : "+"; }
    inline const char* box_dbl_tr() { return use_unicode ? "╗" : "+"; }
    inline const char* box_dbl_bl() { return use_unicode ? "╚" : "+"; }
    inline const char* box_dbl_br() { return use_unicode ? "╝" : "+"; }
    inline const char* box_dbl_h() { return use_unicode ? "═" : "-"; }
    inline const char* box_dbl_v() { return use_unicode ? "║" : "|"; }
    inline const char* box_dbl_lt() { return use_unicode ? "╠" : "+"; }
    inline const char* box_dbl_rt() { return use_unicode ? "╣" : "+"; }
    inline const char* box_dbl_tt() { return use_unicode ? "╦" : "+"; }
    inline const char* box_dbl_bt() { return use_unicode ? "╩" : "+"; }
    inline const char* box_dbl_x() { return use_unicode ? "╬" : "+"; }
    
    // Single-line box (panels)
    inline const char* box_sgl_tl() { return use_unicode ? "┌" : "+"; }
    inline const char* box_sgl_tr() { return use_unicode ? "┐" : "+"; }
    inline const char* box_sgl_bl() { return use_unicode ? "└" : "+"; }
    inline const char* box_sgl_br() { return use_unicode ? "┘" : "+"; }
    inline const char* box_sgl_h() { return use_unicode ? "─" : "-"; }
    inline const char* box_sgl_v() { return use_unicode ? "│" : "|"; }
    inline const char* box_sgl_lt() { return use_unicode ? "├" : "+"; }
    inline const char* box_sgl_rt() { return use_unicode ? "┤" : "+"; }
    inline const char* box_sgl_tt() { return use_unicode ? "┬" : "+"; }
    inline const char* box_sgl_bt() { return use_unicode ? "┴" : "+"; }
    inline const char* box_sgl_x() { return use_unicode ? "┼" : "+"; }
    
    // Rounded corners (alternative style)
    inline const char* box_rnd_tl() { return use_unicode ? "╭" : "+"; }
    inline const char* box_rnd_tr() { return use_unicode ? "╮" : "+"; }
    inline const char* box_rnd_bl() { return use_unicode ? "╰" : "+"; }
    inline const char* box_rnd_br() { return use_unicode ? "╯" : "+"; }
    
    // ==========================================================
    // STATUS/UI INDICATORS
    // ==========================================================
    // Health & Resources
    inline const char* heart_full() { return use_unicode ? "♥" : "*"; }
    inline const char* heart_empty() { return use_unicode ? "♡" : "."; }
    inline const char* heart_half() { return use_unicode ? "❤" : "o"; }
    inline const char* mana_full() { return use_unicode ? "◆" : "*"; }
    inline const char* mana_empty() { return use_unicode ? "◇" : "."; }
    
    // Health/Mana Bars
    inline const char* bar_full() { return use_unicode ? "█" : "#"; }
    inline const char* bar_three_quarter() { return use_unicode ? "▓" : "="; }
    inline const char* bar_half() { return use_unicode ? "▒" : "-"; }
    inline const char* bar_quarter() { return use_unicode ? "░" : "."; }
    inline const char* bar_empty() { return use_unicode ? " " : " "; }
    inline const char* bar_left() { return use_unicode ? "[" : "["; }
    inline const char* bar_right() { return use_unicode ? "]" : "]"; }
    
    // Stats
    inline const char* stat_attack() { return use_unicode ? "⚔" : "ATK"; }
    inline const char* stat_defense() { return use_unicode ? "◈" : "DEF"; }
    inline const char* stat_speed() { return use_unicode ? "»" : "SPD"; }
    inline const char* stat_hp() { return use_unicode ? "♥" : "HP"; }
    inline const char* stat_mp() { return use_unicode ? "◆" : "MP"; }
    inline const char* stat_xp() { return use_unicode ? "★" : "XP"; }
    inline const char* stat_level() { return use_unicode ? "▲" : "LV"; }
    
    // Directional arrows
    inline const char* arrow_up() { return use_unicode ? "↑" : "^"; }
    inline const char* arrow_down() { return use_unicode ? "↓" : "v"; }
    inline const char* arrow_left() { return use_unicode ? "←" : "<"; }
    inline const char* arrow_right() { return use_unicode ? "→" : ">"; }
    inline const char* arrow_ne() { return use_unicode ? "↗" : "/"; }
    inline const char* arrow_nw() { return use_unicode ? "↖" : "\\"; }
    inline const char* arrow_se() { return use_unicode ? "↘" : "\\"; }
    inline const char* arrow_sw() { return use_unicode ? "↙" : "/"; }
    
    // Status effects
    inline const char* status_poison() { return use_unicode ? "☠" : "P"; }
    inline const char* status_bleed() { return use_unicode ? "⚫" : "B"; }
    inline const char* status_fortify() { return use_unicode ? "◈" : "F"; }
    inline const char* status_haste() { return use_unicode ? "⚡" : "H"; }
    inline const char* status_regen() { return use_unicode ? "♥" : "R"; }
    inline const char* status_fire() { return use_unicode ? "🔥" : "F"; }
    inline const char* status_ice() { return use_unicode ? "❄" : "I"; }
    inline const char* status_stun() { return use_unicode ? "✦" : "S"; }
    inline const char* status_blind() { return use_unicode ? "◐" : "B"; }
    inline const char* status_invisible() { return use_unicode ? "◌" : "I"; }
    
    // Misc UI
    inline const char* checkmark() { return use_unicode ? "✓" : "v"; }
    inline const char* crossmark() { return use_unicode ? "✗" : "x"; }
    inline const char* bullet() { return use_unicode ? "●" : "*"; }
    inline const char* bullet_empty() { return use_unicode ? "○" : "o"; }
    inline const char* diamond() { return use_unicode ? "◆" : "*"; }
    inline const char* diamond_empty() { return use_unicode ? "◇" : "o"; }
    inline const char* star() { return use_unicode ? "★" : "*"; }
    inline const char* star_empty() { return use_unicode ? "☆" : "."; }
    inline const char* warning() { return use_unicode ? "⚠" : "!"; }
    inline const char* info() { return use_unicode ? "ℹ" : "i"; }
    inline const char* question() { return use_unicode ? "?" : "?"; }
    inline const char* exclamation() { return use_unicode ? "!" : "!"; }
    inline const char* sparkle() { return use_unicode ? "✨" : "*"; }
    inline const char* magic() { return use_unicode ? "✦" : "*"; }
    
    // Loading/Progress animation frames
    inline const char* spinner_0() { return use_unicode ? "◐" : "|"; }
    inline const char* spinner_1() { return use_unicode ? "◓" : "/"; }
    inline const char* spinner_2() { return use_unicode ? "◑" : "-"; }
    inline const char* spinner_3() { return use_unicode ? "◒" : "\\"; }
    
    // Height indicators for flying enemies
    inline const char* height_flying() { return use_unicode ? "^" : "^"; }
    inline const char* height_low_air() { return use_unicode ? "~" : "~"; }
    inline const char* height_ground() { return " "; }
    
    // ==========================================================
    // MESSAGE LOG PREFIXES
    // ==========================================================
    inline const char* msg_combat() { return use_unicode ? "⚔ " : "[!] "; }
    inline const char* msg_damage() { return use_unicode ? "✗ " : "[-] "; }
    inline const char* msg_heal() { return use_unicode ? "♥ " : "[+] "; }
    inline const char* msg_warning() { return use_unicode ? "⚠ " : "[!] "; }
    inline const char* msg_info() { return use_unicode ? "● " : "[i] "; }
    inline const char* msg_loot() { return use_unicode ? "★ " : "[*] "; }
    inline const char* msg_level() { return use_unicode ? "▲ " : "[^] "; }
    inline const char* msg_death() { return use_unicode ? "☠ " : "[X] "; }
    inline const char* msg_debug() { return use_unicode ? "◇ " : "[D] "; }
    inline const char* msg_magic() { return use_unicode ? "✦ " : "[~] "; }
    inline const char* msg_quest() { return use_unicode ? "◈ " : "[Q] "; }
    inline const char* msg_shop() { return use_unicode ? "$ " : "[$] "; }
    
    // ==========================================================
    // ANSI COLOR CODES
    // ==========================================================
    // Returns empty string if color is disabled
    std::string color(const char* code);
    inline std::string reset() { return color("\033[0m"); }
    
    // Foreground colors
    inline std::string fg_black() { return color("\033[30m"); }
    inline std::string fg_red() { return color("\033[31m"); }
    inline std::string fg_green() { return color("\033[32m"); }
    inline std::string fg_yellow() { return color("\033[33m"); }
    inline std::string fg_blue() { return color("\033[34m"); }
    inline std::string fg_magenta() { return color("\033[35m"); }
    inline std::string fg_cyan() { return color("\033[36m"); }
    inline std::string fg_white() { return color("\033[37m"); }
    
    // Bright foreground colors
    inline std::string fg_bright_black() { return color("\033[90m"); }
    inline std::string fg_bright_red() { return color("\033[91m"); }
    inline std::string fg_bright_green() { return color("\033[92m"); }
    inline std::string fg_bright_yellow() { return color("\033[93m"); }
    inline std::string fg_bright_blue() { return color("\033[94m"); }
    inline std::string fg_bright_magenta() { return color("\033[95m"); }
    inline std::string fg_bright_cyan() { return color("\033[96m"); }
    inline std::string fg_bright_white() { return color("\033[97m"); }
    
    // Text styles
    inline std::string bold() { return color("\033[1m"); }
    inline std::string dim() { return color("\033[2m"); }
    inline std::string italic() { return color("\033[3m"); }
    inline std::string underline() { return color("\033[4m"); }
}

