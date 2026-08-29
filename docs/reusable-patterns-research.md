# 過去短期 AHC の再利用部品調査

調査日: 2026-08-29。AHC070 は調査対象から明示的に除外した。短期（各4時間）の
AHC055/057/058/059/060/062/064/065/067について、AtCoderの順位・提出・公式/ユーザー解説を優先して確認した。
第三者コードはコピーせず、設計パターンだけを抽出している。

## 採用する候補

### `utilities/grid_bfs.hpp`（最優先）

- AHC060: 1位参加記はシミュレーションをBFSベースで構成し、16位のAtCoderユーザー解説は
  `(現在頂点, 直前頂点, アイス列)` の状態BFS、経路長上限、状態ハッシュを明記している。
  - [1位参加記](https://zenn.dev/sweetsweetsoul/articles/993dc8e81fc55f)
  - [16位ユーザー解説・提出リンク](https://atcoder.jp/contests/ahc060/editorial/17814)
- AHC059: 上位解説で全点間距離/最短経路を前計算してTSP的な探索に利用している。
  - [2位ユーザー解説](https://atcoder.jp/contests/ahc059/editorial/15052)
  - [3位ユーザー解説](https://atcoder.jp/contests/ahc059/editorial/15029)
- AHC067: 位置とスイッチ状態を持つ状態グラフの最短経路BFSが使われている。
  - [AHC067ユーザー解説一覧](https://atcoder.jp/contests/ahc067/editorial)

現行の `grid.hpp` は `Point`、境界判定、近傍生成までであり、距離・親・経路復元がない。
したがって、4近傍グリッドに対する単一/複数始点BFSを、`dist=-1`（未到達）と親座標で返す
header-only部品として追加する。始点が範囲外/壁なら無視し、経路列挙や visited 無しの探索は
問題依存なので含めない。

### `local-search/best_solution.hpp`（採用）

- **時間内マルチスタート/Best-of**: AHC055の初期候補試行、AHC058の貪欲/SA/Beam比較、
  AHC060のグリッドサーチ・山登り、AHC062 9位の8方向対称変換、AHC064の並列Beam、
  AHC065の配置/開始位置試行に繰り返し現れる。`Timer`/`RNG`の上に最良解保持器を置く案は
  有用である。探索ループを隠さず、`State` と score の最良の一組だけを保持する
  `BestSolution` として追加した。
  - [AHC058公式解説](https://img.atcoder.jp/ahc058/editorial.pdf)
  - [AHC062 9位参加記](https://dokukuma.hatenablog.com/entry/2026/04/02/123616)
  - [AHC064解説一覧](https://atcoder.jp/contests/ahc064/editorial)

## 頻出だが後回しにする候補

- **LNSのdestroy/repair + RRT受理**: AHC059 2位は区間を削除して再挿入し、既知最善+θを受理条件にしている。
  RRT受理器は共通化しやすいが、repairは制約・評価関数依存で、短期の直接根拠も主に同コンテストのため後回し。
  - [AHC059 2位解説](https://atcoder.jp/contests/ahc059/editorial/15052)
- **frontier/小幅DP**: AHC062公式解説・9位参加記の幅4/5の経路DPは強力だが、状態設計が問題固有で高コスト。
  - [AHC062 9位参加記](https://dokukuma.hatenablog.com/entry/2026/04/02/123616)
- **bitmask wrapper**: AHC060/062/067で在庫・スイッチ・小状態のマスクが現れるが、`std::bitset`や整数で足りる。
- **linked sequence/tree、delta evaluator**: AHC059等で有効だが、制約不変条件と評価差分が問題固有。既存 object pool/
  rollback/PermutationOps とも一部重なる。

既存の Beam Search 3種、Timer/RNG/Zobrist、Annealing/SearchStats/RollbackArray、Grid/PermutationOps/
WeightedSampler、amalgamate は調査結果と重複するため追加候補から除外した。
