# ソリューション全体(Engine/Sample2D/Sample3D)を x64 の Debug と Release でビルドする。
# 使い方: powershell -ExecutionPolicy Bypass -File .claude/scripts/build.ps1 [-Configurations Debug]
# 終了コード: 0=全て成功 / 1=どれかが失敗 / 2=MSBuildが見つからない
param([string[]]$Configurations = @("Debug", "Release"))

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)   # .claude/scripts → リポジトリのルート
$sln = Join-Path $root "3D_FreamWork\3D_FreamWork.sln"

# Visual Studioに付属するvswhereで、インストール済みのMSBuildを探す
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = $null
if (Test-Path $vswhere) {
    $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
}
if (-not $msbuild) {
    Write-Output "MSBuildが見つかりません(Visual Studio 2022のC++ワークロードが必要)"
    exit 2
}

$failed = $false
foreach ($c in $Configurations) {
    Write-Output "=== $c ==="
    $out = & $msbuild $sln /p:Configuration=$c /p:Platform=x64 /m /v:minimal /nologo
    $code = $LASTEXITCODE

    # エラー・警告・出力ファイルの行だけを表示する(並列ビルドで同じ行が重複するので1つにまとめる)
    $out | Where-Object { $_ -match "error|warning C|warning LNK|->" } | Select-Object -Unique | ForEach-Object { Write-Output $_ }

    if ($code -ne 0) {
        $failed = $true
        Write-Output "[$c] ビルド失敗 (exit $code)"
    } else {
        Write-Output "[$c] ビルド成功"
    }
}

if ($failed) { exit 1 } else { exit 0 }
