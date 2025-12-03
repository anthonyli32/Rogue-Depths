#include "ai.h"
#include "logger.h"
#include "glyphs.h"
#include "constants.h"
#include "combat.h"

#include <queue>
#include <unordered_map>
#include <map>
#include <utility>
#include <cstdlib>
#include <random> // FIXED: For central RNG
#include <cmath>
#include <algorithm>
#include <chrono>

namespace {
    struct Node {
        int x;
        int y;
    };

    struct KeyHash {
        std::size_t operator()(const std::pair<int, int>& k) const noexcept {
            return (static_cast<std::size_t>(k.first) << 32) ^ static_cast<std::size_t>(k.second);
        }
    };
    
    // Safety limit to prevent infinite loops in pathfinding
    constexpr int MAX_PATHFIND_ITERATIONS = 10000;
}

namespace ai {
    // Central RNG for AI (FIXED: Use same RNG as combat for determinism)
    static std::mt19937& ai_rng() {
        static std::mt19937 rng(std::random_device{}());
        return rng;
    }
    // Basic pathfinding step toward player
    static void step_toward_player(Enemy& e, const Player& player, const Dungeon& dungeon) {
        Position epos = e.get_position();
        Position ppos = player.get_position();
        if (epos.x == ppos.x && epos.y == ppos.y) {
            return;
        }
        
        std::queue<Node> q;
        std::unordered_map<std::pair<int, int>, std::pair<int, int>, KeyHash> parent;
        q.push({epos.x, epos.y});
        parent[{epos.x, epos.y}] = {-9999, -9999};
        const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        bool found = false;
        int iterations = 0;
        
        while (!q.empty() && iterations < MAX_PATHFIND_ITERATIONS) {
            iterations++;
            Node cur = q.front();
            q.pop();
            if (cur.x == ppos.x && cur.y == ppos.y) {
                found = true;
                break;
            }
            for (auto& d : dirs) {
                int nx = cur.x + d[0];
                int ny = cur.y + d[1];
                if (!dungeon.in_bounds(nx, ny) || !dungeon.is_walkable(nx, ny)) {
                    continue;
                }
                std::pair<int,int> key{nx, ny};
                if (parent.find(key) == parent.end()) {
                    parent[key] = {cur.x, cur.y};
                    q.push({nx, ny});
                }
            }
        }
        
        if (iterations >= MAX_PATHFIND_ITERATIONS) {
            LOG_WARN("Pathfinding hit iteration limit! Enemy at (" + 
                     std::to_string(epos.x) + "," + std::to_string(epos.y) + 
                     ") seeking player at (" + std::to_string(ppos.x) + "," + 
                     std::to_string(ppos.y) + ")");
            return;
        }
        
        if (!found) {
            LOG_DEBUG("No path found from enemy to player");
            return;
        }
        
        // Reconstruct one step
        std::pair<int,int> step{ppos.x, ppos.y};
        std::pair<int,int> prev{-1, -1};
        int reconstructIterations = 0;
        while (reconstructIterations < MAX_PATHFIND_ITERATIONS) {
            reconstructIterations++;
            auto it = parent.find(step);
            if (it == parent.end()) {
                break;
            }
            prev = step;
            if (it->second.first == -9999) {
                break;
            }
            step = it->second;
        }
        
        if (reconstructIterations >= MAX_PATHFIND_ITERATIONS) {
            LOG_WARN("Path reconstruction hit iteration limit!");
            return;
        }
        
        int dx = 0;
        int dy = 0;
        if (prev.first != -1) {
            dx = prev.first - epos.x;
            dy = prev.second - epos.y;
        }
        if (dx != 0 || dy != 0) {
            e.move_by(dx, dy);
        }
    }

    // Tier 1 (Basic): Just chase the player
    static void behavior_basic(Enemy& enemy, const Player& player, const Dungeon& dungeon) {
        step_toward_player(enemy, player, dungeon);
    }

    // Tier 2 (Learning): Pattern recognition, slightly smarter
    static void behavior_learning(Enemy& enemy, const Player& player, const Dungeon& dungeon) {
        // If player often kites, take two steps
        if (enemy.knowledge().timesPlayerKited >= 3) {
            step_toward_player(enemy, player, dungeon);
            step_toward_player(enemy, player, dungeon);
        } else {
            step_toward_player(enemy, player, dungeon);
        }
    }

    // Tier 3 (Adapted): Counter-tactics based on player's dominant strategy
    static void behavior_adapted(Enemy& enemy, const Player& player, const Dungeon& dungeon) {
        int dominant = enemy.knowledge().get_dominant_tactic();
        
        switch (dominant) {
            case 1: // Player uses melee often - be more aggressive
                step_toward_player(enemy, player, dungeon);
                step_toward_player(enemy, player, dungeon);
                break;
            case 3: // Player flees often - pursue aggressively
            case 4: // Player kites - double speed
                step_toward_player(enemy, player, dungeon);
                step_toward_player(enemy, player, dungeon);
                break;
            default:
                step_toward_player(enemy, player, dungeon);
                break;
        }
    }

    // Tier 4 (Master): Advanced tactics - flanking, prediction
    static void behavior_master(Enemy& enemy, const Player& player, const Dungeon& dungeon) {
        Position epos = enemy.get_position();
        Position ppos = player.get_position();
        
        // Calculate distance
        int dist = std::abs(epos.x - ppos.x) + std::abs(epos.y - ppos.y);
        
        // Master enemies can predict player movement and cut them off
        // For now, just be very aggressive with triple speed
        step_toward_player(enemy, player, dungeon);
        if (dist > 2) {
            step_toward_player(enemy, player, dungeon);
        }
        if (dist > 4) {
            step_toward_player(enemy, player, dungeon);
        }
        
        // Flying enemies at master tier occasionally descend to attack
        // FIXED: Use central RNG for random decisions
        if (!enemy.is_grounded() && dist <= 2) {
            std::uniform_int_distribution<int> dist3(0, 2);
            if (dist3(ai_rng()) == 0) {
                enemy.descend();
            }
        }
    }

    // Line of sight using Bresenham's algorithm
    bool has_line_of_sight(int x1, int y1, int x2, int y2, const Dungeon& dungeon) {
        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;
        
        int x = x1;
        int y = y1;
        
        while (x != x2 || y != y2) {
            // Check if current tile blocks line of sight
            if (x != x1 || y != y1) {  // Don't check start position
                if (!dungeon.in_bounds(x, y) || !dungeon.is_walkable(x, y)) {
                    return false;  // Blocked
                }
            }
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
        return true;
    }
    
    // Manhattan distance
    int manhattan_distance(int x1, int y1, int x2, int y2) {
        return std::abs(x2 - x1) + std::abs(y2 - y1);
    }
    
    // Move enemy away from player
    void move_away_from(Enemy& enemy, const Player& player, const Dungeon& dungeon) {
        Position epos = enemy.get_position();
        Position ppos = player.get_position();
        
        // Calculate direction away from player
        int dx = 0, dy = 0;
        if (epos.x < ppos.x) dx = -1;
        else if (epos.x > ppos.x) dx = 1;
        if (epos.y < ppos.y) dy = -1;
        else if (epos.y > ppos.y) dy = 1;
        
        // Try to move away
        int nx = epos.x + dx;
        int ny = epos.y + dy;
        if (dungeon.in_bounds(nx, ny) && dungeon.is_walkable(nx, ny)) {
            enemy.move_by(dx, dy);
            return;
        }
        
        // Try horizontal only
        if (dx != 0) {
            nx = epos.x + dx;
            ny = epos.y;
            if (dungeon.in_bounds(nx, ny) && dungeon.is_walkable(nx, ny)) {
                enemy.move_by(dx, 0);
                return;
            }
        }
        
        // Try vertical only
        if (dy != 0) {
            nx = epos.x;
            ny = epos.y + dy;
            if (dungeon.in_bounds(nx, ny) && dungeon.is_walkable(nx, ny)) {
                enemy.move_by(0, dy);
                return;
            }
        }
    }
    
    // Ranged attack from enemy to player
    void ranged_attack(Enemy& enemy, Player& player, int baseDamage, int depth, MessageLog& log) {
        // Calculate damage with depth scaling
        int damage = baseDamage + depth / 2;
        int finalDamage = std::max(0, damage - player.get_stats().defense);
        
        if (finalDamage > 0) {
            player.get_stats().hp -= finalDamage;
            log.add(MessageType::Damage, enemy.name() + " shoots an arrow for " + 
                    std::to_string(finalDamage) + " damage!");
            ui::flash_damage();
            ui::play_hit_sound();
        } else {
            log.add(MessageType::Combat, enemy.name() + "'s arrow bounces off your armor!");
        }
    }
    
    // Archer behavior: keep distance, shoot if in range (with 4 second cooldown)
    static void behavior_archer(Enemy& enemy, Player& player, const Dungeon& dungeon, MessageLog& log, int depth = 1) {
        static std::map<std::pair<int, int>, std::chrono::steady_clock::time_point> lastArcherShotTime;  // Track last shot time per archer position
        
        Position epos = enemy.get_position();
        Position ppos = player.get_position();
        
        // Use position as key to track individual archers
        auto archerKey = std::make_pair(epos.x, epos.y);

        Position3D enemyPos{epos.x, epos.y, enemy.is_grounded() ? 0 : 2};
        Position3D playerPos{ppos.x, ppos.y, 0};
        CombatDistance distCategory = combat::calculate_combat_distance(enemyPos, playerPos);
        
        if (distCategory <= CombatDistance::CLOSE) {
            move_away_from(enemy, player, dungeon);
            LOG_DEBUG("Archer retreating due to close distance");
            return;
        }
        
        // Check if 4 seconds have passed since last shot
        auto now = std::chrono::steady_clock::now();
        bool canShoot = true;
        if (lastArcherShotTime.find(archerKey) != lastArcherShotTime.end()) {
            auto timeSinceLastShot = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastArcherShotTime[archerKey]);
            canShoot = (timeSinceLastShot.count() >= 4000);  // 4 seconds
        }
        
        if ((distCategory == CombatDistance::MEDIUM || distCategory == CombatDistance::FAR) &&
            has_line_of_sight(epos.x, epos.y, ppos.x, ppos.y, dungeon) && canShoot) {
            ranged_attack(enemy, player, 4, depth, log);
            lastArcherShotTime[archerKey] = now;  // Update last shot time
            LOG_DEBUG("Archer firing from distance category " + std::to_string(static_cast<int>(distCategory)));
            return;
        }
        
        step_toward_player(enemy, player, dungeon);
    }

    void take_turn(Enemy& enemy, const Player& player, const Dungeon& dungeon, MessageLog& log) {
        // Update tier before acting
        enemy.knowledge().update_tier();
        
        enemy.tick_statuses(log);
        if (enemy.stats().hp <= 0) {
            return;
        }
        if (enemy.has_status(StatusType::Freeze) || enemy.has_status(StatusType::Stun)) {
            log.add(MessageType::Warning, enemy.name() + " is unable to act!");
            return;
        }
        
        // Special handling for boss enemies
        if (is_boss_type(enemy.enemy_type())) {
            behavior_boss(enemy, player, dungeon, log);
            return;
        }
        
        // Special handling for archer enemies
        if (enemy.enemy_type() == EnemyType::Archer) {
            behavior_archer(enemy, const_cast<Player&>(player), dungeon, log);
            return;
        }
        
        // Choose behavior based on AI tier for melee enemies
        switch (enemy.knowledge().tier) {
            case AITier::Master:
                behavior_master(enemy, player, dungeon);
                break;
            case AITier::Adapted:
                behavior_adapted(enemy, player, dungeon);
                break;
            case AITier::Learning:
                behavior_learning(enemy, player, dungeon);
                break;
            case AITier::Basic:
            default:
                behavior_basic(enemy, player, dungeon);
                break;
        }
    }
    
    // Check if enemy type is a boss
    bool is_boss_type(EnemyType type) {
        return type == EnemyType::StoneGolem ||
               type == EnemyType::ShadowLord ||
               type == EnemyType::Dragon;
    }
    
    // Boss-specific behavior with attack patterns
    void behavior_boss(Enemy& enemy, const Player& player, const Dungeon& dungeon, MessageLog& log) {
        static std::map<EnemyType, int> bossActionCounter;  // Track action count per boss
        static std::map<EnemyType, std::chrono::steady_clock::time_point> lastBossMessageTime;  // Track last message time per boss
        
        int& counter = bossActionCounter[enemy.enemy_type()];
        counter++;
        
        Position epos = enemy.get_position();
        Position ppos = player.get_position();
        int dist = std::abs(epos.x - ppos.x) + std::abs(epos.y - ppos.y);
        
        // Check if 5 seconds have passed since last boss message
        auto now = std::chrono::steady_clock::now();
        bool canShowMessage = true;
        if (lastBossMessageTime.find(enemy.enemy_type()) != lastBossMessageTime.end()) {
            auto timeSinceLastMessage = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastBossMessageTime[enemy.enemy_type()]);
            canShowMessage = (timeSinceLastMessage.count() >= 5000);  // 5 seconds
        }
        
        // Boss-specific patterns based on type
        switch (enemy.enemy_type()) {
            case EnemyType::StoneGolem: {
                // Pattern: BRACE → TACKLE → BRACE (defensive pattern)
                int patternStep = counter % 3;
                if (patternStep == 0) {
                    // BRACE phase - move toward player but prepare defense
                    if (canShowMessage) {
                    log.add(MessageType::Warning, glyphs::shield() + std::string(" ") + enemy.name() + " braces for impact!");
                        lastBossMessageTime[enemy.enemy_type()] = now;
                    }
                    step_toward_player(enemy, player, dungeon);
                } else if (patternStep == 1 && dist <= 2) {
                    // TACKLE phase - aggressive charge
                    if (canShowMessage) {
                    log.add(MessageType::Combat, enemy.name() + " charges at you!");
                        lastBossMessageTime[enemy.enemy_type()] = now;
                    }
                    step_toward_player(enemy, player, dungeon);
                    step_toward_player(enemy, player, dungeon);
                } else {
                    // Continue moving
                    step_toward_player(enemy, player, dungeon);
                }
                break;
            }
            case EnemyType::ShadowLord: {
                // Pattern: FROST_BOLT → TELEPORT → FIREBALL (crowd control)
                int patternStep = counter % 3;
                if (patternStep == 0 && dist > 2) {
                    if (canShowMessage) {
                    log.add(MessageType::Warning, glyphs::ice() + std::string(" ") + enemy.name() + " prepares a frost attack!");
                        lastBossMessageTime[enemy.enemy_type()] = now;
                    }
                    // Simulate ranged attack - move away to cast
                    move_away_from(enemy, player, dungeon);
                } else if (patternStep == 1) {
                    if (canShowMessage) {
                    log.add(MessageType::Combat, enemy.name() + " teleports!");
                        lastBossMessageTime[enemy.enemy_type()] = now;
                    }
                    // Teleport to random nearby position
                    std::random_device rd;
                    std::mt19937 rng(rd());
                    int newX = ppos.x + (rng() % 5) - 2;
                    int newY = ppos.y + (rng() % 5) - 2;
                    if (dungeon.is_walkable(newX, newY)) {
                        enemy.set_position(newX, newY);
                    }
                } else {
                    if (canShowMessage) {
                    log.add(MessageType::Warning, glyphs::fire() + std::string(" ") + enemy.name() + " channels fire magic!");
                        lastBossMessageTime[enemy.enemy_type()] = now;
                    }
                    step_toward_player(enemy, player, dungeon);
                }
                break;
            }
            case EnemyType::Dragon: {
                // Pattern: FIREBALL → RETREAT → FIREBALL (keep distance)
                int patternStep = counter % 3;
                if (patternStep == 0 || patternStep == 2) {
                    if (dist > 3) {
                        if (canShowMessage) {
                        log.add(MessageType::Warning, glyphs::fire() + std::string(" ") + enemy.name() + " breathes fire!");
                            lastBossMessageTime[enemy.enemy_type()] = now;
                        }
                        // Stay at range
                    } else {
                        // Too close, retreat
                        if (canShowMessage) {
                        log.add(MessageType::Combat, enemy.name() + " retreats to optimal range!");
                            lastBossMessageTime[enemy.enemy_type()] = now;
                        }
                        move_away_from(enemy, player, dungeon);
                    }
                } else {
                    // RETREAT phase
                    if (dist <= 4) {
                        move_away_from(enemy, player, dungeon);
                    } else {
                        step_toward_player(enemy, player, dungeon);
                    }
                }
                break;
            }
            default:
                // Fallback to master behavior
                behavior_master(enemy, player, dungeon);
                break;
        }
    }
}


