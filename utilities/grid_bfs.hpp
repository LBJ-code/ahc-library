// AI pre-created code: https://github.com/LBJ-code/ahc-library
// Header-only four-neighbor BFS for bounded grid paths.
#ifndef AHC_UTILITIES_GRID_BFS_HPP
#define AHC_UTILITIES_GRID_BFS_HPP

#include "grid.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <limits>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ahc {

// A row-major grid BFS result.  `distance` and `parent` have one entry per
// cell when height and width are positive.  A distance of -1 means that the
// cell is blocked or unreachable; {-1, -1} means that the cell has no parent
// (a source, a blocked cell, or an unreachable cell).
struct GridBfsResult {
    int height = 0;
    int width = 0;
    std::vector<int> distance;
    std::vector<Point> parent;

    [[nodiscard]] static constexpr Point no_parent() noexcept {
        return Point{-1, -1};
    }

    // Returns whether p is a cell represented by this result.  For a result
    // made with non-positive dimensions, no cell is represented.
    [[nodiscard]] bool in_bounds(Point p) const noexcept {
        if (!ahc::inside(p, height, width)) {
            return false;
        }
        const std::size_t index = flat_index(p);
        return index < distance.size() && index < parent.size();
    }

    [[nodiscard]] bool reachable(Point p) const noexcept {
        return in_bounds(p) && distance[flat_index(p)] >= 0;
    }

    [[nodiscard]] bool is_reachable(Point p) const noexcept {
        return reachable(p);
    }

    // Returns -1 for an out-of-bounds, blocked, or unreachable cell.
    [[nodiscard]] int distance_to(Point p) const noexcept {
        return reachable(p) ? distance[flat_index(p)] : -1;
    }

    // Returns no_parent() when p is invalid, blocked, unreachable, or a
    // source.  A reachable non-source cell points to a cell one step closer
    // to one of the supplied sources.
    [[nodiscard]] Point parent_of(Point p) const noexcept {
        return in_bounds(p) ? parent[flat_index(p)] : no_parent();
    }

    // Reconstructs a path from one of the valid sources to goal, inclusive.
    // An invalid or unreachable goal produces an empty vector.  For a source,
    // the result is the one-element path {source}.
    [[nodiscard]] std::vector<Point> path_to(Point goal) const {
        if (!reachable(goal)) {
            return {};
        }

        std::vector<Point> reversed_path;
        const int goal_distance = distance_to(goal);
        const std::size_t requested_capacity =
            static_cast<std::size_t>(goal_distance) + 1U;
        if (requested_capacity <= distance.size()) {
            reversed_path.reserve(requested_capacity);
        }

        Point current = goal;
        // A valid BFS path visits at most every cell once.  The bound also
        // keeps this accessor safe if a caller manually alters public fields.
        for (std::size_t step = 0; step < distance.size(); ++step) {
            if (!reachable(current)) {
                return {};
            }
            reversed_path.push_back(current);

            const int current_distance = distance_to(current);
            const Point predecessor = parent_of(current);
            if (predecessor == no_parent()) {
                if (current_distance != 0) {
                    return {};
                }
                std::reverse(reversed_path.begin(), reversed_path.end());
                return reversed_path;
            }
            if (!reachable(predecessor) ||
                distance_to(predecessor) != current_distance - 1) {
                return {};
            }
            current = predecessor;
        }
        return {};
    }

    [[nodiscard]] std::vector<Point> reconstruct_path(Point goal) const {
        return path_to(goal);
    }

private:
    [[nodiscard]] std::size_t flat_index(Point p) const noexcept {
        return static_cast<std::size_t>(p.row) *
                   static_cast<std::size_t>(width) +
               static_cast<std::size_t>(p.col);
    }
};

namespace detail {

[[nodiscard]] inline std::size_t checked_grid_cell_count(
    int height,
    int width
) {
    if (height <= 0 || width <= 0) {
        return 0;
    }

    const std::size_t height_size = static_cast<std::size_t>(height);
    const std::size_t width_size = static_cast<std::size_t>(width);
    if (height_size > std::numeric_limits<std::size_t>::max() / width_size) {
        throw std::length_error("grid dimensions overflow the cell count");
    }
    const std::size_t cell_count = height_size * width_size;
    if (cell_count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("grid has too many cells for int distances");
    }
    return cell_count;
}

template<class Passable, std::ranges::input_range Starts>
GridBfsResult run_grid_bfs(
    int height,
    int width,
    Passable&& passable,
    Starts&& starts
) {
    GridBfsResult result;
    result.height = height;
    result.width = width;

    const std::size_t cell_count = checked_grid_cell_count(height, width);
    if (cell_count == 0) {
        return result;
    }

    result.distance.assign(cell_count, -1);
    result.parent.assign(cell_count, GridBfsResult::no_parent());

    // Evaluate the predicate exactly once for each in-bounds cell.  This
    // makes source validation and traversal agree even for a stateful caller.
    std::vector<unsigned char> passable_cell(cell_count, 0U);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const std::size_t index =
                static_cast<std::size_t>(row) * static_cast<std::size_t>(width) +
                static_cast<std::size_t>(col);
            passable_cell[index] =
                std::invoke(passable, Point{row, col}) ? 1U : 0U;
        }
    }

    std::queue<std::size_t> pending;
    for (const auto& source_value : starts) {
        const Point source = static_cast<Point>(source_value);
        if (!ahc::inside(source, height, width)) {
            continue;
        }
        const std::size_t source_index =
            static_cast<std::size_t>(source.row) * static_cast<std::size_t>(width) +
            static_cast<std::size_t>(source.col);
        if (passable_cell[source_index] == 0U ||
            result.distance[source_index] >= 0) {
            continue;
        }
        result.distance[source_index] = 0;
        pending.push(source_index);
    }

    while (!pending.empty()) {
        const std::size_t current_index = pending.front();
        pending.pop();
        const Point current{
            static_cast<int>(current_index / static_cast<std::size_t>(width)),
            static_cast<int>(current_index % static_cast<std::size_t>(width))
        };

        for (const Point direction : directions4) {
            const Point next = current + direction;
            if (!ahc::inside(next, height, width)) {
                continue;
            }
            const std::size_t next_index =
                static_cast<std::size_t>(next.row) * static_cast<std::size_t>(width) +
                static_cast<std::size_t>(next.col);
            if (passable_cell[next_index] == 0U ||
                result.distance[next_index] >= 0) {
                continue;
            }
            result.distance[next_index] = result.distance[current_index] + 1;
            result.parent[next_index] = current;
            pending.push(next_index);
        }
    }
    return result;
}

}  // namespace detail

// Run four-neighbor BFS from one source.  The source is ignored when it is
// outside the grid or rejected by passable.
template<class Passable>
requires std::predicate<Passable&, Point>
[[nodiscard]] GridBfsResult bfs_grid(
    int height,
    int width,
    Passable&& passable,
    Point start
) {
    const std::array<Point, 1> starts{start};
    return detail::run_grid_bfs(
        height,
        width,
        std::forward<Passable>(passable),
        starts
    );
}

// Run four-neighbor BFS from every valid source in starts.  Sources outside
// the grid, on blocked cells, and duplicate sources are ignored.  Any
// input_range whose elements convert to Point is accepted, including vector,
// array, span, and initializer_list.
template<class Passable, std::ranges::input_range Starts>
requires std::predicate<Passable&, Point> &&
         std::convertible_to<std::ranges::range_reference_t<Starts>, Point>
[[nodiscard]] GridBfsResult bfs_grid(
    int height,
    int width,
    Passable&& passable,
    Starts&& starts
) {
    return detail::run_grid_bfs(
        height,
        width,
        std::forward<Passable>(passable),
        std::forward<Starts>(starts)
    );
}

// Convenience overload for a braced list of sources, for which a generic
// range parameter cannot be deduced.
template<class Passable>
requires std::predicate<Passable&, Point>
[[nodiscard]] GridBfsResult bfs_grid(
    int height,
    int width,
    Passable&& passable,
    std::initializer_list<Point> starts
) {
    return detail::run_grid_bfs(
        height,
        width,
        std::forward<Passable>(passable),
        starts
    );
}

[[nodiscard]] inline std::vector<Point> reconstruct_path(
    const GridBfsResult& result,
    Point goal
) {
    return result.path_to(goal);
}

}  // namespace ahc

#endif  // AHC_UTILITIES_GRID_BFS_HPP
