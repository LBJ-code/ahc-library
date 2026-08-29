# ahc-library

AtCoder Heuristic Contest (AHC) で使用する、再利用可能なライブラリを管理するためのリポジトリです。

まず全体を眺めるなら、[AHC Library Handbook](handbook.html) を開いてください。各部品の「何ができる・いつ使う・主なAPI」を短くまとめています。

## Libraries

- [差分更新ビームサーチ](beam-search/README.md)
  - [図解ドキュメント](beam-search/index.html)
  - 通常版、複数ターン版、Euler Tour 辺配列版を収録しています。
- [基盤部品](core/README.md) — Timer、明示seed RNG、Zobrist hash
  - [図解ドキュメント](core/index.html)
- [局所探索](local-search/README.md) — 焼きなまし、集計、rollback、最良解保持
  - [図解ドキュメント](local-search/index.html)
- [汎用部品](utilities/README.md) — grid/BFS、順列近傍、重み付き抽選
  - [図解ドキュメント](utilities/index.html)
- [提出ツール](tools/README.md) — ローカルincludeを単一ファイルへ展開
  - [図解ドキュメント](tools/index.html)

## Design research

過去の短期AHCで繰り返し使われた構造と、採用・後回しの理由は
[再利用部品調査](docs/reusable-patterns-research.md) に記録しています。調査時点で開催前の
AHC070は対象に含めていません。

## Test

```sh
make test
make sanitize
```

## AI-generated code

このリポジトリには、生成AIを利用して作成・修正したコードが含まれる場合があります。短期AHCでそのようなコードを使用する際は、AtCoderの最新ルールを確認し、提出コードの該当箇所にコンテスト開始前の公開URLを記載します。

## License

ライセンスは未定です。第三者による利用・再配布については、ライセンスが追加されるまで許諾されません。

第三者から許諾を受けて収録したコードには、個別の `NOTICE.md` と利用条件が適用されます。
