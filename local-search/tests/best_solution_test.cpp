#include "../best_solution.hpp"

#include <cassert>
#include <limits>
#include <stdexcept>
#include <string>

int main() {
    ahc::BestSolution<std::string, int> best(ahc::Objective::minimize);
    assert(best.empty());
    try {
        (void)best.state();
        assert(false);
    } catch (const std::logic_error&) {
    }
    assert(best.consider(std::string("first"), 10));
    assert(!best.consider(std::string("worse"), 11));
    assert(!best.consider(std::string("tie"), 10));
    assert(best.consider(std::string("better"), 3));
    assert(best.state() == "better" && best.score() == 3);
    best.clear();
    assert(best.empty());

    ahc::BestSolution<int> maximize(ahc::Objective::maximize);
    assert(maximize.consider(1, 2.0));
    assert(!maximize.consider(2, 1.0));
    assert(maximize.consider(3, 4.0));
    assert(maximize.best_state() == 3 && maximize.best_score() == 4.0);
    assert(!maximize.consider(4, std::numeric_limits<double>::quiet_NaN()));
}
