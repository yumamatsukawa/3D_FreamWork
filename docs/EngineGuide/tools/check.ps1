# 解説ページの検査: ①リンク切れ ②HTMLのタグの閉じ忘れ ③UTF-8 BOM と 改行(CRLF)
#   powershell -NoProfile -ExecutionPolicy Bypass -File docs/EngineGuide/tools/check.ps1
# -Fix を付けると、BOM と CRLF を直す(.html / .css / .js / .ps1 / .txt / .svg が対象)。

param([switch]$Fix)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $here
$problems = 0
function Problem($m) { $script:problems++; Write-Host ("NG: " + $m) -ForegroundColor Red }

# ── ③ BOM と CRLF ──
$exts = '.html', '.css', '.js', '.ps1', '.txt', '.svg'
$files = Get-ChildItem $root -Recurse -File | Where-Object { $exts -contains $_.Extension }
$utf8bom = New-Object System.Text.UTF8Encoding($true)
foreach ($f in $files) {
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    $hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
    $text = [System.Text.Encoding]::UTF8.GetString($bytes)
    if ($hasBom) { $text = $text.Substring(1) }
    $hasLf = ($text -match "(?<!\r)\n")
    if ((-not $hasBom) -or $hasLf) {
        if ($Fix) {
            $t = ($text -replace "`r`n", "`n") -replace "`n", "`r`n"
            [System.IO.File]::WriteAllText($f.FullName, $t, $utf8bom)
            Write-Host ("直しました(BOM/CRLF): " + $f.FullName.Substring($root.Length + 1))
        } else {
            Problem ("BOM か CRLF が違う: " + $f.FullName.Substring($root.Length + 1) + " (BOM=" + $hasBom + ", LFのみの行=" + $hasLf + ")")
        }
    }
}

# ── ② HTML の構造(タグの閉じ忘れ)と ① リンク ──
$void = 'meta', 'link', 'br', 'hr', 'img', 'input', 'area', 'base', 'col', 'embed', 'source', 'track', 'wbr', 'line', 'path', 'rect', 'tspan_', 'circle', 'polygon'
$htmlFiles = Get-ChildItem $root -File -Filter *.html
$ids = @{}
foreach ($f in $htmlFiles) {
    $t = Get-Content $f.FullName -Raw -Encoding UTF8
    $set = @{}
    foreach ($m in [regex]::Matches($t, '\sid="([^"]+)"')) { $set[$m.Groups[1].Value] = $true }
    $ids[$f.Name] = $set
}
$linkCount = 0
$tagIssues = 0
foreach ($f in $htmlFiles) {
    $t = Get-Content $f.FullName -Raw -Encoding UTF8
    foreach ($need in '<!DOCTYPE html>', '<html', '<head>', '<body>', '</body>', '</html>', 'charset="utf-8"') {
        if ($t.IndexOf($need, [StringComparison]::OrdinalIgnoreCase) -lt 0) { Problem ($f.Name + ": " + $need + " が無い") }
    }
    # タグの入れ子
    $stack = New-Object System.Collections.Stack
    $noComments = [regex]::Replace($t, '<!--.*?-->', '', 'Singleline')
    $noScript = [regex]::Replace($noComments, '<script\b[^>]*>.*?</script>', '<script></script>', 'Singleline')
    foreach ($m in [regex]::Matches($noScript, '<(/?)([A-Za-z][A-Za-z0-9]*)\b[^>]*?(/?)>')) {
        $close = $m.Groups[1].Value -eq '/'
        $name = $m.Groups[2].Value.ToLower()
        $selfClose = $m.Groups[3].Value -eq '/'
        if ($void -contains $name -or $selfClose) { continue }
        if (-not $close) { $stack.Push($name); continue }
        if ($stack.Count -eq 0) { Problem ($f.Name + ": 余分な閉じタグ </" + $name + ">"); $tagIssues++; continue }
        $top = $stack.Pop()
        if ($top -ne $name) {
            Problem ($f.Name + ": タグが合わない <" + $top + "> と </" + $name + ">")
            $tagIssues++
            break
        }
    }
    if ($stack.Count -gt 0 -and $tagIssues -eq 0) { Problem ($f.Name + ": 閉じていないタグ <" + $stack.Peek() + ">") }

    # リンク
    foreach ($m in [regex]::Matches($t, '(?:href|src)="([^"]+)"')) {
        $url = $m.Groups[1].Value
        if ($url -match '^(https?:|mailto:|javascript:|data:)') { Problem ($f.Name + ": 外部への参照 " + $url); continue }
        $linkCount++
        $path = $url; $frag = ''
        if ($url.Contains('#')) { $path = $url.Substring(0, $url.IndexOf('#')); $frag = $url.Substring($url.IndexOf('#') + 1) }
        if ($path -eq '') { $target = $f.Name } else { $target = $path }
        $full = [System.IO.Path]::GetFullPath((Join-Path $root $target))
        if (-not (Test-Path $full)) { Problem ($f.Name + ": リンク先が無い " + $url); continue }
        if ($frag -ne '' -and $target -like '*.html' -and $path -notlike '../*') {
            $key = [System.IO.Path]::GetFileName($target)
            if (-not $ids.ContainsKey($key) -or -not $ids[$key].ContainsKey($frag)) { Problem ($f.Name + ": ページ内の行き先が無い " + $url) }
        }
    }
}

# search-index.js の行き先
$idx = Get-Content (Join-Path $root 'search-index.js') -Raw -Encoding UTF8
$searchCount = 0
foreach ($m in [regex]::Matches($idx, '"u":"([^"]+)"')) {
    $searchCount++
    $u = $m.Groups[1].Value
    $p = $u.Substring(0, $u.IndexOf('#')); $fr = $u.Substring($u.IndexOf('#') + 1)
    if (-not (Test-Path (Join-Path $root $p))) { Problem ("search-index.js: ページが無い " + $u) }
    elseif (-not $ids[$p].ContainsKey($fr)) { Problem ("search-index.js: 行き先が無い " + $u) }
}

Write-Host ("検査したHTML: {0} / リンク: {1} / 検索の索引: {2} / 問題: {3}" -f $htmlFiles.Count, $linkCount, $searchCount, $problems)
if ($problems -gt 0) { exit 1 }
