# architecture.html に入れる図(SVG)を作るスクリプト。tools/svg/*.svg に書き出す。build.ps1 が <!--include:名前--> で取り込む。
#   powershell -NoProfile -ExecutionPolicy Bypass -File docs/EngineGuide/tools/make-diagrams.ps1
# 図の内容は Engine のコード(GameEngine.cpp / Scene.cpp / SceneManager.cpp / Physics.cpp など)を読んで描いている。
# コードが変わったら、ここを直して実行し、続けて build.ps1 を実行する。

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$outSvg = Join-Path $here 'svg'
New-Item -ItemType Directory -Force $outSvg | Out-Null
$utf8bom = New-Object System.Text.UTF8Encoding($true)

function Esc($s) { return [System.Net.WebUtility]::HtmlEncode($s) }

# 文字の幅の見積もり(日本語は1文字 = フォントサイズ、英数字は約0.58倍)
function TextWidth($s, $fs) {
    $w = 0.0
    foreach ($c in $s.ToCharArray()) {
        if ([int]$c -gt 255) { $w += $fs } else { $w += $fs * 0.58 }
    }
    return $w
}

$script:parts = $null
$script:svgName = ''
$script:svgW = 0
$script:svgH = 0
$script:svgLabel = ''

function NewSvg($name, $w, $h, $label) {
    $script:svgName = $name
    $script:svgW = $w
    $script:svgH = $h
    $script:svgLabel = $label
    $script:parts = New-Object System.Collections.ArrayList
}
function Add($s) { [void]$script:parts.Add($s) }

function SaveSvg() {
    $id = 'ah-' + $script:svgName
    $head = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ' + $script:svgW + ' ' + $script:svgH + '" width="' + $script:svgW + '" role="img" aria-label="' + (Esc $script:svgLabel) + '">'
    $defs = '<defs><marker id="' + $id + '" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="ahead"/></marker></defs>'
    $body = ($script:parts -join "`n")
    $text = $head + "`n" + $defs + "`n" + $body + "`n</svg>`n"
    $text = $text.Replace('__ARROW__', 'url(#' + $id + ')')
    $p = Join-Path $outSvg ($script:svgName + '.svg')
    $text = ($text -replace "`r`n", "`n") -replace "`n", "`r`n"
    [System.IO.File]::WriteAllText($p, $text, $utf8bom)
    Write-Host ("図を作りました: {0}" -f $p)
}

# 箱。$text は `n で改行。$href があればリンクにする
function Box($x, $y, $w, $h, $text, $cls, $href, $fs) {
    if (-not $cls) { $cls = 'n' }
    if (-not $fs) { $fs = 14 }
    $lines = $text -split "`n"
    $lh = $fs + 4
    $startY = $y + ($h - $lh * $lines.Count) / 2 + $fs
    $t = New-Object System.Text.StringBuilder
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $ty = $startY + $i * $lh - 2
        [void]$t.Append('<text x="' + ($x + $w / 2) + '" y="' + $ty + '" text-anchor="middle" font-size="' + $fs + '">' + (Esc $lines[$i]) + '</text>')
    }
    $g = '<g class="box ' + $cls + '"><rect x="' + $x + '" y="' + $y + '" width="' + $w + '" height="' + $h + '" rx="6"/>' + $t.ToString() + '</g>'
    if ($href) { $g = '<a href="' + $href + '">' + $g + '</a>' }
    Add $g
}
function Line($x1, $y1, $x2, $y2, $arrow, $dashed) {
    $cls = 'ln'
    if ($dashed) { $cls = 'ln dash' }
    $m = ''
    if ($arrow) { $m = ' marker-end="__ARROW__"' }
    Add ('<line class="' + $cls + '" x1="' + $x1 + '" y1="' + $y1 + '" x2="' + $x2 + '" y2="' + $y2 + '"' + $m + '/>')
}
function PathLine($d, $arrow, $dashed) {
    $cls = 'ln'
    if ($dashed) { $cls = 'ln dash' }
    $m = ''
    if ($arrow) { $m = ' marker-end="__ARROW__"' }
    Add ('<path class="' + $cls + '" fill="none" d="' + $d + '"' + $m + '/>')
}
function Label($x, $y, $text, $anchor, $cls) {
    if (-not $anchor) { $anchor = 'middle' }
    if (-not $cls) { $cls = 'lb' }
    $lines = $text -split "`n"
    $t = New-Object System.Text.StringBuilder
    for ($i = 0; $i -lt $lines.Count; $i++) {
        [void]$t.Append('<tspan x="' + $x + '" dy="' + $(if ($i -eq 0) { 0 } else { 16 }) + '">' + (Esc $lines[$i]) + '</tspan>')
    }
    Add ('<text class="' + $cls + '" x="' + $x + '" y="' + $y + '" text-anchor="' + $anchor + '">' + $t.ToString() + '</text>')
}

# ───────────────────────────────────────────────
# 1. 継承図(Component を頂点にした木)
# ───────────────────────────────────────────────
$groups = @(
    @{ cls = 'g-render'; page = 'Render'; items = @(
        @{ name = 'SpriteRenderer'; id = 'SpriteRenderer' },
        @{ name = 'ParticleRenderer'; id = 'ParticleRenderer' },
        @{ name = 'CameraFollowComponent'; id = 'CameraFollowComponent' }) },
    @{ cls = 'g-render3d'; page = 'Render3D'; items = @(
        @{ name = 'MeshRenderer'; id = 'MeshRenderer'; caster = $true },
        @{ name = 'ModelRenderer'; id = 'ModelRenderer'; caster = $true },
        @{ name = 'TerrainRenderer'; id = 'TerrainRenderer'; caster = $true },
        @{ name = 'WaterRenderer'; id = 'WaterRenderer' },
        @{ name = 'LightComponent'; id = 'LightComponent' }) },
    @{ cls = 'g-physics'; page = 'Physics'; items = @(
        @{ name = 'RigidbodyComponent'; id = 'RigidbodyComponent'; children = @(
            'BoxRigidbodyComponent', 'SphereRigidbodyComponent', 'CircleRigidbodyComponent', 'SquareRigidbodyComponent',
            'CapsuleRigidbodyComponent', 'Capsule2DRigidbodyComponent', 'TerrainRigidbodyComponent') }) },
    @{ cls = 'g-behavior'; page = 'Behavior'; items = @(
        @{ name = 'MoveComponent'; id = 'MoveComponent'; children = @(
            'ChaseComponent', 'AccelerateChaseComponent', 'HomingComponent', 'PathFollowComponent');
            childIds = @('ChaseComponent', 'AccelerateChaseComponent', 'HomingComponent', 'PathFollowComponent') },
        @{ name = 'NavGridComponent'; id = 'NavGridComponent' },
        @{ name = 'VisionComponent'; id = 'VisionComponent' }) },
    @{ cls = 'g-animation'; page = 'Animation'; items = @(
        @{ name = 'TweenComponent'; id = 'TweenComponent' },
        @{ name = 'SpriteAnimationComponent'; id = 'SpriteAnimationComponent' },
        @{ name = 'ModelAnimationComponent'; id = 'ModelAnimationComponent' }) },
    @{ cls = 'g-audio'; page = 'Audio'; items = @(
        @{ name = 'AudioSourceComponent'; id = 'AudioSourceComponent' },
        @{ name = 'AudioListenerComponent'; id = 'AudioListenerComponent' }) },
    @{ cls = 'g-ui'; page = 'UI'; items = @(
        @{ name = 'TextRenderer'; id = 'TextRenderer' },
        @{ name = 'GaugeRenderer'; id = 'GaugeRenderer' },
        @{ name = 'SliderComponent'; id = 'SliderComponent' },
        @{ name = 'ButtonComponent'; id = 'ButtonComponent' }) }
)

# 高さを先に計算する
$rowH = 30
$boxH = 24
$total = 0
foreach ($g in $groups) { foreach ($it in $g.items) { $n = 1; if ($it.children) { $n = $it.children.Count }; $total += $n * $rowH } }
$treeTop = 20
$treeBottom = $treeTop + $total
$legendY = $treeBottom + 20
$subY = $legendY + 60
$height = $subY + 150

NewSvg 'inherit' 760 $height 'Component を頂点にした継承の木'
$rootX = 10; $rootW = 120; $rootH = 44
$rootY = $treeTop + ($total - $rootH) / 2
$trunkX = 150
$l1X = 170; $l1W = 235
$l2TrunkX = $l1X + $l1W + 22
$l2X = $l2TrunkX + 20; $l2W = 265

$centers = New-Object System.Collections.ArrayList
$y = $treeTop
foreach ($g in $groups) {
    foreach ($it in $g.items) {
        $n = 1; if ($it.children) { $n = $it.children.Count }
        $band = $n * $rowH
        $cy = $y + $band / 2
        [void]$centers.Add($cy)
        $href = $g.page + '.html#f-' + $it.id
        $itName = $it.name
        $itCls = $g.cls
        Box $l1X ($cy - $boxH / 2) $l1W $boxH $itName $itCls $href 14
        Line $trunkX $cy $l1X $cy $false $false
        if ($it.caster) { Label ($l1X + $l1W + 8) ($cy + 4) '＋ Shadow::Caster' 'start' 'lb' }
        if ($it.children) {
            $firstC = $y + $rowH / 2
            $lastC = $y + $band - $rowH / 2
            Line ($l1X + $l1W) $cy $l2TrunkX $cy $false $false
            Line $l2TrunkX $firstC $l2TrunkX $lastC $false $false
            for ($i = 0; $i -lt $n; $i++) {
                $cyy = $y + $i * $rowH + $rowH / 2
                $childId = $it.id
                if ($it.childIds) { $childId = $it.childIds[$i] }
                $childName = $it.children[$i]
                $childCls = $g.cls
                $childHref = $g.page + '.html#f-' + $childId
                Line $l2TrunkX $cyy $l2X $cyy $false $false
                Box $l2X ($cyy - $boxH / 2) $l2W $boxH $childName $childCls $childHref 14
            }
        }
        $y += $band
    }
}
# 幹
$cFirst = $centers[0]
$cLast = $centers[$centers.Count - 1]
Line $trunkX $cFirst $trunkX $cLast $false $false
Line ($rootX + $rootW) ($rootY + $rootH / 2) $trunkX ($rootY + $rootH / 2) $false $false
Box $rootX $rootY $rootW $rootH 'Component' 'root' 'Core.html#f-Component' 15

# 凡例
Label 10 $legendY 'フォルダごとの色(箱をクリックすると解説に飛びます):' 'start' 'lb'
$legend = @(@('Render', 'g-render'), @('Render3D', 'g-render3d'), @('Physics', 'g-physics'), @('Behavior', 'g-behavior'), @('Animation', 'g-animation'), @('Audio', 'g-audio'), @('UI', 'g-ui'))
$lx = 10
foreach ($lg in $legend) {
    $lgName = $lg[0]
    $lgCls = $lg[1]
    $w = [Math]::Max(70, (TextWidth $lgName 13) + 20)
    Box $lx ($legendY + 10) $w 22 $lgName $lgCls $null 13
    $lx += $w + 8
}

# 別の継承
Label 10 $subY 'Component 以外の継承(Engine のすべての「class A : public B」)' 'start' 'lb'
$sy = $subY + 20
# SceneTransition の木
Box 10 ($sy + 40) 140 28 'SceneTransition' 'g-transition' 'Transition.html#f-SceneTransition' 14
Line 150 ($sy + 54) 180 ($sy + 54) $false $false
Line 180 ($sy + 20) 180 ($sy + 88) $false $false
Line 180 ($sy + 20) 200 ($sy + 20) $false $false
Line 180 ($sy + 88) 200 ($sy + 88) $false $false
Box 200 ($sy + 6) 150 28 'FadeTransition' 'g-transition' 'Transition.html#f-FadeTransition' 14
Box 200 ($sy + 74) 150 28 'SpriteTransition' 'g-transition' 'Transition.html#f-SpriteTransition' 14
Label 10 ($sy + 125) '(Component ではありません。ChangeScene に渡す演出)' 'start' 'lb'
# Shadow::Caster
Box 400 ($sy + 40) 150 28 'Shadow::Caster' 'g-render3d dashed' 'Render3D.html#f-Shadow' 14
Line 550 ($sy + 54) 575 ($sy + 54) $false $false
Line 575 ($sy + 10) 575 ($sy + 98) $false $false
Line 575 ($sy + 10) 595 ($sy + 10) $false $false
Line 575 ($sy + 54) 595 ($sy + 54) $false $false
Line 575 ($sy + 98) 595 ($sy + 98) $false $false
Box 595 ($sy - 2) 160 24 'MeshRenderer' 'g-render3d' 'Render3D.html#f-MeshRenderer' 13
Box 595 ($sy + 42) 160 24 'ModelRenderer' 'g-render3d' 'Render3D.html#f-ModelRenderer' 13
Box 595 ($sy + 86) 160 24 'TerrainRenderer' 'g-render3d' 'Render3D.html#f-TerrainRenderer' 13
Label 400 ($sy + 125) '(影を落とせる物の印。Component と同時に継承する)' 'start' 'lb'
SaveSvg

# ───────────────────────────────────────────────
# 2. 所有関係(Scene / GameObject / Component)
# ───────────────────────────────────────────────
NewSvg 'owner' 800 470 'GameEngine、SceneManager、Scene、GameObject、Component の持ち合い'
Box 40 15 240 40 'GameEngine::Run()' 'g-core' 'Core.html#f-GameEngine' 14
Box 40 100 240 44 'SceneManager（アプリに1つ）' 'g-core' 'Core.html#f-SceneManager' 14
Box 340 100 250 44 'SceneTransition（演出中だけ持つ）' 'g-transition' 'Transition.html#f-SceneTransition' 13
Box 40 190 240 44 'Scene（今のシーン 1つ）' 'g-core' 'Core.html#f-Scene' 14
Box 40 285 240 44 'GameObject × たくさん' 'g-core' 'Core.html#f-GameObject' 14
Box 40 385 240 44 'Transform（必ず1つ）' 'g-core' 'Core.html#f-Transform' 14
Box 340 385 250 44 'Component × 付けた数だけ' 'g-core' 'Core.html#f-Component' 14
Line 160 55 160 100 $true $false
Label 170 82 'Get() で使う' 'start' 'lb'
Line 280 122 340 122 $true $false
Label 310 114 '持つ' 'middle' 'lb'
Line 160 144 160 190 $true $false
Label 170 172 '持つ（unique_ptr）' 'start' 'lb'
Line 160 234 160 285 $true $false
Label 170 264 '持つ（unique_ptr の一覧）' 'start' 'lb'
Line 160 329 160 385 $true $false
Label 170 362 '持つ（メンバ）' 'start' 'lb'
PathLine 'M 280 307 L 465 307 L 465 385' $true $false
Label 345 299 '持つ（unique_ptr の一覧）' 'start' 'lb'
# 戻りのポインタ（点線）
PathLine 'M 590 407 L 650 407 L 650 320 L 280 320' $true $true
PathLine 'M 280 296 L 330 296 L 330 205 L 280 205' $true $true
Label 338 255 'GetScene()' 'start' 'lb'
Add '<text class="lb" x="655" y="360" text-anchor="start">GetOwner()</text>'
# 凡例
Label 640 20 '実線の矢印 = 持っている' 'start' 'lb'
Label 640 38 '（一緒に消える）' 'start' 'lb'
Label 640 62 '点線の矢印 = ポインタで' 'start' 'lb'
Label 640 80 '知っているだけ' 'start' 'lb'
Label 640 98 '（持ち主ではない）' 'start' 'lb'
SaveSvg

# ───────────────────────────────────────────────
# 3. GameObject のライフサイクル
# ───────────────────────────────────────────────
NewSvg 'life' 860 380 'GameObject と Component が生まれて、動いて、消えるまで'
Box 10 15 250 56 "scene.CreateObject(名前, タグ)`nGameObject ができる" 'g-core' $null 14
Box 305 15 250 56 "obj->AddComponent<T>()`nComponent が付く" 'g-core' $null 14
Box 600 15 250 56 "Init() が1回呼ばれる`n（AddComponent の中で）" 'g-core' $null 14
Line 260 43 305 43 $true $false
Line 555 43 600 43 $true $false
Box 600 120 250 70 "毎フレーム`nUpdate(dt) → Draw()`n（付けた順。有効な時だけ）" 'g-render' $null 14
PathLine 'M 725 71 L 725 120' $true $false
Box 305 120 250 70 "obj->Destroy()`n「消す印」を付けるだけ`n（その場では消えない）" 'g-physics' $null 14
Line 600 155 555 155 $true $false
Box 10 120 250 70 "Scene::Update の最後に`nUninit() が呼ばれて`nシーンから取り除かれる" 'g-physics' $null 14
Line 305 155 260 155 $true $false
Box 600 245 250 56 "SetActive(false) = お休み`nUpdate/Draw が止まる。消えない" 'g-behavior' $null 13
PathLine 'M 790 190 L 790 245' $true $true
PathLine 'M 660 245 L 660 190' $true $true
Label 800 222 'false' 'start' 'lb'
Label 650 222 'true' 'end' 'lb'
Label 305 262 '全 Component に OnActiveChanged(bool)' 'start' 'lb'
Label 305 280 'が届く（ObjectPool の出し入れもこれ）' 'start' 'lb'
Box 10 245 250 78 "Update の途中で CreateObject すると`n待ち行列（pendingObjects）に入り、`nUpdate の最後に objects へ移る`n（動き出すのは次のフレーム）" 'g-animation' $null 13
Label 10 350 'GameObject::Update は、アクティブな時だけ、有効（IsEnabled）な Component の Update を呼びます。' 'start' 'lb'
SaveSvg

# ───────────────────────────────────────────────
# 4. ObjectPool
# ───────────────────────────────────────────────
NewSvg 'pool' 800 260 'ObjectPool の使い回しの流れ'
Box 20 100 140 50 "まだ無い" 'n' $null 15
Box 290 100 190 50 "待機中`n（非アクティブ・非表示）" 'g-audio' $null 14
Box 600 100 170 50 "使用中`n（アクティブ）" 'g-render' $null 15
Line 160 125 290 125 $true $false
Label 225 112 'Prewarm' 'middle' 'lb'
Label 225 148 '（任意。count 個）' 'middle' 'lb'
Line 480 112 600 112 $true $false
Label 540 104 'Rent' 'middle' 'lb'
Line 600 138 480 138 $true $false
Label 540 160 'Return' 'middle' 'lb'
PathLine 'M 90 100 C 90 20 685 20 685 100' $true $false
Label 385 33 'Rent で待機中が空いていない時: 新しく作って（Setup）、そのまま使用中にする' 'middle' 'lb'
Label 385 190 'Rent = SetActive(true) / Return = SetActive(false)（Destroy ではない）' 'middle' 'lb'
Label 385 212 'Setup（AddComponent を並べる処理）は、新しく作る時の1回だけ。' 'middle' 'lb'
Label 385 232 '借りた物には前回の値（位置・色）が残っているので、毎回セットし直す。' 'middle' 'lb'
SaveSvg

# ───────────────────────────────────────────────
# 5. 描画の順番(Scene::Draw の3パス)
# ───────────────────────────────────────────────
NewSvg 'draw' 800 480 'Scene::Draw の3つのパスと、その後の画面エフェクト・遷移'
$dx = 90; $dw = 690
Box $dx 20 $dw 62 "⑤ シーン切り替えの演出  SceneManager::DrawTransition()`n（FadeTransition / SpriteTransition。一番手前）" 'g-transition' $null 14
Box $dx 92 $dw 62 "④ 画面エフェクト  ScreenEffect::Draw()`n（下から Vignette → Flash → Fade。文字より手前）" 'g-animation' $null 14
Box $dx 164 $dw 76 "③ パス3  FlushOverlayQueue()  ＝ 文字・四角（呼んだ順）`nText::Draw / TextRenderer / Image::DrawRect`nGaugeRenderer / ButtonComponent の文字 / SliderComponent" 'g-ui' $null 14
Box $dx 250 $dw 76 "② パス2  FlushUiQueue()  ＝ UI スプライト（drawPriority → 作った順）`nSpriteMode::UI の SpriteRenderer、2D の粒（use3D=false）`n深度を使わない。z は使わない。必ず最前面" 'g-behavior' $null 14
Box $dx 336 $dw 90 "① パス1  UI 以外（drawPriority が小さい順。同じなら transform.position.z が小さい順）`n3D メッシュ・FBX・地形・水面 / World2D・World3D のスプライト`nコライダーの枠などのデバッグの線 / 背景（Image::Draw）`n※この間に出た UI スプライトと文字は、すぐには描かずに待ち行列に積む" 'g-render' $null 14
Line 72 440 72 30 $true $false
Label 4 452 '先に描く' 'start' 'lb'
Label 4 468 '（奥）' 'start' 'lb'
Label 4 16 '後に描く' 'start' 'lb'
Label 4 32 '（手前）' 'start' 'lb'
Label 435 462 'Scene::Draw が ①②③ を行い、GameEngine::Run が続けて ④⑤ を描いてから Text::EndDraw で画面に出します。' 'middle' 'lb'
SaveSvg

# ───────────────────────────────────────────────
# 6. 物理とコールバック
# ───────────────────────────────────────────────
NewSvg 'physics' 800 520 'Physics::Update から OnCollision / OnTrigger のコールバックまで'
Box 20 15 760 36 'GameEngine::Run の1フレーム:  Physics::Update(dt)   （ポーズ中などで dt が 0 なら何もしない）' 'g-physics' $null 14
Box 40 70 340 44 "① simulate(dt) → fetchResults()`nPhysX が動かして、当たりを調べる" 'n' $null 14
Box 420 70 340 44 "② fetchResults の中でコールバックが届く`n（Engine が PhysX の通知を変換する）" 'n' $null 14
Line 380 92 420 92 $true $false
Box 420 135 340 56 "接触（すり抜けない）:`nOnCollisionEnter / Stay / Exit(CollisionInfo)" 'g-physics' $null 13
Box 420 200 340 56 "トリガー（すり抜ける）:`nOnTriggerEnter / Stay / Exit(Collider2D* other)" 'g-physics' $null 13
Line 590 114 590 135 $true $false
Line 590 191 590 200 $false $false
Box 40 135 340 121 "届く先は、ぶつかった2つの GameObject`nそれぞれの全 Component`n（override した Component だけが反応する）`n`nKinematic 同士の押し戻しは`nEngine が position を手でずらす" 'n' $null 13
Box 40 280 340 50 "③ 予約された addActor / removeActor を実行`n（コールバック中は直接できないため）" 'n' $null 13
Box 420 280 340 50 "④ コールバック中の AddForce を加える`n⑤ トリガーに重なっている間: OnTriggerStay" 'n' $null 13
Line 380 305 420 305 $true $false
PathLine 'M 210 256 L 210 280' $true $false
Box 20 360 760 36 'そのあと SceneManager::Update(dt) → Scene::Update → 各 Component の Update' 'g-core' $null 14
Box 40 415 340 66 "RigidbodyComponent::Update`nDynamic: PhysX の結果 → transform`nKinematic: transform → PhysX の目標" 'g-physics' $null 13
Box 420 415 340 66 "あなたの Controller::Update`nSetVelocity() / AddForce() を呼ぶ`n（効くのは次の Physics::Update）" 'g-core' $null 13
Line 380 448 420 448 $false $false
PathLine 'M 400 396 L 400 415' $false $false
SaveSvg

# ───────────────────────────────────────────────
# 7. EventBus
# ───────────────────────────────────────────────
NewSvg 'eventbus' 820 330 'EventBus で、お互いを知らない物同士をつなぐ流れ'
Label 10 18 '送る側（Publish）' 'start' 'lb'
Label 330 18 'EventBus（名前 → 登録された関数の一覧）' 'start' 'lb'
Label 640 18 '聞く側（Subscribe）' 'start' 'lb'
Box 10 30 270 56 "EnemyController`nPublish(`"EnemyDefeated`")" 'g-core' $null 13
Box 10 120 270 56 "PlayerController`nPublish(`"PlayerDied`")" 'g-core' $null 13
Box 10 210 270 56 "PlayerController`nPublishObject(`"FireBullet`", owner)" 'g-core' $null 13
Box 330 30 190 56 "`"EnemyDefeated`"`n→ 登録された関数 1つ" 'g-animation' $null 13
Box 330 120 190 56 "`"PlayerDied`"`n→ 登録された関数 2つ" 'g-animation' $null 13
Box 330 210 190 56 "`"FireBullet`"`n→ 関数 1つ（GameObject 付き）" 'g-animation' $null 13
Line 280 58 330 58 $true $false
Line 280 148 330 148 $true $false
Line 280 238 330 238 $true $false
Box 570 30 240 56 "GameManager`nscore += 10" 'g-core' $null 13
Box 570 105 240 40 "GameManager: ゲームオーバーにする" 'g-core' $null 12
Box 570 155 240 40 "EnemyManager: 敵の出現を止める" 'g-core' $null 12
Box 570 210 240 56 "BulletManager`n撃った本人の向きに弾を出す" 'g-core' $null 13
Line 520 58 570 58 $true $true
Line 520 140 570 125 $true $true
Line 520 156 570 175 $true $true
Line 520 238 570 238 $true $true
Label 410 296 'Subscribe は Init で登録しておく。Publish すると、登録された順に呼ばれる。' 'middle' 'lb'
Label 410 316 'this を使うラムダを登録したら、Scene::Uninit() で EventBus::Get().Clear() を呼ぶ。' 'middle' 'lb'
SaveSvg

# ───────────────────────────────────────────────
# 8. シーンの切り替え
# ───────────────────────────────────────────────
NewSvg 'scene' 860 560 'SceneManager::ChangeScene から、新しいシーンが動き出すまで'
Label 10 18 '演出付き ChangeScene<T>(FadeTransition())' 'start' 'lb'
Label 470 18 '演出なし ChangeScene<T>()' 'start' 'lb'
Box 10 30 410 52 "① 切り替えを頼む（RequestChange）`n途中なら無視。演出の Begin() ＋ Time を止める（倍率 0）" 'g-core' $null 13
Box 10 105 410 52 "② 覆う  transition->Update(止まらない時間)`n画面がだんだん隠れる（ゲームの時間は止まっている）" 'g-transition' $null 13
Box 10 180 410 150 "③ 覆い終わった（IsCovered）→ SwitchScene()`n  1. 古いシーンの Uninit()`n  2. ScreenEffect::OnSceneChanged() と CameraShake::Stop()`n  3. 新しいシーンを作って Init()`n  4. 使わなくなった画像・音・モデルを解放`n     （Resources::UnloadUnused）`n  → 演出の BeginOpen()" 'g-physics' $null 13
Box 10 355 410 52 "④ 開く  画面がだんだん見えてくる" 'g-transition' $null 13
Box 10 430 410 66 "⑤ 開き終わった（IsFinished）`n演出を片付けて、Time を元の倍率に戻す（Resume）`n→ 以後は普通に Update / Draw" 'g-core' $null 13
Line 215 82 215 105 $true $false
Line 215 157 215 180 $true $false
Line 215 330 215 355 $true $false
Line 215 407 215 430 $true $false
Box 470 30 380 52 "① 切り替えを頼む（nextScene に覚えるだけ）" 'g-core' $null 13
Box 470 105 380 98 "② 次の SceneManager::Update の最初で`n   SwitchScene()（上の ③ と同じ）`n   → 同じフレームで、新しいシーンの`n   Update まで進む" 'g-physics' $null 13
Line 660 82 660 105 $true $false
Label 470 240 'どちらも、頼んでから終わるまでの間に呼ばれた' 'start' 'lb'
Label 470 258 'ChangeScene は無視されます（二重押し対策）。' 'start' 'lb'
Label 470 290 'ScreenEffect::Fade の onComplete の中で' 'start' 'lb'
Label 470 308 'ChangeScene を呼ぶと、ScreenEffect::Update' 'start' 'lb'
Label 470 326 '（SceneManager::Update の前）なので、' 'start' 'lb'
Label 470 344 '同じフレームのうちに切り替わります。' 'start' 'lb'
Label 470 380 '最初のシーンは main.cpp で' 'start' 'lb'
Label 470 398 'ChangeScene<TitleScene>() を呼ぶと、' 'start' 'lb'
Label 470 416 'Run の最初のフレームで始まります。' 'start' 'lb'
SaveSvg

# ───────────────────────────────────────────────
# 9. 1フレームの流れ(GameEngine::Run)
# ───────────────────────────────────────────────
NewSvg 'frame' 880 770 'GameEngine::Run の 1 フレームの流れ'
Label 10 22 '前半: 動かす（更新）' 'start' 'lb'
Label 450 22 '後半: 描く' 'start' 'lb'
$left = @(
    @('ウィンドウのメッセージ処理', 'PeekMessage ... 閉じられたら終了', 'n'),
    @('Time::Update', 'このフレームの時間（dt）を計算する', 'g-core'),
    @('Input::Update', 'キー・マウス・パッドの今の状態を読む', 'g-core'),
    @('CameraShake::Update', '画面の揺れを進める（ポーズ中も進む）', 'g-render'),
    @('ScreenEffect::Update', 'Flash / Vignette / Fade を進める（Fade の onComplete はここ）', 'g-render'),
    @('Physics::Update(dt)', '物理を進め、OnCollision / OnTrigger を呼ぶ', 'g-physics'),
    @('SceneManager::Update(dt)', 'シーンの切り替え ＋ Scene::Update（全 Component の Update）', 'g-core'),
    @('Audio::Update', '動いた後の位置で、音の聞こえ方を決める', 'g-audio')
)
$right = @(
    @('Image::BeginFrame', '3D カメラを 2D カメラに合わせ、画面と深度を消す', 'g-render'),
    @('CameraShake::BeginDraw', '揺れの分だけカメラをずらす（描き終わったら戻す）', 'g-render'),
    @('Visibility::BeginFrame', '揺れ込みのカメラで視錐台を作る', 'g-render'),
    @('RenderStats::BeginFrame', '描画命令・三角形の数を数え直す', 'g-render3d'),
    @('Lighting::Update', '点光源・スポットライトを集める', 'g-render3d'),
    @('Shadow::Render', '影のパス（有効な時だけ）', 'g-render3d'),
    @('Text::BeginDraw', '文字（Direct2D）の描画を始める', 'g-ui'),
    @('SceneManager::Draw → Scene::Draw', '3 つのパス: ① UI 以外 → ② UI スプライト → ③ 文字・四角', 'g-core'),
    @('ScreenEffect::Draw', '画面エフェクト（文字より手前）', 'g-animation'),
    @('SceneManager::DrawTransition', 'シーン切り替えの演出（一番手前）', 'g-transition'),
    @('Text::EndDraw', '文字の描画を終える', 'g-ui'),
    @('CameraShake::EndDraw', 'カメラを元の位置に戻す', 'g-render'),
    @('Image::EndFrame', '画面に出す（Present）。次のフレームへ', 'g-render')
)
for ($i = 0; $i -lt $left.Count; $i++) {
    $by = 36 + $i * 58
    $t = $left[$i][0] + "`n" + $left[$i][1]
    $cl = $left[$i][2]
    Box 10 $by 410 44 $t $cl $null 13
    if ($i -lt $left.Count - 1) { Line 215 ($by + 44) 215 ($by + 58) $true $false }
}
for ($i = 0; $i -lt $right.Count; $i++) {
    $by = 36 + $i * 58
    $t = $right[$i][0] + "`n" + $right[$i][1]
    $cl = $right[$i][2]
    Box 450 $by 420 44 $t $cl $null 13
    if ($i -lt $right.Count - 1) { Line 660 ($by + 44) 660 ($by + 58) $true $false }
}
PathLine 'M 420 464 L 435 464 L 435 29 L 660 29 L 660 36' $true $false
Label 20 520 '※ 描画は、更新がすべて終わってから行います。' 'start' 'lb'
Label 20 540 '※ Scene::Draw の中では、各 GameObject の Draw が drawPriority の順に呼ばれます。' 'start' 'lb'
SaveSvg
