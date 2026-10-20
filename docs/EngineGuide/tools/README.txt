Engine解説(docs/EngineGuide)の作り方
=====================================

見るだけなら、docs/EngineGuide/index.html をダブルクリックします(インターネット不要)。

ファイルの役割
  *.html / style.css / search.js / search-index.js ... できあがったページ(tools/build.ps1 が作る。手で直さない)
  tools/src/*.txt ........ フォルダごとのページの元データ(下の書式)
  tools/src/*.html ....... 手書きのページ(index / architecture / glossary)
  tools/make-diagrams.ps1  図(SVG)を作る。tools/svg/*.svg に出力
  tools/build.ps1 ........ ページと検索の索引を作る(コード例は実際のソースから読み込む。名前がヘッダーに無い・Engineのファイルが載っていない時は警告)
  tools/check.ps1 ........ リンク切れ・タグの閉じ忘れ・BOM/CRLF の検査(-Fix で BOM/CRLF を直す)

更新の手順
  1. tools/src/*.txt(か .html)を直す。Engineの関数が変わったら、その表の行を直す。
  2. powershell -NoProfile -ExecutionPolicy Bypass -File docs/EngineGuide/tools/make-diagrams.ps1   (図を変えた時だけ)
  3. powershell -NoProfile -ExecutionPolicy Bypass -File docs/EngineGuide/tools/build.ps1
  4. powershell -NoProfile -ExecutionPolicy Bypass -File docs/EngineGuide/tools/check.ps1 -Fix

元データ(.txt)の書式(行の先頭の = が合図)
  =page 名前            ページ名(出力は 名前.html)
  =title 見出し         ページの見出し
  =lead                 導入の文章(次の = の行まで)
  =p                    段落
  =file パス, 追加のファイル名   ファイルの節(例: Engine/Core/Scene.h, Scene.cpp)
  =class 名前, 名前     クラス名(検索用)
  =role                 そのファイルの役割(1行目が、ファイル索引の説明になる)
  =table 見出し         関数の表。1行 = 「名前 | 引数・戻り値 | 説明」(区切りは 半角の空白で挟んだ | )
  =code パス:行範囲     実際のソースを読み込んだコード例。パスは 3D_FreamWork/ からの相対。範囲は 12-20,30-33 のように複数可(間は「(中略)」)
  =notes                気を付ける点(行頭に - )
  =demo / =readme       Demo で見られる所 / README の節番号
  文の中: `code`  **太字**  [文字](リンク)  [[ファイル名]](そのファイルの解説へのリンク)

表の名前(最初の英数字)が、その節のファイルに無いと警告が出ます。コード例の行がずれたら、build.ps1 を実行して出力を確かめ、行範囲を直します。
