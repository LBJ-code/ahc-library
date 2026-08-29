// AI pre-created code: https://github.com/LBJ-code/ahc-library
// Header-only grid primitives for AtCoder Heuristic Contest programs.
#ifndef AHC_UTILITIES_GRID_HPP
#define AHC_UTILITIES_GRID_HPP

#include <array>
#include <compare>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace ahc {

// A grid coordinate.  row/col are the canonical names.  r/c and x/y are
// aliases kept in the object so that either common naming convention can be
// used without a conversion or a second coordinate.
struct Point {
    union {
        int row;
        int r;
        int x;
    };
    union {
        int col;
        int c;
        int y;
    };

    constexpr Point() noexcept : row(0), col(0) {}
    constexpr Point(int row_value, int col_value) noexcept
        : row(row_value), col(col_value) {}

    [[nodiscard]] constexpr auto operator<=>(const Point& other) const noexcept {
        if (const auto row_order = row <=> other.row; row_order != 0) {
            return row_order;
        }
        return col <=> other.col;
    }

    [[nodiscard]] constexpr bool operator==(const Point& other) const noexcept {
        return row == other.row && col == other.col;
    }
};

static_assert(std::is_trivially_copyable_v<Point>);

[[nodiscard]] constexpr Point operator+(Point lhs, Point rhs) noexcept {
    return {lhs.row + rhs.row, lhs.col + rhs.col};
}

[[nodiscard]] constexpr Point operator-(Point lhs, Point rhs) noexcept {
    return {lhs.row - rhs.row, lhs.col - rhs.col};
}

[[nodiscard]] constexpr Point operator-(Point point) noexcept {
    return {-point.row, -point.col};
}

constexpr Point& operator+=(Point& lhs, Point rhs) noexcept {
    lhs.row += rhs.row;
    lhs.col += rhs.col;
    return lhs;
}

constexpr Point& operator-=(Point& lhs, Point rhs) noexcept {
    lhs.row -= rhs.row;
    lhs.col -= rhs.col;
    return lhs;
}

// Directions are ordered clockwise from up.  The order is deterministic and
// is useful when a search wants a stable tie-break rule.
inline constexpr std::array<Point, 4> directions4{
    Point{-1, 0}, Point{0, 1}, Point{1, 0}, Point{0, -1}
};

inline constexpr std::array<Point, 8> directions8{
    Point{-1, 0}, Point{-1, 1}, Point{0, 1}, Point{1, 1},
    Point{1, 0}, Point{1, -1}, Point{0, -1}, Point{-1, -1}
};

// Upper-case aliases are convenient when the constants are used beside
// local variables named directions4/directions8.
inline constexpr auto& kDirections4 = directions4;
inline constexpr auto& kDirections8 = directions8;

[[nodiscard]] constexpr bool inside(Point point, int height, int width) noexcept {
    return height > 0 && width > 0 && point.row >= 0 && point.col >= 0 &&
           point.row < height && point.col < width;
}

[[nodiscard]] constexpr bool inside(int row, int col, int height, int width) noexcept {
    return inside(Point{row, col}, height, width);
}

[[nodiscard]] constexpr bool inside(int height, int width, Point point) noexcept {
    return inside(point, height, width);
}

[[nodiscard]] constexpr long long manhattan_distance(Point lhs, Point rhs) noexcept {
    // Promote before subtracting.  This avoids signed-int overflow for valid
    // points near INT_MIN/INT_MAX.
    const long long row_delta = static_cast<long long>(lhs.row) - rhs.row;
    const long long col_delta = static_cast<long long>(lhs.col) - rhs.col;
    const auto absolute = [](long long value) constexpr noexcept {
        return value < 0 ? -value : value;
    };
    return absolute(row_delta) + absolute(col_delta);
}

[[nodiscard]] constexpr long long manhattan(Point point) noexcept {
    return manhattan_distance(point, Point{});
}

[[nodiscard]] constexpr long long chebyshev_distance(Point lhs, Point rhs) noexcept {
    const long long row_delta = lhs.row >= rhs.row
        ? static_cast<long long>(lhs.row) - rhs.row
        : static_cast<long long>(rhs.row) - lhs.row;
    const long long col_delta = lhs.col >= rhs.col
        ? static_cast<long long>(lhs.col) - rhs.col
        : static_cast<long long>(rhs.col) - lhs.col;
    return row_delta > col_delta ? row_delta : col_delta;
}

[[nodiscard]] constexpr long long chebyshev(Point point) noexcept {
    return chebyshev_distance(point, Point{});
}

[[nodiscard]] constexpr std::array<Point, 4> neighbors4(Point point) noexcept {
    std::array<Point, 4> result{};
    for (std::size_t i = 0; i < directions4.size(); ++i) {
        result[i] = point + directions4[i];
    }
    return result;
}

[[nodiscard]] constexpr std::array<Point, 8> neighbors8(Point point) noexcept {
    std::array<Point, 8> result{};
    for (std::size_t i = 0; i < directions8.size(); ++i) {
        result[i] = point + directions8[i];
    }
    return result;
}

[[nodiscard]] inline std::vector<Point> neighbors4(
    Point point,
    int height,
    int width
) {
    std::vector<Point> result;
    result.reserve(directions4.size());
    for (const Point candidate : neighbors4(point)) {
        if (inside(candidate, height, width)) {
            result.push_back(candidate);
        }
    }
    return result;
}

[[nodiscard]] inline std::vector<Point> neighbors8(
    Point point,
    int height,
    int width
) {
    std::vector<Point> result;
    result.reserve(directions8.size());
    for (const Point candidate : neighbors8(point)) {
        if (inside(candidate, height, width)) {
            result.push_back(candidate);
        }
    }
    return result;
}

// Verbose aliases make the filtering behavior obvious at call sites.
[[nodiscard]] inline std::vector<Point> neighbors4_inside(
    Point point,
    int height,
    int width
) {
    return neighbors4(point, height, width);
}

[[nodiscard]] inline std::vector<Point> neighbors8_inside(
    Point point,
    int height,
    int width
) {
    return neighbors8(point, height, width);
}

struct PointHash {
    [[nodiscard]] std::size_t operator()(Point point) const noexcept {
        // splitmix64 gives stable, inexpensive mixing for the two signed
        // coordinates.  It does not depend on the implementation's hash<int>.
        std::uint64_t value =
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(point.row)) << 32) |
            static_cast<std::uint32_t>(point.col);
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        value ^= value >> 31;
        return static_cast<std::size_t>(value);
    }
};

}  // namespace ahc

namespace std {

template<>
struct hash<ahc::Point> {
    [[nodiscard]] size_t operator()(ahc::Point point) const noexcept {
        return ahc::PointHash{}(point);
    }
};

}  // namespace std

#endif  // AHC_UTILITIES_GRID_HPP
