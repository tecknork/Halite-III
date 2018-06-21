#include "Map.hpp"

#include "nlohmann/json.hpp"

/** Get a field from JSON. */
#define FIELD_FROM_JSON(x) json.at(#x)

namespace hlt {

/**
 * Given a location of a cell, return its neighbors.
 * @param location The location of the cell we want the neighbors of.
 * @return Array of neighbor locations.
 *  A neighbor is a location with Manhattan distance 1 from the input location.
 *  This function encapsulates the wrap-around map -
 *  i.e. cell (0, 0)'s neighbors include cells at the very bottom and very right of the map.
 */
std::array<Location, Map::NEIGHBOR_COUNT> Map::get_neighbors(const Location &location) const {
    // Allow wrap around neighbors
    const auto x = location.first;
    const auto y = location.second;
    return {{{(x + 1) % width, y},
             {(x - 1 + width) % width, y},
             {x, (y + 1) % height},
             {x, (y - 1 + height) % height}}};
}

/**
 * Calculate the Manhattan distance between two cells on a grid.
 *
 * @param from location of the first cell.
 * @param to The location of the second cell.
 * @return The Manhattan distance between the cells, calculated on a wrap-around map.
 */
dimension_type Map::distance(const Location &from, const Location &to) const {
    const auto x_dist = abs(from.first - to.first);
    const auto y_dist = abs(from.second - to.second);
    return std::min(x_dist, width - x_dist) + std::min(y_dist, height - y_dist);
}

/**
 * Convert an encoded Map from JSON format.
 * @param json The JSON.
 * @param[out] map The converted Map.
 */
void from_json(const nlohmann::json &json, Map &map) {
    map = {FIELD_FROM_JSON(width),
           FIELD_FROM_JSON(height),
           FIELD_FROM_JSON(grid)};
}

/**
 * Write a Map to bot serial format.
 * @param ostream The output stream.
 * @param map The Map to write.
 * @return The output stream.
 */
std::ostream &operator<<(std::ostream &os, const Map &map) {
    // Output the map dimensions.
    os << map.width << " " << map.height << std::endl;
    // Output the cells one after another.
    for (const auto &row : map.grid) {
        for (const auto &cell : row) {
            os << cell;
        }
    }
    return os;
}

/**
 * Move a location in a direction.
 * @param location The location to move.
 * @param direction The direction to move it in.
 */
void Map::move_location(Location &location, const Direction &direction) {
    auto &x = location.first;
    auto &y = location.second;
    switch (direction) {
    case Direction::North:
        y = (y + height - 1) % height;
        break;
    case Direction::South:
        y = (y + 1) % height;
        break;
    case Direction::East:
        x = (x + 1) % width;
        break;
    case Direction::West:
        x = (x + width - 1) % width;
        break;
    }
}

}
