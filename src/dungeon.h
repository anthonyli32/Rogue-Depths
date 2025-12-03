#pragma once

#include <vector>
#include <random>
#include "types.h"


/**
 * @brief Structure representing a dungeon room with type and bounds.
 */
struct Room {
    int x = 0; ///< X coordinate of top-left corner
    int y = 0; ///< Y coordinate of top-left corner
    int w = 0; ///< Width of the room
    int h = 0; ///< Height of the room
    RoomType type = RoomType::GENERIC; ///< Room type (treasure, shrine, etc.)

    /**
     * @brief Get the X coordinate of the room center.
     * @return Center X coordinate
     */
    int center_x() const { return x + w / 2; }
    /**
     * @brief Get the Y coordinate of the room center.
     * @return Center Y coordinate
     */
    int center_y() const { return y + h / 2; }
};

/**
 * @brief Represents a dungeon map with rooms, tiles, and generation logic.
 */
class Dungeon {
public:
    /**
     * @brief Construct a default dungeon (80x40).
     */
    Dungeon();
    /**
     * @brief Construct a dungeon with given width and height.
     * @param width Width of the dungeon
     * @param height Height of the dungeon
     */
    Dungeon(int width, int height);

    /**
     * @brief Get the dungeon width.
     * @return Width in tiles
     */
    int width() const;
    /**
     * @brief Get the dungeon height.
     * @return Height in tiles
     */
    int height() const;

    /**
     * @brief Generate the dungeon layout and populate rooms.
     * @param seed Random seed
     * @param playerStart Output: player starting position
     * @param stairsDown Output: stairs down position
     * @param depth Dungeon depth/floor number
     */
    void generate(unsigned int seed, Position& playerStart, Position& stairsDown, int depth = 1);

    /**
     * @brief Check if coordinates are within dungeon bounds.
     * @param x X coordinate
     * @param y Y coordinate
     * @return True if in bounds
     */
    bool in_bounds(int x, int y) const;
    /**
     * @brief Get the tile type at given coordinates.
     * @param x X coordinate
     * @param y Y coordinate
     * @return Tile type
     */
    TileType get_tile(int x, int y) const;
    /**
     * @brief Set the tile type at given coordinates.
     * @param x X coordinate
     * @param y Y coordinate
     * @param t Tile type
     */
    void set_tile(int x, int y, TileType t);
    /**
     * @brief Check if a tile is walkable.
     * @param x X coordinate
     * @param y Y coordinate
     * @return True if walkable
     */
    bool is_walkable(int x, int y) const;
    /**
     * @brief Check if a tile is hazardous (trap, lava, etc.).
     * @param x X coordinate
     * @param y Y coordinate
     * @return True if hazardous
     */
    bool is_hazardous(int x, int y) const;
    /**
     * @brief Check if a tile is deadly (lava, chasm).
     * @param x X coordinate
     * @param y Y coordinate
     * @return True if deadly
     */
    bool is_deadly(int x, int y) const;

    /**
     * @brief Get all rooms in the dungeon.
     * @return Vector of rooms
     */
    const std::vector<Room>& rooms() const { return rooms_; }
    /**
     * @brief Get the room at given coordinates.
     * @param x X coordinate
     * @param y Y coordinate
     * @return Pointer to room or nullptr
     */
    const Room* get_room_at(int x, int y) const;
    /**
     * @brief Get the room type at given coordinates.
     * @param x X coordinate
     * @param y Y coordinate
     * @return Room type
     */
    RoomType get_room_type_at(int x, int y) const;

private:
    struct Rect {
        int x;
        int y;
        int w;
        int h;
    };

    void carve_room(const Rect& room);
    void carve_h_corridor(int x1, int x2, int y);
    void carve_v_corridor(int y1, int y2, int x);
    void assign_room_types(std::mt19937& rng, int depth);
    void populate_room(Room& room, std::mt19937& rng);

    int width_;
    int height_;
    std::vector<TileType> tiles_;
    std::vector<Room> rooms_;
};


