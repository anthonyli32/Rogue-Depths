# Rogue Depths – Terminal Roguelike (v1.0)

A tactical ASCII dungeon crawler with Pokemon-style combat, dual wielding, and adaptive enemies.

## Navigation

- [Quick Start Guide](#quick-start-guide)
- [How to Play](#how-to-play)
- [Controls](#controls)
- [Features](#features)
- [Screenshots / ASCII Preview](#screenshots--ascii-preview)
- [Build and Run](#build-and-run)
- [Advanced Options (CLI Flags)](#advanced-options-cli-flags)
- [Project Structure](#project-structure)
- [Changelog](#changelog)

## Quick Start Guide

1. **Build the game** (see [Build and Run](#build-and-run) section below)
2. **Run the executable**:
   - Linux/WSL: `./build/bin/rogue_depths`
   - Windows: `build\bin\rogue_depths.exe` (if built with Makefile.windows)
3. **Recommended terminal setup**:
   - Terminal must be at least 260 characters wide
   - Use full screen (F11) for best experience
   - Use a monospaced font; adjust font size until the title frame fits on one screen
4. **Start playing**:
   - Select **New Game**
   - Choose **Warrior** if you're new; **Mage** if you like defensive play

## How to Play

### Objective

Descend through the **Rogue Depths**, clear enemies, collect loot, and defeat the final boss floor.

### Core Loop

1. Explore the dungeon using the **MAP** view.
2. Pick up items and manage them in **INVENTORY** and **EQUIPMENT** views.
3. Walk into an enemy to enter **COMBAT**.
4. Use tactical actions and consumables to survive.
5. Find `>` stairs and descend to deeper, harder floors.

### Map Symbols

- `@` – You (the player)
- `r`, `s`, `g`, ... – Enemies (rats, spiders, goblins, etc.)
- `)` – Weapons
- `[` – Armor
- `!` – Potions / consumables
- `_` – Shrine (heal or buff)
- `^` – Trap
- `>` – Stairs down

### Classes

- **Warrior** (+3 HP, +1 ATK) - Tank/melee specialist with Shield Bash
- **Mage** (-1 HP, +2 DEF) - Defensive magic user with Frost Bolt basic attack. Equip weapons to unlock Magic Sword and Sword Casting attacks.

### Combat System

- **Cooldown-based attacks** - No energy/mana system. Basic attacks have 0 cooldown, special attacks have 1-3 turn cooldowns.
- **Dual wielding** - Equip weapons in main hand and offhand slots to unlock different attacks.
- **Pokemon-style viewport** - ASCII art sprites for player and enemies during combat.
- **Tactical actions** - Choose from Attack, Melee, Magic (mages with weapons), Defense, and Utility categories.

### Dungeon Structure

- **6 floors total** - Reach floor 6 to win!
- **Boss floors**: Floor 2 (Stone Golem), Floor 4 (Shadow Lord), Floor 6 (Dragon - Final Boss)
- **Scaling difficulty** - Maps and enemies grow stronger with each floor
- **Procedural generation** - Each run is unique

## Controls

### Map Navigation
- **Movement**: `WASD` or Arrow Keys (4-directional)
- **TAB**: Cycle through UI views (Map/Inventory/Stats/Equipment/Messages)
- **ESC**: Return to map view from any menu
- **Descend stairs**: `>` (when standing on `>`)

### Inventory & Equipment
- **Inventory**: `i` (toggle), `w/s` or arrows to navigate
- **Equip item**: `e` (weapons auto-assign to main hand or offhand)
- **Use consumable**: `u`
- **Drop item**: `d`

### Combat
- **Combat actions**: Number keys (`1`, `2`, `3`, etc.) for available attacks
- **Basic Attack**: Always available (class-specific)
- **Weapon Attacks**: Unlocked by equipped weapons (main hand and offhand)
- **Defend**: `D` key
- **Consumables**: Available in combat menu with effect previews

### Other
- **Help**: `?` (4-page help system)
- **Quit**: `q`

## Features

- **Camera-centered ASCII map** with dynamic viewport sizing
- **Pokemon-style combat viewport** with ASCII art sprites
- **Dual wielding**: main hand + offhand weapons, unlock different attacks
- **Cooldown-based combat** instead of mana/energy
- **Procedural multi-floor dungeon** with scaling difficulty and bosses
- **Corpse run**: defeat your past self to reclaim lost loot
- **SQLite saves & stats**; ASCII-safe mode (`--no-unicode --no-color`)
- **Tab-based UI switching** - 5 views: Map, Inventory, Stats, Equipment, Message Log
- **Adaptive AI** - Enemies learn from your tactics and adapt
- **Multiple loot drops** - Enemies drop 1-3 items per kill
- **Status effects** - Bleed, Poison, Fortify, Haste, Freeze, Burn, Stun

## Screenshots / ASCII Preview

```
╔══════════════════════════════════════════════════════════════════════════╗
║                                                                          ║
║                    ╔═══╗                                                 ║
║                    ║ @ ║  ROGUE DEPTHS                                   ║
║                    ╚═══╝                                                 ║
║                                                                          ║
║              A Terminal Roguelike Adventure                               ║
║                                                                          ║
╚══════════════════════════════════════════════════════════════════════════╝
```

Combat View:
```
╔ ⚔  COMBAT ACTIONS ═══════════════════════════════════════════════════════┌─ ⚔  COMBAT ARENA ───────────────────────────────────────────┐
║                                                                         ║│                                                                          │
║Attack                                                                   ║│ Player HP: [██████████████████░░] 13/14                                  │
║  [1] ⭐ Frost Bolt (Freezing bolt attack)                              ║│ Goblin Archer HP: [████████████████████] 6/6                               │
║                                                                         ║│ Player Status: (none)                                                      │
║Magic                                                                    ║│ Goblin Archer Status: (none)                                                 │
║  [2] ⚔ Magic Sword (Long-range flying sword)                          ║│                                                                          │
║  [3] ⚔ Sword Casting (Magical sword projectile)                       ║│ Damage Modifier: 100%                                                        │
║                                                                         ║│ Hit Chance: 90%                                                              │
║Defense                                                                  ║│                                                                          │
║  [4] 🛡 Defend (Hunker down)                                           ║└──────────────────────────────────────────────────────────────────────────┘
║                                                                         ║
║Utility                                                                  ║
║  [5] ⏸ Wait (Pass turn)                                               ║
║                                                                         ║
╚══════════════════════════════════════════════════════════════════════════╝
```

## Build and Run

### Prerequisites
- g++ (C++17), gcc (for SQLite), make
- Windows with WSL or Linux/Unix terminal

### Build
```bash
make
```

### Run
```bash
make run
# or
./build/bin/rogue_depths
```

### Clean
```bash
make clean
```

**Compiler flags**: `-std=c++17 -O2 -Wall -Wextra -Wpedantic`

## Advanced Options (CLI Flags)

```bash
./rogue_depths --help              # Show all options
./rogue_depths --version           # Show version info
./rogue_depths --seed 12345        # Set random seed
./rogue_depths --difficulty hard   # Set difficulty (easy/normal/hard)
./rogue_depths --debug             # Enable debug mode
./rogue_depths --log-file game.log # Write debug log to file
./rogue_depths --no-color          # Disable ANSI colors
./rogue_depths --no-unicode        # Use ASCII-only characters
```

## Project Structure

```
src/
├── cli.cpp/h          # Command-line argument parsing
├── logger.cpp/h       # Debug logging system
├── glyphs.cpp/h       # Unicode/ASCII glyph system
├── keybinds.cpp/h     # Configurable key bindings
├── floor_manager.cpp/h # On-demand floor generation
├── database.cpp/h     # SQLite persistence layer
├── ui.cpp/h           # UI rendering and views
├── input.cpp/h        # Raw input handling
├── dungeon.cpp/h      # Procedural generation
├── player.cpp/h       # Player class and stats
├── enemy.cpp/h        # Enemy types and AI
├── ai.cpp/h           # Adaptive AI system
├── combat.cpp/h       # Combat calculations and tactical combat
├── fileio.cpp/h       # Legacy binary save system
├── loot.cpp/h         # Item generation and loot drops
├── types.h            # Core enums and structs
└── constants.h        # Game constants and colors

lib/
├── sqlite3.c          # SQLite amalgamation
└── sqlite3.h

config/
└── controls.json      # Key binding configuration

assets/ascii/
├── combat/            # ASCII art sprites for combat
│   ├── warrior.txt
│   ├── mage.txt
│   ├── dragon.txt
│   └── ... (enemy sprites)
├── title.txt          # Title screen
├── gameover.txt       # Game over screen
└── victory.txt        # Victory screen

saves/                 # Database and legacy save files
```

## Changelog

### v1.0.0
- First public release of Rogue Depths
- New class selection screen (Warrior / Mage)
- New combat layout with arena, HP/status inside arena box, and combat tips
- Mage combat system: Frost Bolt basic attack, Magic Sword and Sword Casting weapon attacks
- Distance requirements removed - all attacks work at any distance
- Tutorial hidden by default while redesign is in progress
- Healing potions added to starting inventory
- Boss difficulty nerfed for better gameplay balance
- Depth-based stat scaling (attack +3, HP +5 per depth level)

---

## Technical Details

### Professional Terminal Architecture

Inspired by [Terminal Block Mining Simulation Game](https://github.com/RobertElderSoftware/robert-elder-software-java-modules):

- **Command-line arguments** - Full CLI interface with `--help`, `--seed`, `--difficulty`, `--debug`, `--log-file`, `--no-color`, `--no-unicode`
- **Debug logging system** - File-based logging with DEBUG/INFO/WARN/ERROR levels
- **Configurable keybindings** - JSON config file (`config/controls.json`) for custom key mappings
- **Tab-based UI switching** - 5 views: Map, Inventory, Stats, Equipment, Message Log
- **On-demand floor generation** - Floors generated only when visited, cached for backtracking
- **SQLite database persistence** - Professional database storage for saves, corpses, config, stats
- **ASCII fallback mode** - `--no-unicode` for maximum terminal compatibility

### Visual Polish
- **Animated title screen** with color cycling and main menu
- **Camera-centered viewport** (Dwarf Fortress style) - player always at center
- **Dynamic viewport sizing** - Adapts to terminal size (40x15 min, 100x35 max)
- **Distance-based shading** - Tiles fade with distance for depth perception
- **Box-drawing UI frames** using Unicode characters (╔═╗║╚╝) or ASCII fallback (+=-|)
- **Color-coded everything** - monsters by tier, items by rarity, HP by health %
- **Pokemon-style combat viewport** - ASCII art sprites for player and enemies
- **Floating damage numbers** - Animated damage indicators with color coding
- **Enhanced message log** - 8-line message box with color-coded message types

### Tab-Based UI Views

Press `TAB` to cycle through:
1. **MAP** - Main gameplay view (default)
2. **INVENTORY** - Full item list with details, type, rarity, stats
3. **STATS** - Character stats, HP bar, floor progress, kill count, status effects
4. **EQUIPMENT** - Equipped items with slot visualization
5. **MESSAGES** - Scrollable message history (8 lines)

### Combat System Details

#### Cooldown-Based Attacks
- **No energy system** - Attacks use cooldowns based on damage
- **Basic attacks** (1.0x damage) = 0 cooldown
- **Special attacks** (1.2x-1.5x damage) = 1-2 turn cooldown
- **Heavy attacks** (2.0x+ damage) = 3 turn cooldown

#### Dual Wielding
- **Main hand + Offhand** - Equip up to 2 weapons simultaneously
- Weapons auto-assign: First weapon goes to main hand, second to offhand
- Both weapons contribute to attack unlocks
- Equip via inventory menu (`i` key)

#### Weapon-Based Attack Unlocks
All weapon types unlock attacks regardless of class (mages can telekinetically use swords!):

- **Melee Weapons** (sword, axe, hammer, dagger, etc.):
  - All: Unlock `POWER_STRIKE` (1.5x damage, 2-turn cooldown)
  - Rare+: Unlock `TACKLE` (0.8x damage + stun, 1-turn cooldown)

- **Ranged Weapons** (bow, crossbow):
  - All: Unlock `SHOOT` (1.0x damage, 0 cooldown)
  - Rare+: Unlock `SNIPE` (1.5x damage, 2-turn cooldown)

- **Magic Weapons** (staff, wand, sword for mages):
  - Mages with any weapon: Unlock `FIREBALL` (Magic Sword) and `FROST_BOLT` (Sword Casting)
  - All attacks work at any distance

#### Class Abilities (Basic Attacks)
- **Warrior**: Shield Bash (melee attack + stun, no cooldown)
- **Mage**: Frost Bolt (freezing bolt, no cooldown, applies freeze status)

### Height-Based Combat
- **Ground enemies** - Normal melee combat
- **Hovering enemies** (~) - Harder to hit with melee
- **Flying enemies** (^) - Require ranged/magic weapons
- Dragons fly, Liches hover, most enemies are grounded

### Environmental Hazards
- `~` Water - Slows movement
- `~` Lava (orange) - Deadly damage
- ` ` Chasm - Instant death
- `^` Traps - Deal damage when stepped on
- `_` Shrines - Heal or grant buffs

### Enhanced Adaptive AI
4-tier learning system:
1. **Basic** - Simple pathfinding
2. **Learning** - Pattern recognition (uppercase glyph)
3. **Adapted** - Counter-tactics based on player behavior
4. **Master** - Advanced tactics, bright yellow glyph

Enemies track last 10 player actions and adapt their behavior!

### Monsters (Standard Roguelike Glyphs)
- Weak: `r` rat, `s` spider, `g` goblin, `k` kobold, `o` orc, `z` zombie
- Strong: `G` gnome, `O` ogre, `T` troll, `D` dragon, `L` lich
- Bosses: `G` Stone Golem (floor 2), `L` Shadow Lord (floor 4), `D` Dragon (floor 6)
- Special: `C` vengeful spirit (corpse run)
- Enemies scale with depth and adapt to player tactics
- **Reduced spawns**: 1 + depth/3 enemies per floor (faster gameplay)

### Items & Equipment
- Weapons `)`, Armor `[`, Potions `!` with rarity colors
- Equipment slots: Head, Chest, Weapon (main hand), Offhand, Accessory
- **Multiple loot drops**: Enemies drop 1-3 items (70% chance for 1, 20% for 2, 10% for 3)
- **Loot distribution**: 40% weapons, 30% armor, 30% consumables
- Status effects: Bleed, Poison, Fortify, Haste, Freeze, Burn, Stun
- Ranged weapons can hit flying enemies

### Corpse Run 2.0
- **Multiple corpses** - Last 3 deaths stored
- **Decay system** - Corpses age over runs
- **Death cause tracking** - Different glyphs for enemy/trap/environment deaths
- Defeat your past selves to recover loot!

### Persistence (SQLite)
- **SQLite database** for robust persistence
- Tables: players, floors, corpses, config, stats
- Auto-save on exit
- Backward-compatible binary save fallback
- Corpse run: Previous deaths spawn vengeful spirits
- Difficulty modes: Explorer, Adventurer, Nightmare

### Help System
Press `?` anytime to access 4-page help:
1. Controls - Movement, inventory, combat keys
2. Symbols - Terrain, enemy, item glyphs
3. Classes - Warrior/Mage details
4. Tips - Gameplay hints

## Testing & QA Checklist

| Test | Command | Expected Result |
|------|---------|-----------------|
| Build smoke test | `make clean && make` | All objects link without warnings beyond the known unused helpers |
| CLI sanity | `./build/bin/rogue_depths --help` | Usage text lists every flag, exits 0 |
| Seed determinism | `./build/bin/rogue_depths --seed 12345 --debug` | Same floor layout each run, seed displayed on HUD/title |
| Difficulty scaling | Start new run, set `--difficulty nightmare` | Player HP reduced, enemies hit harder, log shows "Nightmare difficulty" |
| TAB views | Press `TAB` repeatedly in-game | Cycles MAP → INVENTORY → STATS → EQUIPMENT → MESSAGE LOG without rendering glitches |
| Class selection | Start new run | Selector shows Warrior and Mage, log includes "You embark as the …" message |
| Dual wielding | Equip two weapons via inventory | First weapon in main hand, second in offhand, both unlock attacks |
| Weapon attacks | Equip different weapon types | Appropriate attacks unlock (melee/ranged/magic) |
| Multiple loot | Kill enemies | 1-3 items drop per enemy with combined loot message |
| Corpse run | Die, restart, defeat Vengeful Spirit | Inventory from previous run restored, slot 2 cleared |
| Reset hotkeys | Press `R` on title | Reset confirmation appears, saves wiped on confirm |
| ASCII fallback | `./build/bin/rogue_depths --no-unicode --no-color` | UI uses ASCII borders and monochrome palette |

### Debugging & Logs

- Run with verbose logging: `./build/bin/rogue_depths --debug --log-file combat.log`
- Follow the latest log entries: `tail -f combat.log`
- Filter for damage events: `grep -i "damage" combat.log`

### Memory Diagnostics

AddressSanitizer (recommended when Valgrind is unavailable):
```
make clean && CXXFLAGS="-std=c++17 -g -O1 -fsanitize=address" make
ASAN_OPTIONS=detect_leaks=1 ./build/bin/rogue_depths --help
```

Valgrind (once installed on WSL):
```
valgrind --leak-check=full --show-reachable=yes ./build/bin/rogue_depths --help
```
Log is stored in `build/valgrind.log` when invoked via `valgrind … > build/valgrind.log 2>&1`.

## Configuration Files

### config/controls.json
Customize key bindings:
```json
{
  "MOVE_UP": ["w", "W"],
  "MOVE_DOWN": ["s", "S"],
  "INVENTORY": ["i", "I"],
  "HELP": ["?"],
  "QUIT": ["q", "Q"]
}
```

## Dependencies

- Standard C++17 compiler (g++)
- SQLite3 (included as amalgamation - no external dependency)
- POSIX terminal (for raw input mode)

## Notes

- Designed for terminals ≥ 70×40. Uses ANSI escape sequences and Unicode box-drawing.
- Use `--no-unicode --no-color` for maximum compatibility with basic terminals.
- Inspired by [Terminal Block Mining Simulation Game](https://github.com/RobertElderSoftware/robert-elder-software-java-modules)

## Known Limitations

- FOV is radial distance only (no occlusion)
- Raw input is POSIX-only; non-POSIX builds use blocking fallback
- Flying enemies use simplified combat (melee miss message, still take damage from ranged)
- Dual wielding auto-assigns weapons (no manual slot selection in inventory)
