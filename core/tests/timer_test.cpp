#include "../timer.hpp"

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
  using TestTimer = ahc::BasicTimer<FakeClock>;

  FakeClock::current = FakeClock::time_point{100ms};
  TestTimer timer(1s);
  assert(timer.elapsed() == 0ms);
  assert(timer.remaining() == 1s);
  assert(timer.progress() == 0.0);
  assert(!timer.expired());

  FakeClock::current += 250ms;
  assert(timer.elapsed() == 250ms);
  assert(timer.remaining() == 750ms);
  assert(timer.progress() > 0.249 && timer.progress() < 0.251);

  FakeClock::current += 1s;
  assert(timer.elapsed() == 1250ms);
  assert(timer.remaining() == 0ms);
  assert(timer.progress() == 1.0);
  assert(timer.expired());

  // A clock sample before start is clamped, which also makes fake-clock tests
  // deterministic when they intentionally move time backwards.
  assert(timer.elapsed_at(FakeClock::time_point{0ms}) == 0ms);
  assert(timer.remaining_at(FakeClock::time_point{0ms}) == 1s);

  TestTimer immediate(-1ms, FakeClock::time_point{0ms});
  assert(immediate.limit() == 0ms);
  assert(immediate.expired_at(FakeClock::time_point{0ms}));
  assert(immediate.progress_at(FakeClock::time_point{0ms}) == 1.0);

  immediate.reset(FakeClock::time_point{2s});
  assert(immediate.start_time() == FakeClock::time_point{2s});
  return 0;
}
