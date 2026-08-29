#include "../annealing.hpp"
#include "../rollback_array.hpp"
#include "../search_stats.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace {

struct FixedRng {
    using result_type = std::uint32_t;

    static constexpr result_type min() noexcept { return 0U; }
    static constexpr result_type max() noexcept {
        return std::numeric_limits<result_type>::max();
    }
    result_type operator()() noexcept { return 0U; }
};

template <class Exception, class Function>
void expect_throw(Function&& function) {
    bool thrown = false;
    try {
        std::forward<Function>(function)();
    } catch (const Exception&) {
        thrown = true;
    }
    assert(thrown);
}

void test_progress_and_temperature() {
    assert(ahc::progress(std::size_t{0}, std::size_t{10}) == 0.0);
    assert(ahc::progress(std::size_t{5}, std::size_t{10}) == 0.5);
    assert(ahc::progress(std::size_t{10}, std::size_t{10}) == 1.0);
    assert(ahc::progress(std::size_t{100}, std::size_t{10}) == 1.0);
    assert(ahc::progress(std::size_t{0}, std::size_t{0}) == 1.0);
    assert(ahc::progress(1, 10) == 0.1);
    assert(ahc::progress(-1.0, 10.0) == 0.0);
    assert(ahc::progress(10.0, 10.0) == 1.0);
    assert(ahc::progress(10.0, std::numeric_limits<double>::infinity()) ==
           0.0);
    assert(ahc::progress(std::numeric_limits<double>::quiet_NaN(), 10.0) ==
           1.0);

    const ahc::LinearTemperature linear(10.0, 2.0);
    assert(linear(0.0) == 10.0);
    assert(linear(0.25) == 8.0);
    assert(linear(1.0) == 2.0);
    assert(linear(-1.0) == 10.0);
    assert(linear(2.0) == 2.0);
    assert(linear(std::numeric_limits<double>::quiet_NaN()) == 10.0);
    expect_throw<std::invalid_argument>([] { ahc::LinearTemperature(-1, 1); });
    expect_throw<std::invalid_argument>([] {
        ahc::LinearTemperature(1, std::numeric_limits<double>::infinity());
    });

    const ahc::ExponentialTemperature exponential(100.0, 1.0);
    assert(exponential(0.0) == 100.0);
    assert(std::abs(exponential(0.5) - 10.0) < 1e-12);
    assert(exponential(1.0) == 1.0);
    expect_throw<std::invalid_argument>([] { ahc::ExponentialTemperature(0, 1); });
    expect_throw<std::invalid_argument>([] {
        ahc::ExponentialTemperature(1, std::numeric_limits<double>::quiet_NaN());
    });
}

void test_acceptance() {
    std::mt19937 first(20260829U);
    std::mt19937 second(20260829U);
    for (int i = 0; i < 256; ++i) {
        const bool lhs = ahc::accept_minimize(1.0, 0.75, first);
        const bool rhs = ahc::accept_minimize(1.0, 0.75, second);
        assert(lhs == rhs);
    }

    std::mt19937 rng(1U);
    assert(ahc::accept_minimize(-1.0, 0.0, rng));
    assert(ahc::accept_minimize(0.0, 0.0, rng));
    assert(!ahc::accept_minimize(1.0, 0.0, rng));
    assert(ahc::accept_maximize(1.0, 0.0, rng));
    assert(ahc::accept_maximize(0.0, 0.0, rng));
    assert(!ahc::accept_maximize(-1.0, 0.0, rng));
    assert(!ahc::accept_minimize(std::numeric_limits<double>::quiet_NaN(),
                                 1.0, rng));
    assert(ahc::accept_minimize(-std::numeric_limits<double>::infinity(), 0.0,
                                rng));
    assert(!ahc::accept_minimize(std::numeric_limits<double>::infinity(), 1.0,
                                 rng));
    FixedRng fixed;
    // 一様乱数0を、確率0（worsening=+∞）の受理に使ってはいけない。
    assert(!ahc::accept_minimize(std::numeric_limits<double>::infinity(), 1.0,
                                 fixed));
    assert(ahc::accept_maximize(std::numeric_limits<double>::infinity(), 0.0,
                                rng));
    assert(ahc::accept_minimize(1e308, 1e-308, rng) == false);
    assert(ahc::accept_candidate(ahc::Objective::minimize, 3.0, 2.0, 0.0,
                                 rng));
    assert(ahc::accept_candidate(ahc::Objective::maximize, 3.0, 4.0, 0.0,
                                 rng));
}

void test_stats() {
    ahc::SearchStats<double> stats(ahc::Objective::minimize);
    expect_throw<std::logic_error>([&stats] { (void)stats.best(); });
    stats.seed(10.0);
    assert(!stats.observe(12.0, false));
    assert(stats.observe(8.0, true));
    assert(!stats.observe(9.0, true));
    assert(stats.attempts() == 3U);
    assert(stats.accepted() == 2U);
    assert(stats.improved() == 1U);
    assert(stats.best() == 8.0);
    assert(std::abs(stats.acceptance_rate() - 2.0 / 3.0) < 1e-15);
    stats.reset();
    assert(stats.attempts() == 0U && stats.acceptance_rate() == 0.0);
    assert(stats.observe(4.0, true));
    expect_throw<std::invalid_argument>([&stats] {
        stats.seed(std::numeric_limits<double>::quiet_NaN());
    });

    ahc::SearchStats<int> maximize(ahc::Objective::maximize);
    maximize.seed(2);
    assert(maximize.observe(3, true));
    assert(!maximize.observe(1, true));
    assert(maximize.best() == 3);
}

void test_rollback_array() {
    ahc::RollbackArray<int> values{1, 2, 3};
    const auto outer = values.snapshot();
    values[0] = 10;
    const auto inner = values.snapshot();
    values.set(1, 20);
    values[2] = 30;
    values.rollback(inner);
    assert(values[0] == 10 && values[1] == 2 && values[2] == 3);
    values.commit(outer);
    assert(values[0] == 10);
    expect_throw<std::invalid_argument>([&values, &outer] { values.rollback(outer); });

    auto mark = values.snapshot();
    values.fill(7);
    values.rollback();
    assert(values[0] == 10 && values[1] == 2 && values[2] == 3);
    expect_throw<std::out_of_range>([&values] { values.at(3) = 0; });
    expect_throw<std::logic_error>([&values] { values.commit(); });

    const auto data_mark = values.snapshot();
    values.data()[1] = 99;
    values.rollback(data_mark);
    assert(values[1] == 2);

    ahc::RollbackArray<int> other(2, 0);
    expect_throw<std::invalid_argument>([&other, &mark] { other.rollback(mark); });
    expect_throw<std::invalid_argument>([&values] {
        ahc::RollbackArray<int>::Snapshot invalid;
        values.rollback(invalid);
    });

    auto copied = values;
    copied[0] = 42;
    assert(values[0] == 10 && copied[0] == 42);
}

}  // namespace

int main() {
    test_progress_and_temperature();
    test_acceptance();
    test_stats();
    test_rollback_array();
}
