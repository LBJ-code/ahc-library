#include "../rng.hpp"
#include "../zobrist.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>

int main() {
  constexpr std::uint64_t seed = 20260829;
  ahc::ZobristTable table(16, seed);
  ahc::ZobristTable same_table(16, seed);
  ahc::Rng rng(seed);
  for (std::size_t i = 0; i < table.size(); ++i) {
    assert(table[i] == same_table.token(i));
    assert(table[i] == rng());
  }

  std::uint64_t hash = 0;
  table.apply(hash, 3);
  const std::uint64_t after_one = hash;
  assert(after_one == table.token(3));
  table.apply(hash, 3);
  assert(hash == 0);
  hash = ahc::zobrist_update(hash, table.token(4));
  assert(hash == ahc::Zobrist::update_token(0, table[4]));

  ahc::Rng stream(7);
  ahc::Rng expected(7);
  ahc::Zobrist from_stream(2, stream);
  assert(from_stream[0] == expected());
  assert(from_stream[1] == expected());
  assert(from_stream.size() == 2);

  bool threw = false;
  try {
    (void)table.at(table.size());
  } catch (const std::out_of_range&) {
    threw = true;
  }
  assert(threw);
  return 0;
}
