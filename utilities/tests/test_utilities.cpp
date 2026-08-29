#include "../grid.hpp"
#include "../permutation_ops.hpp"
#include "../weighted_sampler.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

int main() {
    using ahc::Point;

    const Point origin{2, 3};
    assert(origin.row == 2 && origin.col == 3);
    assert(origin.r == 2 && origin.c == 3);
    assert(origin.x == 2 && origin.y == 3);
    assert((Point{1, 2} < Point{1, 3}));
    std::unordered_set<Point> points;
    points.insert(origin);
    assert(points.contains(Point{2, 3}));
    assert(ahc::inside(origin, 3, 4));
    assert(!ahc::inside(Point{-1, 0}, 3, 4));
    assert(ahc::manhattan_distance(Point{-2, 4}, Point{1, -1}) == 8);
    assert(ahc::chebyshev_distance(Point{-2, 4}, Point{1, -1}) == 5);
    assert((ahc::neighbors4(origin)[0] == Point{1, 3}));
    assert((ahc::neighbors8(origin)[1] == Point{1, 4}));
    assert(ahc::neighbors4(Point{0, 0}, 2, 2).size() == 2);

    std::vector<int> values{0, 1, 2, 3, 4};
    const ahc::SwapOperation swap{1, 3};
    assert(swap.apply(values));
    assert((values == std::vector<int>{0, 3, 2, 1, 4}));
    assert(swap.inverse().apply(values));
    assert((values == std::vector<int>{0, 1, 2, 3, 4}));

    const ahc::ReverseOperation reverse{1, 4};
    assert(reverse.apply(values));
    assert((values == std::vector<int>{0, 3, 2, 1, 4}));
    assert(reverse.inverse().apply(values));
    assert((values == std::vector<int>{0, 1, 2, 3, 4}));

    const ahc::RelocateOperation relocate{1, 4};
    assert(relocate.apply(values));
    assert((values == std::vector<int>{0, 2, 3, 4, 1}));
    assert(relocate.inverse().apply(values));
    assert((values == std::vector<int>{0, 1, 2, 3, 4}));
    assert(!ahc::swap_at(values, 0, values.size()));
    assert(!ahc::reverse_at(values, 4, 3));
    assert(!ahc::relocate_at(values, 5, 0));

    ahc::FenwickTree<long double> tree;
    tree.build(std::array<long double, 4>{0, 2, 0, 3});
    assert(tree.prefix_sum(2) == 2);
    assert(tree.lower_bound(0) == 1);
    assert(tree.lower_bound(1.999L) == 1);
    assert(tree.lower_bound(2) == 3);
    assert(tree.lower_bound(5) == tree.size());

    ahc::FenwickTree<int> dense_tree;
    dense_tree.build(std::array<int, 5>{1, 2, 4, 8, 16});
    const std::array<int, 6> dense_prefix{0, 1, 3, 7, 15, 31};
    for (std::size_t end = 0; end < dense_prefix.size(); ++end) {
        assert(dense_tree.prefix_sum(end) == dense_prefix[end]);
    }
    assert(dense_tree.total() == 31);
    for (int target = 0; target < 31; ++target) {
        const std::size_t expected = target < 1 ? 0 :
            target < 3 ? 1 : target < 7 ? 2 : target < 15 ? 3 : 4;
        assert(dense_tree.lower_bound(target) == expected);
    }

    ahc::WeightedSampler<double> sampler({0, 2, 0, 3});
    assert(sampler.total_weight() == 5);
    assert(sampler.sample_with([] { return -0.1L; }) == 1);
    assert(sampler.sample_with([] { return 0.0L; }) == 1);
    assert(sampler.sample_with([] { return 0.39L; }) == 1);
    assert(sampler.sample_with([] { return 0.8L; }) == 3);
    assert(sampler.sample_with([] { return 1.0L; }) == 3);
    assert(sampler.sample([] { return 0.0L; }) == 1);
    assert(!sampler.set(0, -1));
    assert(sampler.set(0, 1));
    assert(sampler.add(0, 2));
    assert(sampler.weight(0) == 3);

    ahc::WeightedSampler<int> empty({0, 0});
    assert(!empty.sample_with([] { return 0.5L; }).has_value());
    assert(!empty.set(2, 1));

    return 0;
}
