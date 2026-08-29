// AI事前作成コードの公開元: https://github.com/LBJ-code/ahc-library
#ifndef AHC_CORE_ZOBRIST_HPP
#define AHC_CORE_ZOBRIST_HPP

#include "rng.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace ahc {

/** A Zobrist token is one deterministic, independently generated 64-bit word. */
using ZobristToken = std::uint64_t;

/** Generate one token from the caller-owned RNG stream. */
[[nodiscard]] inline ZobristToken next_zobrist_token(Rng& rng) noexcept {
  return rng();
}

/** Toggle one token in a hash. Applying the same token twice restores hash. */
[[nodiscard]] constexpr std::uint64_t zobrist_update(
    std::uint64_t hash, ZobristToken token) noexcept {
  return hash ^ token;
}

/**
 * A fixed table of Zobrist tokens.
 *
 * The seed constructor consumes a private `Rng(seed)`, so a table is stable
 * across runs and independent from every other table. The Rng constructor
 * intentionally consumes the caller's stream, which is useful when one
 * reproducible random sequence owns several tables.
 */
class Zobrist {
 public:
  using token_type = ZobristToken;

  Zobrist(std::size_t count, std::uint64_t seed) : tokens_(count) {
    Rng rng(seed);
    fill(rng);
  }

  Zobrist(std::size_t count, Rng& rng) : tokens_(count) { fill(rng); }

  [[nodiscard]] std::size_t size() const noexcept { return tokens_.size(); }

  /** Fast access. index must be less than size(). */
  [[nodiscard]] token_type operator[](std::size_t index) const noexcept {
    return tokens_[index];
  }

  /** Checked access; throws std::out_of_range when index is outside the table. */
  [[nodiscard]] token_type at(std::size_t index) const {
    return tokens_.at(index);
  }

  /** Checked token lookup, suitable for an API boundary. */
  [[nodiscard]] token_type token(std::size_t index) const { return at(index); }

  /** XOR the token at index into hash. */
  [[nodiscard]] std::uint64_t update(std::uint64_t hash,
                                      std::size_t index) const {
    return zobrist_update(hash, at(index));
  }

  /** In-place form of update(). */
  void apply(std::uint64_t& hash, std::size_t index) const {
    hash = update(hash, index);
  }

  /** The token-level form is useful without constructing a table. */
  [[nodiscard]] static constexpr std::uint64_t update_token(
      std::uint64_t hash, token_type token_value) noexcept {
    return zobrist_update(hash, token_value);
  }

 private:
  void fill(Rng& rng) noexcept {
    for (auto& token_value : tokens_) {
      token_value = next_zobrist_token(rng);
    }
  }

  std::vector<token_type> tokens_;
};

using ZobristTable = Zobrist;

}  // namespace ahc

#endif  // AHC_CORE_ZOBRIST_HPP
