#include "../rng.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

int main() {
  ahc::Rng first(0x123456789abcdef0ULL);
  ahc::Rng second(0x123456789abcdef0ULL);
  for (int i = 0; i < 32; ++i) {
    assert(first() == second());
  }
  static_assert(std::uniform_random_bit_generator<ahc::Rng>);

  ahc::Rng rng(42);
  std::vector<int> values(100);
  std::iota(values.begin(), values.end(), 0);
  std::shuffle(values.begin(), values.end(), rng);
  std::sort(values.begin(), values.end());
  for (int i = 0; i < 100; ++i) {
    assert(values[static_cast<std::size_t>(i)] == i);
  }

  for (int i = 0; i < 5000; ++i) {
    const int value = rng.uniform_int(-7, 13);
    assert(-7 <= value && value < 13);
    const unsigned unsigned_value = rng.uniform_int(3U, 9U);
    assert(3U <= unsigned_value && unsigned_value < 9U);
    const double real_value = rng.uniform_real(-2.0, 3.0);
    assert(-2.0 <= real_value && real_value < 3.0);
  }
  for (int i = 0; i < 5000; ++i) {
    const double wide = rng.uniform_real(-1.7e308, 1.7e308);
    assert(-1.7e308 <= wide && wide < 1.7e308);
  }
  assert(!rng.bernoulli(0.0));
  assert(rng.bernoulli(1.0));
  for (int i = 0; i < 1000; ++i) {
    const double value = rng.unit();
    assert(0.0 <= value && value < 1.0);
  }

  bool threw = false;
  try {
    (void)rng.uniform_int(3, 3);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
  threw = false;
  try {
    (void)rng.uniform_real(0.0, 0.0);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
  threw = false;
  try {
    (void)rng.bernoulli(1.1);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
  return 0;
}
