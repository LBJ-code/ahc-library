// AI事前作成コードの公開元: https://github.com/LBJ-code/ahc-library
#ifndef AHC_CORE_TIMER_HPP
#define AHC_CORE_TIMER_HPP

#include <algorithm>
#include <chrono>
#include <concepts>

namespace ahc {

/**
 * A small elapsed-time timer whose clock can be replaced in tests.
 *
 * `Clock` only needs the same interface as a std::chrono clock (`duration`,
 * `time_point`, and a static `now()` function).  The normal alias `Timer`
 * uses steady_clock, so a wall-clock adjustment cannot make a timer go back.
 */
template <typename Clock>
requires requires {
    typename Clock::duration;
    typename Clock::time_point;
    { Clock::now() } -> std::same_as<typename Clock::time_point>;
}
class BasicTimer {
 public:
  using clock_type = Clock;
  using duration = typename Clock::duration;
  using time_point = typename Clock::time_point;

  /** Start now. A non-positive limit expires immediately. */
  explicit BasicTimer(duration limit) : BasicTimer(limit, Clock::now()) {}

  /** Start at an explicitly supplied time point (useful with a fake clock). */
  BasicTimer(duration limit, time_point start) noexcept
      : start_(start), limit_(normalise_limit(limit)) {}

  /** Same as BasicTimer(limit, start), with an argument order convenient in tests. */
  BasicTimer(time_point start, duration limit) noexcept
      : BasicTimer(limit, start) {}

  [[nodiscard]] static BasicTimer from_start(time_point start,
                                              duration limit) noexcept {
    return BasicTimer(limit, start);
  }

  /** Move the start point to now. */
  void reset() { start_ = Clock::now(); }

  /** Move the start point to an explicitly supplied time point. */
  void reset(time_point start) noexcept { start_ = start; }

  [[nodiscard]] time_point start_time() const noexcept { return start_; }
  [[nodiscard]] duration limit() const noexcept { return limit_; }

  /** Elapsed time. It is never negative, and may be greater than limit(). */
  [[nodiscard]] duration elapsed() const { return elapsed_at(Clock::now()); }

  /** Deterministic overload for tests or a caller's already captured timestamp. */
  [[nodiscard]] duration elapsed_at(time_point now) const noexcept {
    if (now <= start_) {
      return duration::zero();
    }
    return now - start_;
  }

  /** Time left, clamped to zero after the deadline. */
  [[nodiscard]] duration remaining() const {
    return remaining_at(Clock::now());
  }

  [[nodiscard]] duration remaining_at(time_point now) const noexcept {
    const duration spent = elapsed_at(now);
    if (spent >= limit_) {
      return duration::zero();
    }
    return limit_ - spent;
  }

  /** Progress from 0.0 to 1.0, clamped at both ends. */
  [[nodiscard]] double progress() const { return progress_at(Clock::now()); }

  [[nodiscard]] double progress_at(time_point now) const noexcept {
    if (limit_ == duration::zero()) {
      return 1.0;
    }
    const duration spent = elapsed_at(now);
    if (spent <= duration::zero()) {
      return 0.0;
    }
    if (spent >= limit_) {
      return 1.0;
    }
    const long double numerator = static_cast<long double>(spent.count());
    const long double denominator = static_cast<long double>(limit_.count());
    const long double fraction = numerator / denominator;
    // The comparisons above are enough for ordinary clocks. Keep the public
    // contract closed under unusual duration representations and rounding.
    return static_cast<double>(std::clamp(fraction, 0.0L, 1.0L));
  }

  [[nodiscard]] bool expired() const { return expired_at(Clock::now()); }

  [[nodiscard]] bool expired_at(time_point now) const noexcept {
    return elapsed_at(now) >= limit_;
  }

 private:
  [[nodiscard]] static duration normalise_limit(duration limit) noexcept {
    return limit < duration::zero() ? duration::zero() : limit;
  }

  time_point start_;
  duration limit_;
};

/** The contest-facing timer: monotonic std::chrono::steady_clock. */
using Timer = BasicTimer<std::chrono::steady_clock>;

}  // namespace ahc

#endif  // AHC_CORE_TIMER_HPP
