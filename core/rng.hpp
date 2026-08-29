// AI事前作成コードの公開元: https://github.com/LBJ-code/ahc-library
#ifndef AHC_CORE_RNG_HPP
#define AHC_CORE_RNG_HPP

#include <bit>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace ahc {

/**
 * A fast, deterministic xoshiro256** generator.
 *
 * Constructing an object always requires a seed. The object owns its state;
 * there is no process-wide random state and two objects with the same seed
 * produce the same raw sequence.
 */
class Rng {
 public:
  using result_type = std::uint64_t;

  explicit Rng(std::uint64_t seed) noexcept { reseed(seed); }
  Rng() = delete;

  [[nodiscard]] static constexpr result_type min() noexcept { return 0; }
  [[nodiscard]] static constexpr result_type max() noexcept {
    return std::numeric_limits<result_type>::max();
  }

  /** Return the next uniformly distributed 64-bit value. */
  result_type operator()() noexcept {
    const result_type result = std::rotl(state_[1] * 5U, 7) * 9U;
    const result_type t = state_[1] << 17U;

    state_[2] ^= state_[0];
    state_[3] ^= state_[1];
    state_[1] ^= state_[2];
    state_[0] ^= state_[3];
    state_[2] ^= t;
    state_[3] = std::rotl(state_[3], 45);
    return result;
  }

  /** Start the deterministic sequence over with a new explicit seed. */
  void reseed(std::uint64_t seed) noexcept {
    std::uint64_t stream = seed;
    for (auto& word : state_) {
      word = splitmix64(stream);
    }
    // xoshiro's all-zero state is absorbing. This guard is deterministic and
    // has no effect for ordinary seeds (including seed 0).
    if (state_[0] == 0 && state_[1] == 0 && state_[2] == 0 && state_[3] == 0) {
      state_[0] = 0x9e3779b97f4a7c15ULL;
    }
  }

  /**
   * Integer in the half-open interval [lower, upper).
   *
   * The bounds must satisfy lower < upper. Invalid bounds throw
   * std::invalid_argument instead of silently swapping or widening them.
   */
  template <std::integral Int>
  requires(!std::same_as<std::remove_cv_t<Int>, bool>)
  Int uniform_int(Int lower, Int upper) noexcept(false) {
    if (!(lower < upper)) {
      throw std::invalid_argument("ahc::Rng::uniform_int requires lower < upper");
    }

    using Unsigned = std::make_unsigned_t<Int>;
    // In unsigned arithmetic this is the exact span even when signed bounds
    // cross zero. A valid half-open interval cannot have a span of 2^N.
    const Unsigned span = static_cast<Unsigned>(upper) -
                          static_cast<Unsigned>(lower);
    if (span == 0) {
      throw std::invalid_argument("ahc::Rng::uniform_int interval is too wide");
    }

    // Rejection removes modulo bias. The cast keeps the generator's low N
    // bits, which are uniformly distributed for every integral N <= 64.
    // Cast before the modulo: uint8_t/uint16_t promote to int, whose negative
    // remainder would otherwise lose the intended 2^N wraparound.
    const Unsigned threshold =
        static_cast<Unsigned>(Unsigned{0} - span) % span;
    Unsigned draw;
    do {
      draw = static_cast<Unsigned>((*this)());
    } while (draw < threshold);

    const Unsigned value = static_cast<Unsigned>(lower) + draw % span;
    return from_unsigned<Int>(value);
  }

  /** Real number in the half-open interval [lower, upper). */
  template <std::floating_point Real>
  Real uniform_real(Real lower, Real upper) {
    if (!std::isfinite(lower) || !std::isfinite(upper) || !(lower < upper)) {
      throw std::invalid_argument(
          "ahc::Rng::uniform_real requires finite lower < upper");
    }

    // std::lerp avoids overflowing in (upper - lower) for finite endpoints.
    Real value = std::lerp(lower, upper, static_cast<Real>(unit()));
    // Rounding is allowed to land exactly on the upper endpoint. Keep the
    // documented half-open boundary even for very large or adjacent values.
    if (!(value < upper)) {
      value = std::nextafter(upper, lower);
    }
    if (value < lower) {
      value = lower;
    }
    return value;
  }

  /** Bernoulli trial with probability p of true; p must be in [0, 1]. */
  bool bernoulli(double probability) {
    if (!std::isfinite(probability) || probability < 0.0 || probability > 1.0) {
      throw std::invalid_argument(
          "ahc::Rng::bernoulli requires 0 <= probability <= 1");
    }
    return unit() < probability;
  }

  /** A uniformly distributed value in [0, 1). */
  double unit() noexcept {
    // 53 bits are exactly representable in double. The denominator is 2^53.
    constexpr double denominator = 9007199254740992.0;
    return static_cast<double>((*this)() >> 11U) / denominator;
  }

  // Short aliases that read naturally in contest code.
  template <std::integral Int>
  requires(!std::same_as<std::remove_cv_t<Int>, bool>)
  Int integer(Int lower, Int upper) {
    return uniform_int(lower, upper);
  }

  template <std::floating_point Real>
  Real real(Real lower, Real upper) {
    return uniform_real(lower, upper);
  }

 private:
  [[nodiscard]] static result_type splitmix64(result_type& value) noexcept {
    result_type z = (value += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31U);
  }

  template <std::integral Int>
  [[nodiscard]] static Int from_unsigned(
      std::make_unsigned_t<Int> value) noexcept {
    using Unsigned = std::make_unsigned_t<Int>;
    if constexpr (std::is_unsigned_v<Int>) {
      return value;
    } else {
      // Avoid an implementation-defined unsigned-to-signed conversion for
      // the negative half on the usual two's-complement integer model.
      constexpr Unsigned sign_bit =
          Unsigned{1} << (std::numeric_limits<Unsigned>::digits - 1);
      if (value < sign_bit) {
        return static_cast<Int>(value);
      }
      return std::numeric_limits<Int>::min() +
             static_cast<Int>(value - sign_bit);
    }
  }

  result_type state_[4]{};
};

}  // namespace ahc

#endif  // AHC_CORE_RNG_HPP
