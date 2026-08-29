// AI pre-created code: https://github.com/LBJ-code/ahc-library

#include "../core/rng.hpp"
#include "../core/timer.hpp"
#include "../core/zobrist.hpp"
#include "../local-search/annealing.hpp"
#include "../local-search/best_solution.hpp"
#include "../local-search/rollback_array.hpp"
#include "../local-search/search_stats.hpp"
#include "../utilities/grid.hpp"
#include "../utilities/grid_bfs.hpp"
#include "../utilities/permutation_ops.hpp"
#include "../utilities/weighted_sampler.hpp"

#include <cassert>
#include <chrono>
#include <vector>

int main() {
    ahc::Rng rng(20260829U);
    ahc::Timer timer(std::chrono::seconds(1));
    ahc::Zobrist zobrist(4, rng);

    std::vector<int> order{0, 1, 2};
    assert(ahc::swap_at(order, 0, 2));

    ahc::RollbackArray<int> values{1, 2};
    const auto snapshot = values.snapshot();
    values[0] = 9;
    values.rollback(snapshot);

    const auto bfs = ahc::bfs_grid(
        2, 2, [](ahc::Point) { return true; }, ahc::Point{0, 0});
    assert(bfs.distance_to(ahc::Point{1, 1}) == 2);

    ahc::WeightedSampler<int> sampler({1, 2});
    assert(sampler.sample(rng).has_value());

    ahc::BestSolution<std::vector<int>, int> best(ahc::Objective::maximize);
    assert(best.consider(order, 1));
    ahc::SearchStats<int> stats(ahc::Objective::maximize);
    stats.seed(0);
    assert(ahc::accept_maximize(1.0, 0.0, rng));

    assert(zobrist.size() == 4);
    assert(timer.progress() >= 0.0);
}
