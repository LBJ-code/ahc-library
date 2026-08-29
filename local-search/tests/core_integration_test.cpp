#include "../../core/rng.hpp"
#include "../../core/timer.hpp"
#include "../annealing.hpp"
#include "../search_stats.hpp"

#include <cassert>
#include <chrono>

namespace {

struct FakeClock {
    using duration = std::chrono::milliseconds;
    using time_point = std::chrono::time_point<FakeClock>;

    static inline time_point current{};

    static time_point now() noexcept { return current; }
};

}  // namespace

int main() {
    using namespace std::chrono_literals;
    FakeClock::current = FakeClock::time_point{0ms};
    ahc::BasicTimer<FakeClock> timer(1s);
    ahc::Rng rng(42U);
    ahc::LinearTemperature temperature(10.0, 0.1);
    ahc::SearchStats<double> stats(ahc::Objective::minimize);

    double score = 100.0;
    stats.seed(score);
    FakeClock::current += 250ms;
    const double p = timer.progress();
    assert(p > 0.249 && p < 0.251);
    const double candidate = 99.0;
    const bool accepted = ahc::accept(
        ahc::Objective::minimize, candidate - score, temperature(p), rng);
    if (accepted) {
        score = candidate;
    }
    stats.observe(candidate, accepted);
    assert(stats.attempts() == 1U);
    assert(stats.accepted() <= stats.attempts());
    assert(score == 99.0);  // 改善は温度・乱数によらず受理される。
}
