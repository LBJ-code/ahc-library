# utilities

AHCで何度も使う、小さなC++23部品です。すべてheader-onlyで、`namespace ahc`にあります。外部ライブラリは使いません。

## 4つの部品

- [`grid.hpp`](grid.html): `Point`、盤面内判定、4/8近傍、距離
- [`grid_bfs.hpp`](grid_bfs.html): 4近傍の単一/複数始点BFS、距離、親、経路復元
- [`permutation_ops.hpp`](permutation_ops.html): swap、区間reverse、relocateと逆操作
- [`weighted_sampler.hpp`](weighted_sampler.html): 非負重みを確率として選ぶFenwick sampler

図解は[一覧ページ](index.html)にあります。提出用の結合は[`tools/amalgamate.py`](../tools/amalgamate.html)で行えます。

## 最小例

```cpp
#include "utilities/grid.hpp"
#include "utilities/grid_bfs.hpp"
#include "utilities/permutation_ops.hpp"
#include "utilities/weighted_sampler.hpp"

ahc::Point p{2, 3};
if (ahc::inside(p, 10, 10)) {
    for (ahc::Point q : ahc::neighbors4(p, 10, 10)) {
        // q を使う
    }
}
auto path_result = ahc::bfs_grid(
    10, 10,
    [](ahc::Point) { return true; },
    p
);
auto path = path_result.path_to(ahc::Point{8, 9});

std::vector<int> order{0, 1, 2, 3};
auto move = ahc::RelocateOperation{1, 3}; // 移動後の添字
move.apply(order);
move.undo(order);

ahc::WeightedSampler<double> sampler({1.0, 0.0, 3.0});
auto selected = sampler.sample(); // 空なら std::nullopt
```

## 共通の注意

- `Point` の添字は `row, col`（`r, c` と `x, y` も同じ順）です。
- 添字は0始まりです。範囲外の変更操作は`false`を返し、列を壊しません。
- 重みは有限の非負値だけです。総和が0なら抽選結果は`std::nullopt`です。
- `sample(rng)`では標準URBG、`sample_with([&]{ ... })`では`[0,1]`の乱数を注入できます。
