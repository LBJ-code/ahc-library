# AHC core

AHC（AtCoder Heuristic Contest）でよく使う、依存なしのC++23ヘッダです。

| ヘッダ | 役割 |
| --- | --- |
| [`timer.hpp`](timer.hpp) | 単調時計で制限時間を測る |
| [`rng.hpp`](rng.hpp) | seedから再現可能な乱数を作る |
| [`zobrist.hpp`](zobrist.hpp) | 状態比較用の64bit XORハッシュを作る |

```cpp
#include "core/timer.hpp"
#include "core/rng.hpp"
#include "core/zobrist.hpp"

ahc::Timer timer(std::chrono::milliseconds(1900));
ahc::Rng rng(12345);                         // seedを明示
ahc::ZobristTable feature_tokens(100, 12345);
const int action = rng.uniform_int(0, 4);    // 0, 1, 2, 3
const std::uint64_t next_hash =
    feature_tokens.update(0, static_cast<std::size_t>(action));
```

## 境界

- `Timer` の制限時間が0以下なら最初から期限切れです。`remaining()` は0に張り付き、`progress()` は1.0です。
- `Rng::uniform_int(low, high)` は `[low, high)` です。`low >= high` は `std::invalid_argument` になります。
- `uniform_real` も `[low, high)`、`bernoulli(p)` の `p` は `[0, 1]` です。範囲外・非有限値は例外です。
- Zobristの同じトークンを2回 XOR すると元に戻ります。`operator[]` は高速・未検査、`at()` / `token()` は範囲検査付きです。

`Rng` は標準のURBGなので、`std::shuffle(begin, end, rng)` にそのまま渡せます。同じseed・同じ呼び出し順なら同じ値になります。並列処理ではRngをスレッドごとに分けてください。

テストは `core/tests/` にあります。リポジトリのルートで次を実行できます。

```sh
for src in core/tests/*_test.cpp; do
  g++ -std=c++23 -Wall -Wextra -Wpedantic -Icore "$src" -o /tmp/ahc-core-test
  /tmp/ahc-core-test
done
```

詳しい図と失敗例は [`index.html`](index.html) と各個別ページを開いてください。
