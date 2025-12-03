#pragma once

#include "dungeon.h"
#include "player.h"
#include "enemy.h"
#include "types.h"
#include <vector>

// Simple FOV check (circular radius)
bool in_simple_fov(const Position& center, int x, int y, int radius);

// Calculate distance for shading (Euclidean)
int calculate_distance(int x1, int y1, int x2, int y2);

// Get shaded wall color based on distance
const std::string& get_wall_shade(int dist);

// Get shaded floor color based on distance
const std::string& get_floor_shade(int dist);

// Get general shade multiplier for entities
const std::string& get_entity_shade(int dist);

// Camera-centered viewport rendering (Dwarf Fortress style)
// Player is always at center of viewport; map scrolls around them
// Now with distance-based shading for depth perception
void draw_map_viewport(const Dungeon& dungeon, const Player& player, 
                       const std::vector<Enemy>& enemies, 
                       int startRow, int startCol, int vw, int vh);

