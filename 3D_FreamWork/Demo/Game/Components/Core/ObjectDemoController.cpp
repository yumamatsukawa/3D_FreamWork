#include "ObjectDemoController.h"
#include "SpinComponent.h"
#include "BobComponent.h"
#include "../Common/DemoTextStyle.h"
#include "../Common/DemoMouseGround.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    constexpr float spinSpeed = 120.f;     // 中央の物に付ける回る部品の速さ(度 / 秒)
    constexpr float bobHeight = 40.f;      // 上下に動く部品の幅
    constexpr float bobSpeed = 4.f;        // 上下に動く部品の速さ
    constexpr float infoX = -610.f;        // 状態の文字の左端
    constexpr float infoY = 190.f;         // 状態の文字の1行目(説明の板の下)
    constexpr float infoGap = 32.f;        // 行の間隔

    // 部品の状態を文字にする(付いていない / 動いている / 止めている)
    const wchar_t* GetStateText(const Component* component) {
        if (!component) return L"none";
        return component->IsEnabled() ? L"ON" : L"OFF (SetEnabled false)";
    }
}

bool ObjectDemoController::GetClickPosition(DirectX::XMFLOAT3& position) const {
    // 2D: マウスの下のワールド座標
    if (mode == DemoMode::Mode2D) {
        DirectX::XMFLOAT2 mouse = Input::GetMouseWorldPosition();
        position = { mouse.x, mouse.y, 0.f };
        return true;
    }

    // 3D: マウスの線が、地面に当たる場所(地面に当たらなければfalse)。物が地面に埋まらないよう少し上に出す
    DirectX::XMFLOAT3 ground;
    if (!GetMouseGroundPoint(groundY, ground)) return false;
    position = { ground.x, groundY + 20.f, ground.z };
    return true;
}

void ObjectDemoController::Spawn(const DirectX::XMFLOAT3& position) {
    // 出す物を作る(Updateの中で作っても大丈夫。次のフレームから動く)
    GameObject* object = GetOwner()->GetScene()->CreateObject("Spawned");
    object->transform.position = position;
    if (mode == DemoMode::Mode3D) {
        object->transform.scale = { 40.f, 40.f, 40.f };
        auto* mesh = object->AddComponent<MeshRenderer>(Mesh::CreateCube());
        mesh->SetColor(Math::RandomRange(0.3f, 1.f), Math::RandomRange(0.3f, 1.f), Math::RandomRange(0.3f, 1.f), 1.f);
    }
    else {
        object->transform.scale = { 50.f, 50.f, 1.f };
        object->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(Math::RandomRange(0, 8));
    }

    // 回る部品を付けて、消えるまでの時間を数え始める
    object->AddComponent<SpinComponent>(mode, Math::RandomRange(-360.f, 360.f));
    spawned.push_back({ object, Timer(spawnedLifeTime) });
    spawnedTotal++;
}

void ObjectDemoController::DestroyAllSpawned() {
    for (Spawned& item : spawned) item.object->Destroy();
    spawned.clear();
}

void ObjectDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1: 回る部品を付ける(すでに付いていれば何もしない。GetComponentで付いているか調べる)
    if (Input::GetKeyDown(KEY_1) && !target->GetComponent<SpinComponent>()) {
        target->AddComponent<SpinComponent>(mode, spinSpeed);
    }

    // 2: 上下に動く部品を付ける
    if (Input::GetKeyDown(KEY_2) && !target->GetComponent<BobComponent>()) {
        target->AddComponent<BobComponent>(bobHeight, bobSpeed);
    }

    // 3: 付いている部品を全部止める / 再開する(SetEnabled。部品そのものは付いたまま)
    if (Input::GetKeyDown(KEY_3)) {
        SpinComponent* spin = target->GetComponent<SpinComponent>();
        BobComponent* bob = target->GetComponent<BobComponent>();
        const bool anyRunning = (spin && spin->IsEnabled()) || (bob && bob->IsEnabled());
        if (spin) spin->SetEnabled(!anyRunning);
        if (bob) bob->SetEnabled(!anyRunning);
    }

    // クリック: 小さな物を出す
    DirectX::XMFLOAT3 position;
    if (Input::GetKeyDown(MOUSE_LEFT) && (int)spawned.size() < maxSpawned && GetClickPosition(position)) Spawn(position);

    // X: 出した物をすべてDestroyする
    if (Input::GetKeyDown(KEY_X)) DestroyAllSpawned();

    // 出した物の寿命: 時間が来たらDestroyして一覧から外す(Destroyした物は、このフレームの終わりに本当に消える)
    for (size_t i = 0; i < spawned.size();) {
        if (spawned[i].life.Update(dt)) {
            spawned[i].object->Destroy();
            spawned.erase(spawned.begin() + i);
        }
        else {
            i++;
        }
    }
}

void ObjectDemoController::Draw() {
    // 中央の物に付いている部品の状態(GetComponentで調べた結果)
    float y = infoY;
    Text::Draw(L"中央の物に付いている Component", infoX, y, MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f }));
    y -= infoGap;
    Text::Draw(std::wstring(L"SpinComponent : ") + GetStateText(target->GetComponent<SpinComponent>()), infoX, y, MakeDemoTextStyle(22.f));
    y -= infoGap;
    Text::Draw(std::wstring(L"BobComponent  : ") + GetStateText(target->GetComponent<BobComponent>()), infoX, y, MakeDemoTextStyle(22.f));

    // クリックで出した物の数
    y -= infoGap * 1.5f;
    Text::Draw(L"出した物(" + std::to_wstring((int)spawned.size()) + L" 個。今までに " + std::to_wstring(spawnedTotal) + L" 個)",
        infoX, y, MakeDemoTextStyle(22.f, { 0.7f, 1.0f, 0.7f, 1.0f }));
    y -= infoGap;
    Text::Draw(L"3秒たつとDestroyで消えます", infoX, y, MakeDemoTextStyle(20.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
}
