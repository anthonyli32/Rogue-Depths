#include "dungeon.h"
#include "logger.h"

#include <algorithm>
#include <cmath>

Dungeon::Dungeon()
    : Dungeon(80, 40) {
}

Dungeon::Dungeon(int width, int height)
    : width_(width), height_(height), tiles_(static_cast<size_t>(width) * static_cast<size_t>(height), TileType::Wall) {
}

const Room* Dungeon::get_room_at(int x, int y) const {
    for (const auto& room : rooms_) {
        if (x >= room.x && x < room.x + room.w &&
            y >= room.y && y < room.y + room.h) {
            return &room;
        }
    }
    return nullptr;
}

RoomType Dungeon::get_room_type_at(int x, int y) const {
    const Room* room = get_room_at(x, y);
    return room ? room->type : RoomType::GENERIC;
}

int Dungeon::width() const {
    return width_;
}

int Dungeon::height() const {
    return height_;
}

bool Dungeon::in_bounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

TileType Dungeon::get_tile(int x, int y) const {
    if (!in_bounds(x, y)) {
        return TileType::Unknown;
    }
    size_t idx = static_cast<size_t>(y) * static_cast<size_t>(width_) + static_cast<size_t>(x);
    if (idx >= tiles_.size()) {
        LOG_ERROR("Dungeon::get_tile out-of-bounds access: idx=" + std::to_string(idx) + ", tiles_.size()=" + std::to_string(tiles_.size()));
        return TileType::Unknown;
    }
    return tiles_[idx];
}

void Dungeon::set_tile(int x, int y, TileType t) {
    if (!in_bounds(x, y)) {
        return;
    }
    size_t idx = static_cast<size_t>(y) * static_cast<size_t>(width_) + static_cast<size_t>(x);
    if (idx >= tiles_.size()) {
        LOG_ERROR("Dungeon::set_tile out-of-bounds access: idx=" + std::to_string(idx) + ", tiles_.size()=" + std::to_string(tiles_.size()));
        return;
    }
    tiles_[idx] = t;
}

bool Dungeon::is_walkable(int x, int y) const {
    TileType t = get_tile(x, y);
    return t == TileType::Floor || t == TileType::Door || 
           t == TileType::StairsDown || t == TileType::StairsUp ||
           t == TileType::Trap || t == TileType::Shrine ||
           t == TileType::Water; // Water is walkable but slows
    // Lava, Chasm, DeepWater are NOT walkable
}

bool Dungeon::is_hazardous(int x, int y) const {
    TileType t = get_tile(x, y);
    return t == TileType::Lava || t == TileType::Trap || 
           t == TileType::Chasm || t == TileType::DeepWater;
}

bool Dungeon::is_deadly(int x, int y) const {
    TileType t = get_tile(x, y);
    return t == TileType::Lava || t == TileType::Chasm;
}

void Dungeon::carve_room(const Rect& room) {
    for (int y = room.y; y < room.y + room.h; ++y) {
        for (int x = room.x; x < room.x + room.w; ++x) {
            set_tile(x, y, TileType::Floor);
        }
    }
}

void Dungeon::carve_h_corridor(int x1, int x2, int y) {
    int start = std::min(x1, x2);
    int end = std::max(x1, x2);
    for (int x = start; x <= end; ++x) {
        set_tile(x, y, TileType::Floor);
    }
}

void Dungeon::carve_v_corridor(int y1, int y2, int x) {
    int start = std::min(y1, y2);
    int end = std::max(y1, y2);
    for (int y = start; y <= end; ++y) {
        set_tile(x, y, TileType::Floor);
    }
}

void Dungeon::assign_room_types(std::mt19937& rng, int depth) {
    if (rooms_.empty()) return;

    // First room is always SANCTUARY (safe start)
    rooms_.front().type = RoomType::SANCTUARY;

    // Last room has stairs (GENERIC)
    if (rooms_.size() > 1) {
        rooms_.back().type = RoomType::GENERIC;
    }

    // Boss room every 4 floors
    if (depth % 4 == 0 && rooms_.size() > 2) {
        size_t bossIdx = rooms_.size() - 2;
        if (bossIdx < rooms_.size()) {
            rooms_[bossIdx].type = RoomType::BOSS_CHAMBER;
            LOG_INFO("Boss chamber placed on floor " + std::to_string(depth));
        } else {
            LOG_ERROR("assign_room_types: bossIdx out of bounds");
        }
    }

    // Assign types to middle rooms
    std::uniform_int_distribution<int> typeDist(0, 100);
    for (size_t i = 1; i + 1 < rooms_.size(); ++i) {
        if (i >= rooms_.size()) {
            LOG_ERROR("assign_room_types: i out of bounds");
            break;
        }
        if (rooms_[i].type != RoomType::GENERIC) continue;  // Already assigned

        int roll = typeDist(rng);

        // Deeper floors have more special rooms
        int treasureChance = 10 + depth * 2;
        int shrineChance = treasureChance + 8;
        int trapChance = shrineChance + 12 + depth;
        int secretChance = trapChance + 5;

        if (roll < treasureChance) {
            rooms_[i].type = RoomType::TREASURE;
        } else if (roll < shrineChance) {
            rooms_[i].type = RoomType::SHRINE;
        } else if (roll < trapChance) {
            rooms_[i].type = RoomType::TRAP_CHAMBER;
        } else if (roll < secretChance) {
            rooms_[i].type = RoomType::SECRET;
        }
        // Otherwise stays GENERIC
    }
}

void Dungeon::populate_room(Room& room, std::mt19937& rng) {
    std::uniform_int_distribution<int> xDist(room.x + 1, std::max(room.x + 1, room.x + room.w - 2));
    std::uniform_int_distribution<int> yDist(room.y + 1, std::max(room.y + 1, room.y + room.h - 2));
    
    switch (room.type) {
        case RoomType::TREASURE: {
            // Place multiple item spots (rendered as floor, items spawned separately)
            LOG_DEBUG("Populating treasure room at (" + std::to_string(room.x) + "," + std::to_string(room.y) + ")");
            break;
        }
        case RoomType::SHRINE: {
            // Place shrine in center
            int sx = room.center_x();
            int sy = room.center_y();
            if (get_tile(sx, sy) == TileType::Floor) {
                set_tile(sx, sy, TileType::Shrine);
            }
            LOG_DEBUG("Placed shrine in room at (" + std::to_string(sx) + "," + std::to_string(sy) + ")");
            break;
        }
        case RoomType::TRAP_CHAMBER: {
            // Place multiple traps
            int trapCount = 2 + (rng() % 3);  // 2-4 traps
            for (int t = 0; t < trapCount; ++t) {
                int tx = xDist(rng);
                int ty = yDist(rng);
                if (get_tile(tx, ty) == TileType::Floor) {
                    set_tile(tx, ty, TileType::Trap);
                }
            }
            LOG_DEBUG("Placed " + std::to_string(trapCount) + " traps in trap chamber");
            break;
        }
        case RoomType::BOSS_CHAMBER: {
            // Boss rooms are larger, clear of hazards
            // Boss enemy spawning handled in main.cpp
            LOG_DEBUG("Boss chamber prepared");
            break;
        }
        
        case RoomType::SECRET: {
            // Secret rooms have treasure but might have traps
            if (rng() % 2 == 0) {
                int tx = xDist(rng);
                int ty = yDist(rng);
                if (get_tile(tx, ty) == TileType::Floor) {
                    set_tile(tx, ty, TileType::Trap);
                }
            }
            LOG_DEBUG("Secret room populated");
            break;
        }
        
        case RoomType::SANCTUARY: {
            // Safe room - maybe a shrine for healing
            if (rng() % 3 == 0) {
                int sx = room.center_x();
                int sy = room.center_y();
                if (get_tile(sx, sy) == TileType::Floor) {
                    set_tile(sx, sy, TileType::Shrine);
                }
            }
            break;
        }
        
        default:
            break;
    }
}

void Dungeon::generate(unsigned int seed, Position& playerStart, Position& stairsDown, int depth) {
    LOG_INFO("Generating dungeon with seed " + std::to_string(seed) + 
             " size " + std::to_string(width_) + "x" + std::to_string(height_) +
             " depth " + std::to_string(depth));
    
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> roomWDist(5, 12);
    std::uniform_int_distribution<int> roomHDist(4, 8);
    std::uniform_int_distribution<int> roomXDist(1, std::max(2, width_ - 14));
    std::uniform_int_distribution<int> roomYDist(1, std::max(2, height_ - 10));

    std::fill(tiles_.begin(), tiles_.end(), TileType::Wall);
    rooms_.clear();

    std::vector<Rect> tempRooms;
    const int maxRooms = 12;
    for (int i = 0; i < maxRooms; ++i) {
        Rect r;
        r.w = roomWDist(rng);
        r.h = roomHDist(rng);
        r.x = roomXDist(rng);
        r.y = roomYDist(rng);
        if (r.x + r.w + 1 >= width_ || r.y + r.h + 1 >= height_) {
            continue;
        }
        // simple overlap check
        bool overlaps = false;
        for (const auto& existing : tempRooms) {
            if (r.x <= existing.x + existing.w && r.x + r.w >= existing.x &&
                r.y <= existing.y + existing.h && r.y + r.h >= existing.y) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) {
            continue;
        }
        carve_room(r);
        if (!tempRooms.empty()) {
            // connect to previous room
            const Rect& prev = tempRooms.back();
            int prevCenterX = prev.x + prev.w / 2;
            int prevCenterY = prev.y + prev.h / 2;
            int newCenterX = r.x + r.w / 2;
            int newCenterY = r.y + r.h / 2;
            if (rng() % 2) {
                carve_h_corridor(prevCenterX, newCenterX, prevCenterY);
                carve_v_corridor(prevCenterY, newCenterY, newCenterX);
            } else {
                carve_v_corridor(prevCenterY, newCenterY, prevCenterX);
                carve_h_corridor(prevCenterX, newCenterX, newCenterY);
            }
        }
        tempRooms.push_back(r);
        
        // Convert to Room struct
        Room room;
        room.x = r.x;
        room.y = r.y;
        room.w = r.w;
        room.h = r.h;
        room.type = RoomType::GENERIC;
        rooms_.push_back(room);
    }

    // ensure at least one room
    if (tempRooms.empty()) {
        Rect r{2, 2, std::max(5, width_ / 3), std::max(4, height_ / 3)};
        carve_room(r);
        tempRooms.push_back(r);
        
        Room room;
        room.x = r.x;
        room.y = r.y;
        room.w = r.w;
        room.h = r.h;
        room.type = RoomType::GENERIC;
        rooms_.push_back(room);
    }

    LOG_INFO("Generated " + std::to_string(rooms_.size()) + " rooms");
    
    // Assign room types based on depth
    assign_room_types(rng, depth);
    
    // Place player at first room center
    playerStart.x = rooms_.front().center_x();
    playerStart.y = rooms_.front().center_y();
    set_tile(playerStart.x, playerStart.y, TileType::Floor);
    LOG_DEBUG("Player start at (" + std::to_string(playerStart.x) + "," + std::to_string(playerStart.y) + ")");

    // Place stairs down in last room
    stairsDown.x = rooms_.back().center_x();
    stairsDown.y = rooms_.back().center_y();
    set_tile(stairsDown.x, stairsDown.y, TileType::StairsDown);
    LOG_DEBUG("Stairs down at (" + std::to_string(stairsDown.x) + "," + std::to_string(stairsDown.y) + ")");

    // Populate rooms based on their types
    for (auto& room : rooms_) {
        populate_room(room, rng);
    }

    // Place environmental hazards in generic rooms
    std::uniform_int_distribution<int> eventChance(0, 100);
    for (auto& room : rooms_) {
        if (room.type != RoomType::GENERIC) continue;
        
        int roll = eventChance(rng);
        std::uniform_int_distribution<int> xDist(room.x + 1, std::max(room.x + 1, room.x + room.w - 2));
        std::uniform_int_distribution<int> yDist(room.y + 1, std::max(room.y + 1, room.y + room.h - 2));
        
        if (roll < 15) {
            // 15% chance for water pool
            int wx = xDist(rng);
            int wy = yDist(rng);
            // Place small water pool (3x3 or smaller)
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (get_tile(wx + dx, wy + dy) == TileType::Floor && rng() % 2 == 0) {
                        set_tile(wx + dx, wy + dy, TileType::Water);
                    }
                }
            }
        } else if (roll < 25) {
            // 10% chance for lava (dangerous!)
            int lx = xDist(rng);
            int ly = yDist(rng);
            if (get_tile(lx, ly) == TileType::Floor) {
                set_tile(lx, ly, TileType::Lava);
            }
        } else if (roll < 35) {
            // 10% chance for chasm
            int cx = xDist(rng);
            int cy = yDist(rng);
            if (get_tile(cx, cy) == TileType::Floor) {
                set_tile(cx, cy, TileType::Chasm);
            }
        }
    }
}


