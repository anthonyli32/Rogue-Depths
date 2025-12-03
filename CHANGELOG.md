# Changelog

All notable changes to Rogue Depths will be documented in this file.

## [1.0.0] - 2024-12-XX

### Added
- First public release of Rogue Depths
- Class selection screen with Warrior and Mage options
- New combat layout with arena, HP/status inside arena box, and combat tips
- Mage combat system:
  - Frost Bolt as basic attack (SKILL)
  - Magic Sword and Sword Casting as weapon-unlocked attacks
  - All mage attacks work at any distance
- Healing potions added to starting inventory (5 potions, +20 HP each)
- Depth-based stat scaling (+3 attack, +5 max HP per depth level)
- Low HP reminders on map and in combat
- Congratulations page after completing the game
- Contextual tips box in normal gameplay
- Tips for combat, loot, and interactions

### Changed
- Distance requirements removed - all attacks work at any distance
- Boss difficulty heavily nerfed for better gameplay balance
- Player basic attack damage buffed
- Weapon-based attack damage nerfed (SLASH, POWER_STRIKE, TACKLE, WHIRLWIND)
- Combat actions menu: Class ability (Attack) moved to top
- Goblin archer cooldown: shoots every 4 seconds instead of every move
- Boss messages appear every 5 seconds instead of every move
- Starter weapon: All classes now get "Starter Sword" (mages no longer get wand)
- Inventory consumable display: Changed from `[C]` to `[U]`
- Shrine messages appear in message log with staged timing (1 second between messages)
- Tutorial hidden by default while redesign is in progress

### Fixed
- Player death logic: Player now dies correctly when HP reaches 0 or below
- Healing potion logic: Potions now correctly heal HP without affecting max HP
- Max HP accumulation bug: Fixed issue where max HP would incorrectly increase
- Mage combat actions: Mages now properly unlock Magic Sword and Sword Casting when weapon is equipped
- Combat message box size increased to show 10 lines
- HP and status information moved into Combat Arena box with proper indentation
- Double line box alignment fixed

### Removed
- Old shrine concept that fortified the user
- "Full Tutorial" option from main menu
- "View Leaderboard" option from main menu
- "Load Game" option from main menu
- "1-2 Quick Select" text from class selection (functionality kept)
- "G" button debug spawn test items (disabled)
- Distance-based damage modifiers (all attacks now do full damage at any distance)

