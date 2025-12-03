#include "viewport.h"
#include "constants.h"
#include "glyphs.h"
#include "ui.h"
#include <cmath>
#include <iostream>

// Simple FOV check (circular radius)
bool in_simple_fov(const Position& center, int x, int y, int radius) {
    int dx = x - center.x;
    int dy = y - center.y;
    return dx * dx + dy * dy <= radius * radius;
}

// Calculate distance for shading (Euclidean)
int calculate_distance(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    return static_cast<int>(std::sqrt(dx * dx + dy * dy));
}

// Get shaded wall color based on distance
const std::string& get_wall_shade(int dist) {
    if (dist <= 2) return constants::shade_wall_close;
    if (dist <= 4) return constants::shade_wall_medium;
    if (dist <= 6) return constants::shade_wall_far;
    return constants::shade_fog;
}

// Get shaded floor color based on distance
const std::string& get_floor_shade(int dist) {
    if (dist <= 2) return constants::shade_floor_close;
    if (dist <= 4) return constants::shade_floor_medium;
    if (dist <= 6) return constants::shade_floor_far;
    return constants::shade_fog;
}

// Get general shade multiplier for entities
const std::string& get_entity_shade(int dist) {
    if (dist <= 2) return constants::shade_close;
    if (dist <= 4) return constants::shade_medium;
    if (dist <= 6) return constants::shade_far;
    return constants::shade_fog;
}

// Camera-centered viewport rendering (Dwarf Fortress style)
// Player is always at center of viewport; map scrolls around them
// Now with distance-based shading for depth perception
void draw_map_viewport(const Dungeon& dungeon, const Player& player, 
                       const std::vector<Enemy>& enemies, 
                       int startRow, int startCol, int vw, int vh) {
    
    // Calculate camera position (centered on player)
    Position pp = player.get_position();
    int cam_x = pp.x - vw / 2;
    int cam_y = pp.y - vh / 2;
    
    // Clamp camera to map bounds
    cam_x = std::max(0, std::min(cam_x, dungeon.width() - vw));
    cam_y = std::max(0, std::min(cam_y, dungeon.height() - vh));
    
    // Draw the main game frame
    ui::draw_box_double(startRow, startCol, vw + 2, vh + 2, constants::color_frame_main);
    
    // Draw title in frame
    ui::move_cursor(startRow, startCol + 2);
    ui::set_color(constants::color_frame_main);
    std::cout << " ROGUE DEPTHS ";
    ui::reset_color();
    
    // Draw viewport contents
    for (int vy = 0; vy < vh; ++vy) {
        ui::move_cursor(startRow + 1 + vy, startCol + 1);
        for (int vx = 0; vx < vw; ++vx) {
            int x = cam_x + vx;
            int y = cam_y + vy;
            
            // Out of map bounds
            if (!dungeon.in_bounds(x, y)) {
                std::cout << ' ';
                continue;
            }
            
            // FOV check
            if (!in_simple_fov(pp, x, y, constants::fov_radius)) {
                std::cout << ' ';
                continue;
            }
            
            // Calculate distance for shading
            int dist = calculate_distance(pp.x, pp.y, x, y);
            
            TileType t = dungeon.get_tile(x, y);
            
            // Player (always at center when visible)
            if (pp.x == x && pp.y == y) {
                ui::set_color(constants::color_player);
                std::cout << glyphs::player();
                ui::reset_color();
                continue;
            }
            
            // Enemies (with distance shading)
            bool drawnEnemy = false;
            for (const auto& e : enemies) {
                if (e.get_position().x == x && e.get_position().y == y) {
                    // Apply distance-based shading to enemy color
                    // Color based on distance and height
                    if (e.height() == HeightLevel::Flying) {
                        // Flying enemies have a distinct bright color
                        ui::set_color("\033[38;5;51m"); // Bright cyan for flying
                    } else if (e.height() == HeightLevel::LowAir) {
                        // Hovering enemies are slightly faded
                        ui::set_color("\033[38;5;147m"); // Light purple for hovering
                    } else if (dist <= 3) {
                        ui::set_color(e.color()); // Full color when close
                    } else {
                        ui::set_color(get_entity_shade(dist)); // Faded when far
                    }
                    
                    char eglyph = e.glyph();
                    
                    // Show AI tier indicator or height indicator
                    AITier tier = e.knowledge().tier;
                    if (e.height() == HeightLevel::Flying) {
                        std::cout << glyphs::height_flying(); // Flying indicator
                    } else if (e.height() == HeightLevel::LowAir) {
                        std::cout << glyphs::height_low_air(); // Hovering indicator
                    } else if (tier == AITier::Master) {
                        // Master tier: uppercase glyph + bright color
                        if (eglyph >= 'a' && eglyph <= 'z') {
                            eglyph = eglyph - 'a' + 'A';
                        }
                        ui::set_color("\033[38;5;226m"); // Bright yellow for master
                        std::cout << eglyph;
                    } else if (tier == AITier::Adapted || tier == AITier::Learning) {
                        // Learning/Adapted: uppercase glyph
                        if (eglyph >= 'a' && eglyph <= 'z') {
                            eglyph = eglyph - 'a' + 'A';
                        }
                        std::cout << eglyph;
                    } else {
                        std::cout << eglyph;
                    }
                    ui::reset_color();
                    drawnEnemy = true;
                    break;
                }
            }
            if (drawnEnemy) continue;
            
            // Terrain with distance-based shading
            switch (t) {
                case TileType::Wall:
                    ui::set_color(get_wall_shade(dist));
                    std::cout << glyphs::wall();
                    ui::reset_color();
                    break;
                case TileType::Floor:
                    ui::set_color(get_floor_shade(dist));
                    std::cout << glyphs::floor_tile();
                    ui::reset_color();
                    break;
                case TileType::Door:
                    ui::set_color(get_entity_shade(dist));
                    std::cout << glyphs::door_closed();
                    ui::reset_color();
                    break;
                case TileType::StairsDown:
                    // Stairs are always bright (important navigation)
                    ui::set_color(constants::color_stairs);
                    std::cout << glyphs::stairs_down();
                    ui::reset_color();
                    break;
                case TileType::StairsUp:
                    ui::set_color(constants::color_stairs);
                    std::cout << glyphs::stairs_up();
                    ui::reset_color();
                    break;
                case TileType::Trap:
                    // Traps fade with distance (harder to see from far)
                    if (dist <= 3) {
                        ui::set_color(constants::color_trap);
                    } else {
                        ui::set_color(get_floor_shade(dist)); // Blend with floor when far
                    }
                    std::cout << glyphs::trap();
                    ui::reset_color();
                    break;
                case TileType::Shrine:
                    // Shrines glow (always visible)
                    ui::set_color(constants::color_shrine);
                    std::cout << glyphs::shrine();
                    ui::reset_color();
                    break;
                case TileType::Water:
                    // Water shimmers
                    ui::set_color(constants::color_water);
                    std::cout << glyphs::water();
                    ui::reset_color();
                    break;
                case TileType::DeepWater:
                    ui::set_color(constants::color_deep_water);
                    std::cout << glyphs::deep_water();
                    ui::reset_color();
                    break;
                case TileType::Lava:
                    // Lava glows bright (always visible, dangerous)
                    ui::set_color(constants::color_lava);
                    std::cout << glyphs::lava();
                    ui::reset_color();
                    break;
                case TileType::Chasm:
                    // Chasm is dark void
                    ui::set_color(constants::color_chasm);
                    std::cout << glyphs::chasm();
                    ui::reset_color();
                    break;
                default:
                    std::cout << ' ';
                    break;
            }
        }
    }
}

