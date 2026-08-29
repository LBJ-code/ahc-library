# 局所探索（simulated annealing）の小さな部品

状態・近傍・タイマーを持たない C++23 の header-only 部品です。問題側が
`delta = 候補 score - 現在 score` を計算し、受理できたときだけ状態を変更します。

## 最小例

```cpp
#include "local-search/annealing.hpp"
#include "local-search/search_stats.hpp"

std::mt19937_64 rng(123456);                 // 呼出側が所有する乱数
double score = evaluate(state);              // minimize の例
ahc::SearchStats<double> stats(ahc::Objective::minimize);
stats.seed(score);                           // 初期値は試行数に含めない
ahc::LinearTemperature temperature(20.0, 0.01);

while (!timer.expired()) {                   // Timer は問題側
    const double p = timer.progress();              // core::Timer ならこれでよい
    const auto move = make_neighbor(state, rng); // 近傍も問題側
    const double candidate_score = evaluate_after(state, move);
    const double delta = candidate_score - score;
    const bool accepted = ahc::accept(
        ahc::Objective::minimize, delta, temperature(p), rng);
    if (accepted) {
        apply(state, move);
        score = candidate_score;
    }
    stats.observe(candidate_score, accepted);
}
```

最大化なら `Objective::maximize` と同じ符号の `delta` を使います。改善・同値は
常に受理され、悪化だけが温度に応じて確率的に受理されます。

## ファイル

| ファイル | 役割 |
| --- | --- |
| [`annealing.hpp`](annealing.html) | 進捗率、線形/指数温度、数値安全な受理判定 |
| [`search_stats.hpp`](search_stats.html) | 試行数・受理数・best・受理率 |
| [`rollback_array.hpp`](rollback_array.html) | `snapshot` / `rollback` / `commit` 付き配列 |
| [`best_solution.hpp`](best_solution.html) | Objective に応じた最良の State + score |

### 受理判定

`accept(objective, delta, temperature, rng)` の `delta` は必ず
`candidate_score - current_score` です。`rng` は `std::mt19937` などを参照で渡します。
乱数の global 状態は作りません。温度が 0 以下・NaN のときは貪欲判定となり、
大きな悪化量の確率比較は `exp` ではなく対数空間で行います。

### Stats

`seed(initial_score)` の後に `observe(candidate_score, accepted)` を1試行ごとに
呼びます。`accepted == true` の候補だけを best の比較対象とします。初期値を
seed しない場合、最初に受理した値が best になり `improved()` が1増えます。
試行0回の `acceptance_rate()` は `0.0` です。best が未登録の `best()` は
`std::logic_error`、NaN の seed は `std::invalid_argument` になります。

### RollbackArray

```cpp
ahc::RollbackArray<int> a{1, 2, 3};
auto mark = a.snapshot();
a[1] = 99;
if (keep_change) {
    a.commit(mark);
} else {
    a.rollback(mark);
}
```

checkpoint は入れ子にできます。非 const の `operator[]`、`at`、`set`、`fill`、
`data`、走査用 `begin`/`end` を通る変更が追跡対象です。checkpoint 中の同じ要素は
各 checkpoint につき一度だけ差分を保存します。生の `T&` を checkpoint 前に保持し、
その参照で後から書き換える操作は追跡できないため避けてください。

### BestSolution

`BestSolution<State, Score>` は状態型を知らずに最良の一組だけを保持します。
`consider(state, score)` が更新時だけ `true` を返します。同値は最初の状態を残す
strict 比較です。空の `state()` / `score()` は `std::logic_error` になります。

## よくある失敗

```cpp
// NG: maximize なのに「current - candidate」を渡す
const double delta = current - candidate;
ahc::accept(ahc::Objective::maximize, delta, temp, rng);

// OK: 常に候補 - 現在
const double delta = candidate - current;
```

```cpp
// NG: apply してから reject する
apply(state, move);
if (!ahc::accept(ahc::Objective::minimize, delta, temp, rng)) {
    // state を戻せない
}

// OK: 判定してから apply（または RollbackArray で戻す）
```

## ビルドとテスト

```sh
g++ -std=c++23 -Wall -Wextra -Wpedantic -Ilocal-search \
    local-search/tests/test_local_search.cpp -o /tmp/local-search-test
/tmp/local-search-test

g++ -std=c++23 -Wall -Wextra -Wpedantic -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Ilocal-search \
    local-search/tests/test_local_search.cpp -o /tmp/local-search-test-san
ASAN_OPTIONS=detect_leaks=0 /tmp/local-search-test-san

# core::Timer と core::Rng を組み合わせる確認
g++ -std=c++23 -Wall -Wextra -Wpedantic -I. \
    local-search/tests/core_integration_test.cpp -o /tmp/local-search-core-test
/tmp/local-search-core-test
```

AI事前作成コード公開元: <https://github.com/LBJ-code/ahc-library>
