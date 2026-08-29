// AI pre-created code: https://github.com/LBJ-code/ahc-library
#include "../grid_bfs.hpp"

#include <array>
#include <cassert>
#include <string>
#include <vector>

namespace {

[[nodiscard]] bool is_open(const std::vector<std::string>& map, ahc::Point p) {
    return map[static_cast<std::size_t>(p.row)][static_cast<std::size_t>(p.col)] != '#';
}

void assert_valid_path(
    const std::vector<ahc::Point>& path,
    ahc::Point start,
    ahc::Point goal
) {
    assert(!path.empty());
    assert(path.front() == start);
    assert(path.back() == goal);
    for (std::size_t i = 1; i < path.size(); ++i) {
        assert(ahc::manhattan_distance(path[i - 1], path[i]) == 1);
    }
}

}  // namespace

int main() {
    using ahc::Point;

    const std::vector<std::string> map{
        ".....",
        ".###.",
        "....."
    };
    const auto passable = [&map](Point point) { return is_open(map, point); };

    const ahc::GridBfsResult result =
        ahc::bfs_grid(3, 5, passable, Point{0, 0});
    assert(result.height == 3 && result.width == 5);
    assert(result.distance.size() == 15);
    assert(result.parent.size() == 15);
    assert(result.distance_to(Point{0, 4}) == 4);
    assert(result.distance_to(Point{2, 4}) == 6);
    assert(result.reachable(Point{2, 4}));
    assert(result.is_reachable(Point{2, 4}));
    assert(!result.reachable(Point{1, 2}));
    assert(result.distance_to(Point{-1, 0}) == -1);
    assert(result.parent_of(Point{0, 0}) == ahc::GridBfsResult::no_parent());
    assert((result.parent_of(Point{0, 1}) == Point{0, 0}));
    assert_valid_path(result.path_to(Point{2, 4}), Point{0, 0}, Point{2, 4});
    assert_valid_path(
        ahc::reconstruct_path(result, Point{2, 4}),
        Point{0, 0},
        Point{2, 4}
    );

    // A blocked region makes a valid in-bounds goal unreachable.
    const std::vector<std::string> separated{
        "...",
        "###",
        "..."
    };
    const auto separated_result = ahc::bfs_grid(
        3,
        3,
        [&separated](Point point) { return is_open(separated, point); },
        Point{0, 0}
    );
    assert(!separated_result.reachable(Point{2, 2}));
    assert(separated_result.distance_to(Point{2, 2}) == -1);
    assert(separated_result.parent_of(Point{1, 1}) == ahc::GridBfsResult::no_parent());
    assert(separated_result.path_to(Point{2, 2}).empty());

    // A source is a zero-length path, including its parent sentinel.
    const auto source_result = ahc::bfs_grid(
        2,
        2,
        [](Point) { return true; },
        Point{1, 1}
    );
    assert(source_result.distance_to(Point{1, 1}) == 0);
    assert((source_result.path_to(Point{1, 1}) == std::vector<Point>{Point{1, 1}}));
    assert(source_result.parent_of(Point{1, 1}) == ahc::GridBfsResult::no_parent());

    // Multi-source BFS assigns the nearest source and reconstructs from it.
    const std::vector<Point> starts{
        Point{0, 0}, Point{2, 4}, Point{-1, 0}, Point{2, 4}
    };
    const auto multi_result = ahc::bfs_grid(
        3,
        5,
        passable,
        starts
    );
    assert(multi_result.distance_to(Point{0, 4}) == 2);
    assert_valid_path(
        multi_result.path_to(Point{0, 4}),
        Point{2, 4},
        Point{0, 4}
    );

    // Invalid and blocked sources are ignored without changing the result.
    const std::vector<Point> invalid_starts{
        Point{-1, 0}, Point{3, 0}, Point{0, 5}, Point{1, 2}
    };
    const auto invalid_result = ahc::bfs_grid(
        3,
        5,
        passable,
        invalid_starts
    );
    for (const int distance : invalid_result.distance) {
        assert(distance == -1);
    }
    assert(invalid_result.path_to(Point{0, 0}).empty());

    // Non-positive dimensions represent an empty grid and do not invoke the
    // predicate.  This is part of the boundary contract.
    int predicate_calls = 0;
    const auto empty_result = ahc::bfs_grid(
        0,
        4,
        [&predicate_calls](Point) {
            ++predicate_calls;
            return true;
        },
        Point{0, 0}
    );
    assert(empty_result.distance.empty());
    assert(empty_result.parent.empty());
    assert(!empty_result.reachable(Point{0, 0}));
    assert(predicate_calls == 0);

    const auto negative_result = ahc::bfs_grid(
        -1,
        4,
        [&predicate_calls](Point) {
            ++predicate_calls;
            return true;
        },
        Point{0, 0}
    );
    assert(negative_result.distance.empty());
    assert(negative_result.parent.empty());
    assert(predicate_calls == 0);

    return 0;
}
