# Engine解説HTMLを作るスクリプト。
#   tools/src/*.txt  : フォルダごとの解説の元データ(簡単な書式。書式は tools/README.txt)
#   tools/src/*.html : 手書きのページ本文(index / architecture / glossary)
# 実行すると docs/EngineGuide/*.html と search-index.js を作り直す。
#   powershell -NoProfile -ExecutionPolicy Bypass -File docs/EngineGuide/tools/build.ps1
# コード例は「ファイル:行範囲」から実際のソースを読み込んで載せる(書き間違いが起きないように)。
# 表の名前がヘッダーに実在するか、Engineの全ファイルが解説に載っているかも検査する。

$ErrorActionPreference = 'Stop'
$here    = Split-Path -Parent $MyInvocation.MyCommand.Path
$outDir  = Split-Path -Parent $here
$repo    = Split-Path -Parent (Split-Path -Parent $outDir)
$srcRoot = Join-Path $repo '3D_FreamWork'            # Engine / Sample2D / Sample3D / Demo の親
$srcDir  = Join-Path $here 'src'
$utf8bom = New-Object System.Text.UTF8Encoding($true)
$warnings = New-Object System.Collections.ArrayList

function Warn($m) { [void]$warnings.Add($m); Write-Host "WARN: $m" -ForegroundColor Yellow }
function Esc($s) { return [System.Net.WebUtility]::HtmlEncode($s) }

# 本文の中の `code` / **bold** / [text](url) / [[Name]] を HTML にする
$script:fileIds = @{}   # Name(拡張子なし) -> 'Page.html#f-Name'
function Inline($s) {
    $t = Esc $s
    $t = [regex]::Replace($t, '`([^`]+)`', '<code>$1</code>')
    $t = [regex]::Replace($t, '\*\*([^*]+)\*\*', '<strong>$1</strong>')
    $t = [regex]::Replace($t, '\[([^\]]+)\]\(([^)]+)\)', '<a href="$2">$1</a>')
    $t = [regex]::Replace($t, '\[\[([A-Za-z0-9_]+)\]\]', {
        param($m)
        $n = $m.Groups[1].Value
        if ($script:fileIds.ContainsKey($n)) { return '<a href="' + $script:fileIds[$n] + '"><code>' + $n + '</code></a>' }
        return '<code>' + $n + '</code>'
    })
    return $t
}

# ─── 共通の枠(ナビ付き) ────────────────────────────
$navItems = @(
    @('index.html', 'トップ'), @('architecture.html', '設計図(図解)'),
    @('Core.html', 'Core'), @('Render.html', 'Render'), @('Render3D.html', 'Render3D'),
    @('Physics.html', 'Physics'), @('UI.html', 'UI'), @('Animation.html', 'Animation'),
    @('Transition.html', 'Transition'), @('Behavior.html', 'Behavior'), @('Audio.html', 'Audio'),
    @('Data.html', 'Data'), @('ThirdParty.html', 'ThirdParty'), @('files.html', 'ファイル索引'), @('glossary.html', '用語集')
)
function Wrap($fileName, $title, $body) {
    $nav = New-Object System.Text.StringBuilder
    foreach ($n in $navItems) {
        $cls = ''
        if ($n[0] -eq $fileName) { $cls = ' class="current"' }
        [void]$nav.AppendLine("<li><a href=`"$($n[0])`"$cls>$($n[1])</a></li>")
    }
    $html = @"
<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>$(Esc $title) - Engine解説</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<div class="layout">
<nav class="side">
<div class="brand"><a href="index.html">Engine解説</a></div>
<div class="search"><input type="search" id="q" placeholder="関数名・クラス名・ファイル名を検索" autocomplete="off"><ul id="results"></ul></div>
<ul class="menu">
$($nav.ToString())</ul>
<p class="sidenote">3D_FreamWork の Engine を、初心者向けに解説したページです。</p>
</nav>
<main>
$body
<footer class="foot">この解説は <code>docs/EngineGuide/tools/build.ps1</code> で作っています。表の内容は Engine のヘッダーから拾っています。コード例は Sample2D / Sample3D / Demo の実際のソースを読み込んで載せています。</footer>
</main>
</div>
<script src="search-index.js"></script>
<script src="search.js"></script>
</body>
</html>
"@
    return $html
}

function SaveHtml($name, $text) {
    $p = Join-Path $outDir $name
    $t = ($text -replace "`r`n", "`n") -replace "`n", "`r`n"
    [System.IO.File]::WriteAllText($p, $t, $utf8bom)
}

# ─── コード例を実際のソースから切り出す ────────────────────
function ExtractCode($spec) {
    # spec: "Sample2D/Game/X.cpp:12-20,30-33"
    $idx = $spec.LastIndexOf(':')
    $rel = $spec.Substring(0, $idx).Trim()
    $ranges = $spec.Substring($idx + 1).Trim()
    $full = Join-Path $srcRoot $rel
    if (-not (Test-Path $full)) { Warn "コード例の元ファイルが無い: $rel"; return $null }
    $lines = Get-Content $full -Encoding UTF8
    $picked = New-Object System.Collections.ArrayList
    $first = $true
    foreach ($r in ($ranges -split ',')) {
        $a, $b = $r.Trim() -split '-'
        if (-not $b) { $b = $a }
        $a = [int]$a; $b = [int]$b
        if ($a -lt 1 -or $b -gt $lines.Count -or $a -gt $b) { Warn "行範囲がおかしい: $spec"; return $null }
        if (-not $first) { [void]$picked.Add('@@GAP@@') }
        $first = $false
        for ($i = $a; $i -le $b; $i++) { [void]$picked.Add($lines[$i - 1]) }
    }
    # 共通のインデントを外す
    $minIndent = 999
    foreach ($l in $picked) {
        if ($l -eq '@@GAP@@' -or $l.Trim() -eq '') { continue }
        $n = $l.Length - $l.TrimStart().Length
        if ($n -lt $minIndent) { $minIndent = $n }
    }
    if ($minIndent -eq 999) { $minIndent = 0 }
    $out = New-Object System.Collections.ArrayList
    foreach ($l in $picked) {
        if ($l -eq '@@GAP@@') { [void]$out.Add('// (中略)'); continue }
        if ($l.Length -ge $minIndent) { [void]$out.Add($l.Substring($minIndent)) } else { [void]$out.Add($l.TrimStart()) }
    }
    return ,@($rel, ($out -join "`n"), $ranges)
}

# ─── 元データ(.txt)の読み込み ────────────────────────────
$engineRoot = Join-Path $srcRoot 'Engine'
$allEngineFiles = Get-ChildItem $engineRoot -Recurse -File | Where-Object { $_.Extension -in '.h', '.cpp' } |
    ForEach-Object { 'Engine/' + $_.FullName.Substring($engineRoot.Length + 1).Replace('\', '/') }
$covered = @{}
$pages = @()
$searchIndex = New-Object System.Collections.ArrayList
$fileRows = New-Object System.Collections.ArrayList

function ParseSpec($path) {
    $page = @{ name = ''; title = ''; lead = ''; blocks = New-Object System.Collections.ArrayList }
    $cur = $null; $curDir = $null; $buf = New-Object System.Collections.ArrayList
    $flush = {
        if ($null -eq $script:curDir) { return }
        $text = ($script:buf -join "`n").Trim("`n", ' ')
        switch ($script:curDir) {
            'lead'  { $page.lead = $text }
            'p'     { [void]$page.blocks.Add(@{ kind = 'p'; text = $text }) }
            'h2'    { [void]$page.blocks.Add(@{ kind = 'h2'; text = $text }) }
            'role'  { $script:cur.role = $text }
            'class' { $script:cur.classes = $text }
            'demo'  { $script:cur.demo = $text }
            'readme' { $script:cur.readme = $text }
            'notes' { $script:cur.notes = $text }
            'table' { [void]$script:cur.items.Add(@{ kind = 'table'; cap = $script:curArg; text = $text }) }
            'code'  { [void]$script:cur.items.Add(@{ kind = 'code'; arg = $script:curArg }) }
            'lcode' { [void]$script:cur.items.Add(@{ kind = 'lcode'; arg = $script:curArg; text = $text }) }
        }
        $script:buf = New-Object System.Collections.ArrayList
        $script:curDir = $null
    }
    $script:cur = $null; $script:curDir = $null; $script:buf = $buf; $script:curArg = ''
    foreach ($line in (Get-Content $path -Encoding UTF8)) {
        if ($line -match '^=(\w+)\s*(.*)$') {
            & $flush
            $d = $Matches[1]; $arg = $Matches[2]
            switch ($d) {
                'page'  { $page.name = $arg.Trim() }
                'title' { $page.title = $arg.Trim() }
                'file'  {
                    $script:cur = @{ kind = 'file'; spec = $arg.Trim(); role = ''; classes = ''; demo = ''; readme = ''; notes = ''; items = New-Object System.Collections.ArrayList }
                    [void]$page.blocks.Add($script:cur)
                }
                'end'   { }
                default {
                    $script:curDir = $d; $script:curArg = $arg.Trim()
                    if ($d -eq 'code') { }  # 1行ディレクティブ。本文なし
                }
            }
            if ($d -eq 'code') { & $flush }
        } else {
            if ($null -ne $script:curDir) { [void]$script:buf.Add($line) }
        }
    }
    & $flush
    return $page
}

# ─── 1回目: 全ページを読んで、リンク先の表を作る ────────────────
$specPages = @()
foreach ($f in (Get-ChildItem $srcDir -Filter *.txt | Sort-Object Name)) {
    $p = ParseSpec $f.FullName
    $specPages += $p
    foreach ($b in $p.blocks) {
        if ($b.kind -ne 'file') { continue }
        $paths = $b.spec -split '\s*,\s*'
        $b.paths = @()
        $dir = $paths[0].Substring(0, $paths[0].LastIndexOf('/'))
        $b.paths += $paths[0]
        for ($i = 1; $i -lt $paths.Count; $i++) {
            if ($paths[$i] -match '/') { $b.paths += $paths[$i] } else { $b.paths += ($dir + '/' + $paths[$i]) }
        }
        $b.id = [System.IO.Path]::GetFileNameWithoutExtension($paths[0])
        $script:fileIds[$b.id] = $p.name + '.html#f-' + $b.id
    }
}

# ─── 2回目: HTMLを作る ────────────────────────────
$tableRows = 0; $codeCount = 0; $fileCount = 0
$script:handCodeCount = 0; $script:handSvgCount = 0
foreach ($p in $specPages) {
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine("<h1>$(Esc $p.title)</h1>")
    [void]$sb.AppendLine("<div class=`"lead`">$(Inline $p.lead)</div>")
    # 目次
    $files = @($p.blocks | Where-Object { $_.kind -eq 'file' })
    [void]$sb.AppendLine('<div class="toc"><strong>このページのファイル</strong><ul>')
    foreach ($b in $files) {
        $short = ($b.paths | ForEach-Object { $_.Substring($_.LastIndexOf('/') + 1) }) -join ' / '
        [void]$sb.AppendLine("<li><a href=`"#f-$($b.id)`">$(Esc $short)</a></li>")
    }
    [void]$sb.AppendLine('</ul></div>')

    foreach ($b in $p.blocks) {
        if ($b.kind -eq 'h2') { [void]$sb.AppendLine("<h2>$(Inline $b.text)</h2>"); continue }
        if ($b.kind -eq 'p')  { [void]$sb.AppendLine("<p>$(Inline $b.text)</p>"); continue }
        # ファイルの節
        $fileCount++
        $headerText = ($b.paths | ForEach-Object { "<code>$(Esc $_)</code>" }) -join ' '
        [void]$sb.AppendLine("<section class=`"file`" id=`"f-$($b.id)`">")
        [void]$sb.AppendLine("<h3>$headerText</h3>")
        foreach ($pa in $b.paths) {
            $covered[$pa] = $true
            if ($allEngineFiles -notcontains $pa) { Warn "実在しないEngineファイル: $pa ($($p.name))" }
        }
        $roleHtml = Inline $b.role
        [void]$sb.AppendLine("<p class=`"role`">$roleHtml</p>")
        $firstRole = ($b.role -split "`n")[0]
        $classNames = @()
        if ($b.classes) {
            $classNames = $b.classes -split '\s*,\s*' | Where-Object { $_ }
            [void]$sb.AppendLine("<p class=`"classes`">クラス / 型: " + (($classNames | ForEach-Object { "<code>$(Esc $_)</code>" }) -join ' ') + '</p>')
        }
        [void]$searchIndex.Add(@{ t = 'ファイル'; n = ($b.paths -join ' '); d = $firstRole; u = "$($p.name).html#f-$($b.id)" })
        foreach ($cn in $classNames) { [void]$searchIndex.Add(@{ t = 'クラス'; n = $cn; d = $firstRole; u = "$($p.name).html#f-$($b.id)" }) }
        foreach ($pa in $b.paths) {
            [void]$fileRows.Add(@{ folder = $pa.Split('/')[1]; path = $pa; role = $firstRole; url = "$($p.name).html#f-$($b.id)" })
        }

        # ヘッダーの全文(名前の実在検査用)
        $hdrText = ''
        foreach ($pa in $b.paths) {
            $full = Join-Path $srcRoot $pa
            if (Test-Path $full) { $hdrText += (Get-Content $full -Raw -Encoding UTF8) + "`n" }
        }

        foreach ($it in $b.items) {
            if ($it.kind -eq 'table') {
                if ($it.cap) { [void]$sb.AppendLine("<h4>$(Inline $it.cap)</h4>") }
                [void]$sb.AppendLine('<div class="tw"><table class="api"><thead><tr><th>名前</th><th>引数・戻り値</th><th>何をするか</th></tr></thead><tbody>')
                foreach ($row in ($it.text -split "`n")) {
                    if ($row.Trim() -eq '') { continue }
                    $cells = $row -split '\s\|\s', 3
                    if ($cells.Count -lt 3) { Warn "表の行の区切りが足りない: $row ($($b.id))"; continue }
                    $name = $cells[0].Trim()
                    $m = [regex]::Match($name, '[A-Za-z_][A-Za-z0-9_]*')
                    $ident = $m.Value
                    if ($ident -and $hdrText -notmatch [regex]::Escape($ident)) { Warn "表の名前がヘッダーに無い: $ident ($($b.id))" }
                    $tableRows++
                    [void]$sb.AppendLine("<tr><td class=`"fn`"><code>$(Esc $name)</code></td><td class=`"sig`"><code>$(Esc $cells[1].Trim())</code></td><td>$(Inline $cells[2].Trim())</td></tr>")
                    if ($ident) { [void]$searchIndex.Add(@{ t = '関数/メンバ'; n = $name; d = $cells[2].Trim(); u = "$($p.name).html#f-$($b.id)"; f = ($b.paths[0].Substring($b.paths[0].LastIndexOf('/') + 1)) }) }
                }
                [void]$sb.AppendLine('</tbody></table></div>')
            }
            elseif ($it.kind -eq 'code') {
                $r = ExtractCode $it.arg
                if ($r) {
                    $codeCount++
                    [void]$sb.AppendLine("<div class=`"ex`"><div class=`"exhead`">使い方の例 <span class=`"src`">出典: <code>$(Esc $r[0]):$(Esc $r[2])</code></span></div><pre><code>$(Esc $r[1])</code></pre></div>")
                }
            }
            elseif ($it.kind -eq 'lcode') {
                # 解説用に手で書いたコード(出典なし。短い断片だけ)
                [void]$sb.AppendLine("<div class=`"ex`"><div class=`"exhead`">$(Inline $it.arg)</div><pre><code>$(Esc $it.text)</code></pre></div>")
            }
        }
        if ($b.notes) {
            [void]$sb.AppendLine('<div class="notes"><strong>気を付ける点</strong><ul>')
            foreach ($n in ($b.notes -split "`n")) {
                if ($n -match '^\s*-\s*(.*)$') { [void]$sb.AppendLine("<li>$(Inline $Matches[1])</li>") }
            }
            [void]$sb.AppendLine('</ul></div>')
        }
        if ($b.demo -or $b.readme) {
            [void]$sb.AppendLine('<p class="refs">')
            if ($b.demo)   { [void]$sb.AppendLine("<span class=`"ref`"><strong>Demo で見られる所</strong>: $(Inline $b.demo)</span>") }
            if ($b.readme) { [void]$sb.AppendLine("<span class=`"ref`"><strong>README</strong>: <a href=`"../../README.md`">$(Esc $b.readme)</a></span>") }
            [void]$sb.AppendLine('</p>')
        }
        [void]$sb.AppendLine('</section>')
    }
    SaveHtml ($p.name + '.html') (Wrap ($p.name + '.html') $p.title $sb.ToString())
}

# ─── 手書きページ(src/*.html) ────────────────────────────
foreach ($f in (Get-ChildItem $srcDir -Filter *.html)) {
    $raw = Get-Content $f.FullName -Raw -Encoding UTF8
    $title = $f.BaseName
    if ($raw -match '^<!--title:\s*(.*?)\s*-->') { $title = $Matches[1] }
    # [[Name]] と {{rows}} だけ置き換える
    $raw = [regex]::Replace($raw, '\[\[([A-Za-z0-9_]+)\]\]', {
        param($m)
        $n = $m.Groups[1].Value
        if ($script:fileIds.ContainsKey($n)) { return '<a href="' + $script:fileIds[$n] + '"><code>' + $n + '</code></a>' }
        return '<code>' + $n + '</code>'
    })
    # <!--code:ファイル:行範囲--> は実際のソースを読み込んだコード例に、<!--include:名前--> は tools/svg/名前.svg の図に置き換える
    $raw = [regex]::Replace($raw, '<!--code:([^>]+?)-->', {
        param($m)
        $r = ExtractCode $m.Groups[1].Value.Trim()
        if (-not $r) { return '<p>(コード例を読み込めませんでした)</p>' }
        $script:handCodeCount++
        return "<div class=`"ex`"><div class=`"exhead`">コード例 <span class=`"src`">出典: <code>$(Esc $r[0]):$(Esc $r[2])</code></span></div><pre><code>$(Esc $r[1])</code></pre></div>"
    })
    $raw = [regex]::Replace($raw, '<!--include:([^>]+?)-->', {
        param($m)
        $svgPath = Join-Path $here ('svg\' + $m.Groups[1].Value.Trim() + '.svg')
        if (-not (Test-Path $svgPath)) { Warn "図が無い: $svgPath"; return '' }
        $script:handSvgCount++
        return (Get-Content $svgPath -Raw -Encoding UTF8)
    })
    SaveHtml ($f.BaseName + '.html') (Wrap ($f.BaseName + '.html') $title $raw)
}

# ─── ファイル索引(files.html) ────────────────────────────
$fb = New-Object System.Text.StringBuilder
[void]$fb.AppendLine('<h1>ファイル索引</h1>')
[void]$fb.AppendLine("<div class=`"lead`">Engine の全ファイル(<code>.h</code> / <code>.cpp</code>)の一覧です。<strong>$($allEngineFiles.Count) ファイル</strong>。文字を入力すると絞り込めます。名前をクリックすると、そのファイルの解説に飛びます。</div>")
[void]$fb.AppendLine('<p><input type="search" id="filter" class="filter" placeholder="絞り込み(例: Scene, Physics, Raycast)" autocomplete="off"></p>')
[void]$fb.AppendLine('<div class="tw"><table class="api" id="filetable"><thead><tr><th>フォルダ</th><th>ファイル</th><th>役割</th></tr></thead><tbody>')
$order = 'Core', 'Render', 'Render3D', 'Physics', 'UI', 'Animation', 'Transition', 'Behavior', 'Audio', 'Data', 'ThirdParty'
foreach ($folder in $order) {
    foreach ($row in ($fileRows | Where-Object { $_.folder -eq $folder } | Sort-Object { $_.path })) {
        $nm = $row.path.Substring($row.path.LastIndexOf('/') + 1)
        [void]$fb.AppendLine("<tr><td>$folder</td><td class=`"fn`"><a href=`"$($row.url)`"><code>$(Esc $nm)</code></a></td><td>$(Inline $row.role)</td></tr>")
    }
}
[void]$fb.AppendLine('</tbody></table></div>')
[void]$fb.AppendLine('<script>(function(){var f=document.getElementById("filter");if(!f)return;var rows=document.querySelectorAll("#filetable tbody tr");f.addEventListener("input",function(){var q=f.value.toLowerCase();for(var i=0;i<rows.length;i++){rows[i].style.display=rows[i].textContent.toLowerCase().indexOf(q)>=0?"":"none";}});})();</script>')
SaveHtml 'files.html' (Wrap 'files.html' 'ファイル索引' $fb.ToString())

# ─── 検索の索引 ────────────────────────────────
$js = New-Object System.Text.StringBuilder
[void]$js.AppendLine('// build.ps1 が作る検索の索引(手で直さない)')
[void]$js.AppendLine('window.SEARCH_INDEX = [')
foreach ($e in $searchIndex) {
    $ff = ''
    if ($e.f) { $ff = $e.f }
    $o = '{"t":' + (ConvertTo-Json $e.t -Compress) + ',"n":' + (ConvertTo-Json $e.n -Compress) + ',"d":' + (ConvertTo-Json $e.d -Compress) + ',"u":' + (ConvertTo-Json $e.u -Compress) + ',"f":' + (ConvertTo-Json $ff -Compress) + '},'
    [void]$js.AppendLine($o)
}
[void]$js.AppendLine('];')
SaveHtml 'search-index.js' $js.ToString()

# ─── 検査 ────────────────────────────────────
foreach ($f in $allEngineFiles) {
    if (-not $covered.ContainsKey($f)) { Warn "解説に載っていないEngineファイル: $f" }
}
Write-Host ("解説ページ: {0} / ファイル節: {1} / 表の行: {2} / コード例: {3}(手書きページ内: {4}) / 図: {5} / 警告: {6}" -f $specPages.Count, $fileCount, $tableRows, ($codeCount + $script:handCodeCount), $script:handCodeCount, $script:handSvgCount, $warnings.Count)
