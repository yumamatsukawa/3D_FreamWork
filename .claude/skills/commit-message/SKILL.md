---
name: commit-message
description: 前回のコミットからの変更を読んでコミット文を考え、README.mdも同じターンで変更に合わせて更新する。「コミット文を考えて」「コミットメッセージ作って」と言われた時に必ず使う。
---

# コミット文の作成 + README更新

このプロジェクトでは、コミット文を考える時に **README.md も必ず一緒に更新する** 決まりになっている。

## 1. 変更を把握する

gitはPATHに無いことがあるので、無ければGitHub Desktop付属のものを使う:
```powershell
$git = (Get-Command git -ErrorAction SilentlyContinue).Source
if (-not $git) { $git = Get-ChildItem "$env:LOCALAPPDATA\GitHubDesktop\app-*\resources\app\git\cmd\git.exe" | Sort-Object FullName | Select-Object -Last 1 -ExpandProperty FullName }
& $git log --oneline -5
& $git status --short
& $git diff --stat HEAD
```
- `git log` で前回のコミット文を1つ読み、書き方(見出しの付け方・粒度)をそろえる
- 変更内容は `git diff HEAD -- <ファイル>` で読む。`??`(未追跡)の新規ファイルも忘れずに読む
- 会話の中でやった作業も手がかりにするが、実際の差分と食い違う時は差分を正とする

## 2. README.md を更新する

変更に関係する節を探して直す(`Grep` で、名前を変えた/消したクラス・関数・ファイル名が README に残っていないか確認する):
- 新しいEngine機能 → 使い方の節にコード例付きで追加
- Sample2D/Sample3Dのオブジェクト・Componentの追加/移動 → 「サンプル構成」節の一覧、役割分担の表
- 設計ルールの変更 → 該当する説明

README に載せる必要のない変更(コメントの追加だけ等)しか無い時は、更新不要と判断した理由を一言報告する。

## 3. コミット文を書く

日本語で、次の形にする:
```
<1行目: 変更全体の要約(何をしたか)>

機能追加(Engine):
- <何ができるようになったか。使い方が分かる程度に具体的に>

機能追加(Sample2D):
- ...

設計の整理:
- <何をどこへ移したか、なぜか>

バグ修正:
- <どんな症状が、何が原因で、どう直したか>

- README.mdを上記の変更に合わせて更新
```
- 該当する変更が無い見出しは書かない
- 1行目は長くなりすぎないように、主な変更を2〜3個まで

## 4. 報告

- コミット文を ```text のコードブロックで示す
- README のどこを更新したかを短く
- 未追跡(`??`)のファイルがあれば「GitHub Desktopで全ファイルにチェックが入っているか確認してからコミットして」と伝える
- **コミット自体はしない**(ユーザーが頼んだ時だけ)
