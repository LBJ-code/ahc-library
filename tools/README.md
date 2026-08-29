# tools

提出前に、ローカルの`#include "..."`を1つのC++ファイルへまとめるツールです。

## amalgamate.py

```sh
python3 tools/amalgamate.py main.cpp -o submission.cpp
python3 tools/amalgamate.py main.cpp -I include --no-provenance > submission.cpp
```

ファイルの場所を基準にquoted includeを再帰展開します。`<bits/stdc++.h>`などのsystem includeはそのままです。同じファイルは一度だけ出力し、循環includeはエラーにします。展開結果には、検出したGitHub URLと可能ならgit commitをコメントで残します。

`#pragma once`は展開時に省略します。提出用のmain fileでGCC警告にならないためです。

`-I/--include-dir`は追加の検索先です。`--no-provenance`を付けると生成コメントを省けます。入力はUTF-8です。

図解は[ツール一覧](index.html)と[amalgamate解説](amalgamate.html)を参照してください。部品は[utilities](../utilities/index.html)にあります。
