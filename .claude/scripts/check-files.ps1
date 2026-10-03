# ビルド前のファイルチェック。Engine/Sample2D/Sample3D の .h/.cpp について、
#   1. 日本語を含むのにBOMが無いファイル(MSVCでC4819+構文エラーになる)
#   2. ディスクにあるのに .vcxproj / .vcxproj.filters に登録されていないファイル
#   3. .vcxproj に登録されているのにディスクに無いファイル
# を一覧にする。
# 使い方: powershell -ExecutionPolicy Bypass -File .claude/scripts/check-files.ps1 [-FixBom]
#   -FixBom を付けると、1のファイルにBOMを自動で付ける(2,3は報告だけ)
# 終了コード: 0=問題なし / 1=問題あり(-FixBomで直した分は問題に数えない)
param([switch]$FixBom)

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sol = Join-Path $root "3D_FreamWork"
$problems = 0

# vcxproj/filtersに登録されているファイルの一覧(プロジェクトフォルダからの相対パス)
function Get-ListedFiles([string]$path) {
    if (-not (Test-Path $path)) { return @() }
    [xml]$xml = Get-Content $path -Raw -Encoding UTF8
    $items = foreach ($group in $xml.Project.ItemGroup) { $group.ClCompile; $group.ClInclude }
    return @($items | Where-Object { $_ } | ForEach-Object { $_.Include })
}

foreach ($proj in "Engine", "Sample2D", "Sample3D") {
    $dir = Join-Path $sol $proj
    if (-not (Test-Path $dir)) { continue }
    $files = Get-ChildItem $dir -Recurse -Include *.cpp, *.h -File

    # ─── 1. BOM ───
    foreach ($f in $files) {
        $bytes = [IO.File]::ReadAllBytes($f.FullName)
        $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
        if ($hasBom) { continue }
        # 0x80以上のバイトがあれば、ASCII以外(日本語など)を含んでいる
        $latin1 = [Text.Encoding]::GetEncoding(28591).GetString($bytes)
        if ($latin1 -notmatch '[\x80-\xFF]') { continue }

        $rel = $f.FullName.Substring($sol.Length + 1)
        if ($FixBom) {
            [IO.File]::WriteAllBytes($f.FullName, [byte[]](0xEF, 0xBB, 0xBF) + $bytes)
            Write-Output "BOMを付けました: $rel"
        } else {
            Write-Output "BOMなし(日本語を含む): $rel"
            $problems++
        }
    }

    # ─── 2, 3. vcxproj / filters への登録 ───
    $vcx = Get-ListedFiles (Join-Path $dir "$proj.vcxproj")
    $flt = Get-ListedFiles (Join-Path $dir "$proj.vcxproj.filters")
    foreach ($f in $files) {
        $rel = $f.FullName.Substring($dir.Length + 1)
        if ($vcx -notcontains $rel) { Write-Output "vcxprojに未登録: $proj\$rel"; $problems++ }
        if ($flt -notcontains $rel) { Write-Output "filtersに未登録: $proj\$rel"; $problems++ }
    }
    foreach ($rel in $vcx) {
        if (-not (Test-Path (Join-Path $dir $rel))) { Write-Output "vcxprojにあるがファイルが無い: $proj\$rel"; $problems++ }
    }
}

if ($problems -eq 0) {
    Write-Output "ファイルチェック: 問題なし"
    exit 0
}
Write-Output "ファイルチェック: $problems 件の問題"
exit 1
