#include "Physics.h"
#include "Collider.h"
#include "../Core/GameObject.h"
#include "../Core/MathUtil.h"
#include <windows.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <PxPhysicsAPI.h>

using namespace physx;

namespace {
    // ★ PhysXの既定フィルターシェーダーは、パフォーマンスのためキネマティック同士・
    //   キネマティックと静的物体同士の接触を検出しないことがある。このゲームは
    //   Player/Enemyのようなキネマティックなオブジェクト同士の当たり判定(OnCollisionEnter等)
    //   を必要とするため、全てのペアで接触/トリガー通知を要求する専用のシェーダーを使う
    PxFilterFlags CollisionFilterShader(
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize) {
        if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
            pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        }
        else {
            pairFlags = PxPairFlag::eCONTACT_DEFAULT
                | PxPairFlag::eNOTIFY_TOUCH_FOUND
                | PxPairFlag::eNOTIFY_TOUCH_PERSISTS
                | PxPairFlag::eNOTIFY_TOUCH_LOST
                | PxPairFlag::eNOTIFY_CONTACT_POINTS
                | PxPairFlag::eDETECT_DISCRETE_CONTACT
                // ★ CCD(連続衝突判定)。薄い箱(SquareRigidbodyComponentなど)同士が角で
                //   ぶつかった時、通常の判定だけでは押し返しが弱く貫通することがあるため、
                //   スイープ(掃引)ベースの判定も併用して貫通を防ぐ
                | PxPairFlag::eDETECT_CCD_CONTACT;
        }
        return PxFilterFlag::eDEFAULT;
    }

    // PxRigidDynamicがキネマティックかどうか(静的PxRigidStaticはfalseになる)
    bool IsKinematic(PxRigidActor* actor) {
        PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
        return dynamic && (dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC);
    }

    // 力の影響で動く、純粋なDynamic(キネマティックではない)かどうか
    bool IsPureDynamic(PxRigidActor* actor) {
        PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
        return dynamic && !(dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC);
    }

    // ─── トリガーの「重なっている間」(OnTriggerStay) ─────────────
    // ★ PhysX 4.1はトリガーの「重なっている間」の通知を出さないので、Enterで組み合わせを覚えて、
    //   Exitで忘れ、Physics::Update()の最後に、覚えている組み合わせへ毎回OnTriggerStayを呼ぶ

    // トリガーに重なっている組み合わせ1つ分
    struct TriggerPair {
        PxRigidActor* triggerActor = nullptr;   // トリガー側
        PxRigidActor* otherActor = nullptr;     // 入ってきた側
        unsigned long long enterStep = 0;       // 入った時のgStepCount(このstepの間はStayを呼ばない)
    };

    std::vector<TriggerPair> gTriggerPairs;     // 今重なっている組み合わせ(重複なし)
    unsigned long long gStepCount = 0;          // Physics::Update()でシミュレーションを進めた回数

    // 動いている(非アクティブでもDestroy済みでもない)オブジェクトか
    bool IsWorkingObject(const GameObject* obj) {
        return obj && obj->GetIsActive() && !obj->GetIsDestroyed();
    }

    // 覚えている組み合わせを探す(無ければgTriggerPairs.end())。アドレスを比べるだけで、アクターの中身は読まない
    std::vector<TriggerPair>::iterator FindTriggerPair(const PxRigidActor* triggerActor, const PxRigidActor* otherActor) {
        return std::find_if(gTriggerPairs.begin(), gTriggerPairs.end(),
            [=](const TriggerPair& pair) { return pair.triggerActor == triggerActor && pair.otherActor == otherActor; });
    }

    // 組み合わせを覚える(PhysXが同じEnterを2回送ってきても、重複して覚えない)
    void RememberTriggerPair(PxRigidActor* triggerActor, PxRigidActor* otherActor) {
        if (FindTriggerPair(triggerActor, otherActor) != gTriggerPairs.end()) return;
        gTriggerPairs.push_back({ triggerActor, otherActor, gStepCount });
    }

    // OnTriggerStayを呼んでよいか: まだ覚えていて、両方とも動いているオブジェクトか
    bool CanCallTriggerStay(const TriggerPair& pair) {
        if (FindTriggerPair(pair.triggerActor, pair.otherActor) == gTriggerPairs.end()) return false;
        Collider2D* colTrigger = static_cast<Collider2D*>(pair.triggerActor->userData);
        Collider2D* colOther = static_cast<Collider2D*>(pair.otherActor->userData);
        return colTrigger && colOther && IsWorkingObject(colTrigger->owner) && IsWorkingObject(colOther->owner);
    }

    // 覚えている組み合わせすべてに、OnTriggerStayを両方向で呼ぶ
    void CallTriggerStay() {
        // ★ コールバックの中でプールに戻す・Destroy・SetActive・新しいオブジェクトを作るなどで
        //   一覧が変わっても壊れないように、コピーを回す。呼ぶ前に毎回「まだ覚えているか」を確かめる
        //   (忘れた組み合わせのアクターは、解放済みかもしれないので触らない)
        std::vector<TriggerPair> pairs = gTriggerPairs;
        for (const TriggerPair& pair : pairs) {
            if (pair.enterStep == gStepCount) continue;   // 入ったフレームはEnterだけ(次のフレームからStay。Unityと同じ)
            if (!CanCallTriggerStay(pair)) continue;
            Collider2D* colTrigger = static_cast<Collider2D*>(pair.triggerActor->userData);
            Collider2D* colOther = static_cast<Collider2D*>(pair.otherActor->userData);
            colTrigger->owner->OnTriggerStay(colOther);

            // 1つ目のコールバックで、どちらかをプールに戻した・Destroyした時は、もう片方には呼ばない
            if (!CanCallTriggerStay(pair)) continue;
            colOther->owner->OnTriggerStay(colTrigger);
        }
    }

    // PhysXのシミュレーション結果を、GameObjectのOnTriggerEnter/OnCollisionEnterなどの
    // コールバックへ変換して届ける。旧・自作2D当たり判定システム(Collider.cpp)が担っていた役割。
    class SimulationEventCallback : public PxSimulationEventCallback {
    public:
        void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
        void onWake(PxActor**, PxU32) override {}
        void onSleep(PxActor**, PxU32) override {}
        void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}

        void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override {
            // ★ onTriggerと同じ理由のガード: シーン終了時などにアクターをまとめて
            //   release()すると、「接触していた相手が削除された」という通知が後から
            //   (別のfetchResults()のタイミングで)届くことがある。この時点では
            //   actors[0]/[1]は既に解放済み(もう存在しない)ので、絶対に触ってはいけない
            if (pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1)) {
                return;
            }

            PxRigidActor* actorA = pairHeader.actors[0];
            PxRigidActor* actorB = pairHeader.actors[1];
            Collider2D* colA = static_cast<Collider2D*>(actorA->userData);
            Collider2D* colB = static_cast<Collider2D*>(actorB->userData);
            if (!colA || !colB) return;

            bool kinA = IsKinematic(actorA);
            bool kinB = IsKinematic(actorB);
            bool dynA = IsPureDynamic(actorA);
            bool dynB = IsPureDynamic(actorB);

            for (PxU32 i = 0; i < nbPairs; i++) {
                const PxContactPair& pair = pairs[i];

                // ★ こちらは個々のペア(shape)単位での同じガード。消えたshapeが絡む
                //   ペアはスキップする(接触点を取り出すextractContacts等が危険なため)
                if (pair.flags & (PxContactPairFlag::eREMOVED_SHAPE_0 | PxContactPairFlag::eREMOVED_SHAPE_1)) {
                    continue;
                }

                PxContactPairPoint points[4];
                PxU32 n = pair.extractContacts(points, 4);
                PxVec3 normal = (n > 0) ? points[0].normal : PxVec3(0.f, 0.f, 0.f);  // Aから見てBへ向かう向き
                float depth = (n > 0) ? -points[0].separation : 0.f;
                if (depth < 0.f) depth = 0.f;

                CollisionInfo infoA{ colB, { -normal.x, -normal.y }, depth };  // Aから見て、Bから離れる方向
                CollisionInfo infoB{ colA, {  normal.x,  normal.y }, depth };  // Bから見て、Aから離れる方向

                // ★ 押し戻し: キネマティックな側だけを手動でずらす(相手がDynamicでない時に限る)。
                //   相手がDynamicなら、PhysX自身が正しく押し返してくれるのでここでは何もしない。
                //   PlayerControllerはW/Sでposition.zを動かす仕様なので、x/yだけでなくzも補正する
                if (depth > 0.f) {
                    if (kinA && !colA->isStatic && !dynB) {
                        colA->owner->transform.position.x -= normal.x * depth;
                        colA->owner->transform.position.y -= normal.y * depth;
                        colA->owner->transform.position.z -= normal.z * depth;
                    }
                    if (kinB && !colB->isStatic && !dynA) {
                        colB->owner->transform.position.x += normal.x * depth;
                        colB->owner->transform.position.y += normal.y * depth;
                        colB->owner->transform.position.z += normal.z * depth;
                    }
                }

                if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
                    colA->owner->OnCollisionEnter(infoA);
                    colB->owner->OnCollisionEnter(infoB);
                }
                else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
                    colA->owner->OnCollisionStay(infoA);
                    colB->owner->OnCollisionStay(infoB);
                }
                else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                    colA->owner->OnCollisionExit(infoA);
                    colB->owner->OnCollisionExit(infoB);
                }
            }
        }

        void onTrigger(PxTriggerPair* pairs, PxU32 count) override {
            for (PxU32 i = 0; i < count; i++) {
                const PxTriggerPair& pair = pairs[i];
                // アクターやシェイプがこのフレームで消えている場合は触らない(覚えていた組み合わせは忘れる)
                if (pair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER)) {
                    // ★ アクターは解放済みかもしれないので、アドレスを比べるだけにする。
                    //   このstepで入ったばかりの組み合わせ(プールで再利用されて、また入ってきた時)は、
                    //   消えた方の知らせとは別物なので忘れない
                    auto found = FindTriggerPair(pair.triggerActor, pair.otherActor);
                    if (found != gTriggerPairs.end() && found->enterStep != gStepCount) gTriggerPairs.erase(found);
                    continue;
                }

                Collider2D* colTrigger = static_cast<Collider2D*>(pair.triggerActor->userData);
                Collider2D* colOther = static_cast<Collider2D*>(pair.otherActor->userData);
                if (!colTrigger || !colOther) continue;

                if (pair.status & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
                    // Stay用に覚える。Enterの中で相手をプールに戻した時にちゃんと忘れられるよう、呼ぶ前に覚える
                    if (IsWorkingObject(colTrigger->owner) && IsWorkingObject(colOther->owner)) {
                        RememberTriggerPair(pair.triggerActor, pair.otherActor);
                    }
                    colTrigger->owner->OnTriggerEnter(colOther);
                    colOther->owner->OnTriggerEnter(colTrigger);
                }
                else if (pair.status & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                    auto found = FindTriggerPair(pair.triggerActor, pair.otherActor);
                    if (found != gTriggerPairs.end()) gTriggerPairs.erase(found);
                    colTrigger->owner->OnTriggerExit(colOther);
                    colOther->owner->OnTriggerExit(colTrigger);
                }
            }
        }
    };

    // アクターからGameObjectを取り出す(当たり判定のコールバックと同じく、userDataのCollider2Dのownerを使う)
    GameObject* GetGameObject(const PxRigidActor* actor) {
        if (!actor) return nullptr;
        Collider2D* collider = static_cast<Collider2D*>(actor->userData);
        return collider ? collider->owner : nullptr;
    }

    // Raycast / Overlap で、探す物を選ぶフィルター
    class QueryFilter : public PxQueryFilterCallback {
    public:
        QueryFilter(const std::string& tag, QueryTriggers triggers, PxQueryHitType::Enum hitType)
            : tag(tag), triggers(triggers), hitType(hitType) {
        }

        PxQueryHitType::Enum preFilter(const PxFilterData&, const PxShape* shape,
            const PxRigidActor* actor, PxHitFlags&) override {
            GameObject* obj = GetGameObject(actor);
            if (!obj) return PxQueryHitType::eNONE;
            // ★ SetActive(false)したアクターがシーンから外れるのは次のPhysics::Update()なので、
            //   それまでの間も見つからないように、GameObjectの状態で確かめる
            if (!obj->GetIsActive() || obj->GetIsDestroyed()) return PxQueryHitType::eNONE;
            if (triggers == QueryTriggers::Ignore && (shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)) return PxQueryHitType::eNONE;
            if (!tag.empty() && obj->GetTag() != tag) return PxQueryHitType::eNONE;
            return hitType;
        }

        // Raycastだけが使う: originが中に入っている物には、PhysXは「距離0で当たった」と返すので、当たらなかった扱いにする
        PxQueryHitType::Enum postFilter(const PxFilterData&, const PxQueryHit& hit) override {
            const PxLocationHit& locationHit = static_cast<const PxLocationHit&>(hit);
            if (locationHit.distance <= 0.f) return PxQueryHitType::eNONE;
            return hitType;
        }

    private:
        const std::string& tag;          // 探すタグ(""なら全部)
        QueryTriggers triggers;          // トリガーも探すか
        PxQueryHitType::Enum hitType;    // 見つけた物の扱い(Raycast=eBLOCK(一番近い物だけ)、Overlap=eTOUCH(全部))
    };

    // Overlapで見つかった物を、すべてvectorに集める(バッファがいっぱいになる度にprocessTouchesが呼ばれる)
    class OverlapCollector : public PxOverlapCallback {
    public:
        std::vector<GameObject*> objects;   // 見つかったオブジェクト(重複なし)

        OverlapCollector(PxOverlapHit* buffer, PxU32 bufferSize) : PxOverlapCallback(buffer, bufferSize) {}

        PxAgain processTouches(const PxOverlapHit* hits, PxU32 count) override {
            Collect(hits, count);
            return true;   // まだあれば続けて受け取る(nbTouchesは0に戻る)
        }

        void Collect(const PxOverlapHit* hits, PxU32 count) {
            for (PxU32 i = 0; i < count; i++) {
                GameObject* obj = GetGameObject(hits[i].actor);
                if (obj && std::find(objects.begin(), objects.end(), obj) == objects.end()) objects.push_back(obj);
            }
        }
    };

    PxForceMode::Enum ToPxForceMode(ForceMode mode) {
        switch (mode) {
        case ForceMode::Impulse:        return PxForceMode::eIMPULSE;
        case ForceMode::Acceleration:   return PxForceMode::eACCELERATION;
        case ForceMode::VelocityChange: return PxForceMode::eVELOCITY_CHANGE;
        case ForceMode::Force:
        default:                        return PxForceMode::eFORCE;
        }
    }

    // 力・回す力を今すぐ加える。ロックした軸(SetFreezePositionX/SetFreezeRotationなど)の成分は0にしてから加える
    void AddForceNow(PxRigidActor* actor, PxVec3 value, ForceMode mode, bool torque) {
        PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
        if (!dynamic || !actor->getScene()) return;   // シーンに入っていない(非アクティブ)アクターには加えない
        if (dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC) return;

        PxRigidDynamicLockFlags locks = dynamic->getRigidDynamicLockFlags();
        if (torque) {
            if (locks & PxRigidDynamicLockFlag::eLOCK_ANGULAR_X) value.x = 0.f;
            if (locks & PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y) value.y = 0.f;
            if (locks & PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z) value.z = 0.f;
            dynamic->addTorque(value, ToPxForceMode(mode));
        }
        else {
            if (locks & PxRigidDynamicLockFlag::eLOCK_LINEAR_X) value.x = 0.f;
            if (locks & PxRigidDynamicLockFlag::eLOCK_LINEAR_Y) value.y = 0.f;
            if (locks & PxRigidDynamicLockFlag::eLOCK_LINEAR_Z) value.z = 0.f;
            dynamic->addForce(value, ToPxForceMode(mode));
        }
    }

    // コールバックの中で呼ばれたApplyForceを、fetchResults()の後まで預かっておくための1件分
    struct PendingForce {
        PxRigidActor* actor = nullptr;
        PxVec3 value{ 0.f, 0.f, 0.f };
        ForceMode mode = ForceMode::Force;
        bool torque = false;
    };
}

namespace Physics {
    static PxDefaultAllocator gAllocator;
    static PxDefaultErrorCallback gErrorCallback;
    static PxFoundation* gFoundation = nullptr;
    static PxPhysics* gPhysics = nullptr;
    static PxDefaultCpuDispatcher* gDispatcher = nullptr;
    static PxScene* gScene = nullptr;
    static PxMaterial* gDefaultMaterial = nullptr;
    static PxCooking* gCooking = nullptr;   // 初めてGetCooking()が呼ばれた時に作る
    static SimulationEventCallback gEventCallback;
    static std::vector<std::pair<PxRigidActor*, bool>> gPendingSceneChanges;  // {アクター, true=追加/false=削除}
    static std::vector<PendingForce> gPendingForces;  // コールバックの中で呼ばれたApplyForce(fetchResultsの後で加える)
    static bool gFetchingResults = false;             // fetchResults()の途中(= onContact/onTriggerのコールバックの中)か

    bool Init() {
        gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
        if (!gFoundation) { OutputDebugStringA("★ Physics: PxCreateFoundation 失敗\n"); return false; }

        // ★ PxTolerancesScaleはデフォルトだと「1単位=1m」を想定した値になっており、
        //   このエンジンの「100単位=1m」というスケール感のままだと、スリープ判定や接触の
        //   許容誤差がズレてシミュレーションが不安定になる。lengthをそのスケールに合わせておく
        PxTolerancesScale tolerancesScale;
        tolerancesScale.length = 100.0f;
        tolerancesScale.speed = 981.0f;

        gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, tolerancesScale, true, nullptr);
        if (!gPhysics) { OutputDebugStringA("★ Physics: PxCreatePhysics 失敗\n"); return false; }

        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        // ★ 標準的な重力加速度(9.81 m/s^2)を、このエンジンのスケール(100単位=1m)に合わせて981にしてある
        sceneDesc.gravity = PxVec3(0.0f, -981.0f, 0.0f);

        gDispatcher = PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = CollisionFilterShader;
        sceneDesc.simulationEventCallback = &gEventCallback;
        // ★ CCD(連続衝突判定)をシーン全体で使えるようにする(RigidbodyComponent.cppでDynamic
        //   アクターごとに有効化している)。これが無いと、各アクターのCCDフラグが無視される
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;

        gScene = gPhysics->createScene(sceneDesc);
        if (!gScene) { OutputDebugStringA("★ Physics: createScene 失敗\n"); return false; }

        // RigidbodyComponentが特に指定しなければ使う、共通のデフォルト材質
        // (静止摩擦, 動摩擦, 反発係数)
        gDefaultMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.3f);
        if (!gDefaultMaterial) { OutputDebugStringA("★ Physics: createMaterial 失敗\n"); return false; }

        return true;
    }

    void Update(float dt) {
        if (!gScene || dt <= 0.f) return;   // ポーズ中はOnTriggerStayも呼ばない
        gStepCount++;
        gScene->simulate(dt);
        gFetchingResults = true;    // この間にonContact/onTrigger(OnCollisionStayなど)が呼ばれる
        gScene->fetchResults(true);
        gFetchingResults = false;

        // ★ シミュレーションコールバック(onContact/onTrigger)の中では、PhysXのAPI書き込み
        //   (addActor/removeActorなど)が禁止されている。そこから予約されていた変更を、
        //   fetchResults()が完全に終わった今、安全なタイミングでまとめて反映する
        for (auto& change : gPendingSceneChanges) {
            PxRigidActor* actor = change.first;
            bool add = change.second;
            if (add) gScene->addActor(*actor);
            else {
                gScene->removeActor(*actor);
                // SetActive(false)の後、外れるまでの間に入った組み合わせも忘れる
                ForgetTriggerPairs(actor);
            }
        }
        gPendingSceneChanges.clear();

        // コールバックの中で預かった力を加える(シーンの変更の後なので、シーンから外れたアクターには加わらない)
        for (auto& force : gPendingForces) {
            AddForceNow(force.actor, force.value, force.mode, force.torque);
        }
        gPendingForces.clear();

        // トリガーに重なっている組み合わせへOnTriggerStayを呼ぶ。
        // fetchResults()の外なので、この中で呼ばれたAddForceなどはその場で効く
        CallTriggerStay();
    }

    void Uninit() {
        gPendingForces.clear();
        gTriggerPairs.clear();
        if (gDefaultMaterial) { gDefaultMaterial->release(); gDefaultMaterial = nullptr; }
        if (gCooking) { gCooking->release(); gCooking = nullptr; }
        if (gScene) { gScene->release(); gScene = nullptr; }
        if (gDispatcher) { gDispatcher->release(); gDispatcher = nullptr; }
        if (gPhysics) { gPhysics->release(); gPhysics = nullptr; }
        if (gFoundation) { gFoundation->release(); gFoundation = nullptr; }
    }

    PxPhysics* GetPhysics() { return gPhysics; }
    PxScene* GetScene() { return gScene; }
    PxMaterial* GetDefaultMaterial() { return gDefaultMaterial; }

    PxCooking* GetCooking() {
        if (!gCooking && gFoundation && gPhysics) {
            gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(gPhysics->getTolerancesScale()));
            if (!gCooking) OutputDebugStringA("★ Physics: PxCreateCooking 失敗\n");
        }
        return gCooking;
    }

    void QueueSceneChange(PxRigidActor* actor, bool add) {
        gPendingSceneChanges.push_back({ actor, add });
    }

    void CancelQueuedSceneChange(PxRigidActor* actor) {
        gPendingSceneChanges.erase(
            std::remove_if(gPendingSceneChanges.begin(), gPendingSceneChanges.end(),
                [actor](const std::pair<PxRigidActor*, bool>& change) { return change.first == actor; }),
            gPendingSceneChanges.end());
        gPendingForces.erase(
            std::remove_if(gPendingForces.begin(), gPendingForces.end(),
                [actor](const PendingForce& force) { return force.actor == actor; }),
            gPendingForces.end());
    }

    void ForgetTriggerPairs(PxRigidActor* actor) {
        gTriggerPairs.erase(
            std::remove_if(gTriggerPairs.begin(), gTriggerPairs.end(),
                [actor](const TriggerPair& pair) { return pair.triggerActor == actor || pair.otherActor == actor; }),
            gTriggerPairs.end());
    }

    // ─── 問い合わせ ─────────────────────────────
    bool Raycast(XMFLOAT3 origin, XMFLOAT3 direction, float maxDistance,
        RaycastHit& hit, const std::string& tag, QueryTriggers triggers) {
        hit = RaycastHit();
        if (!gScene || maxDistance <= 0.f) return false;
        XMFLOAT3 dir = Math::Normalize(direction);
        if (Math::Length(dir) < 0.5f) return false;   // 長さ0の向き(Normalizeが0ベクトルを返した)

        // 条件に合う物だけを「行く手をふさぐ物(eBLOCK)」にして、一番近い1つを探す
        QueryFilter filter(tag, triggers, PxQueryHitType::eBLOCK);
        PxQueryFilterData filterData;
        filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER | PxQueryFlag::ePOSTFILTER;
        PxRaycastBuffer buffer;
        gScene->raycast(PxVec3(origin.x, origin.y, origin.z), PxVec3(dir.x, dir.y, dir.z), maxDistance,
            buffer, PxHitFlag::eDEFAULT, filterData, &filter);
        if (!buffer.hasBlock) return false;

        const PxRaycastHit& block = buffer.block;
        hit.object = GetGameObject(block.actor);
        hit.point = { block.position.x, block.position.y, block.position.z };
        hit.normal = { block.normal.x, block.normal.y, block.normal.z };
        hit.distance = block.distance;
        return hit.object != nullptr;
    }

    namespace {
        // OverlapSphere / OverlapBox の共通部分
        std::vector<GameObject*> Overlap(const PxGeometry& geometry, XMFLOAT3 center,
            const std::string& tag, QueryTriggers triggers) {
            if (!gScene) return {};

            // 条件に合う物を全部「触れている物(eTOUCH)」として集める
            QueryFilter filter(tag, triggers, PxQueryHitType::eTOUCH);
            PxQueryFilterData filterData;
            filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;
            PxOverlapHit hitBuffer[64];
            OverlapCollector collector(hitBuffer, 64);
            gScene->overlap(geometry, PxTransform(PxVec3(center.x, center.y, center.z)), collector, filterData, &filter);

            // 最後にバッファへ残っている分も集める(processTouchesで受け取った分はnbTouchesが0に戻っている)
            collector.Collect(collector.touches, collector.nbTouches);
            return collector.objects;
        }
    }

    std::vector<GameObject*> OverlapSphere(XMFLOAT3 center, float radius,
        const std::string& tag, QueryTriggers triggers) {
        if (radius <= 0.f) return {};
        return Overlap(PxSphereGeometry(radius), center, tag, triggers);
    }

    std::vector<GameObject*> OverlapBox(XMFLOAT3 center, XMFLOAT3 size,
        const std::string& tag, QueryTriggers triggers) {
        if (size.x <= 0.f || size.y <= 0.f || size.z <= 0.f) return {};
        return Overlap(PxBoxGeometry(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f), center, tag, triggers);
    }

    // ─── 力(Engine内部用) ─────────────────────────────
    void ApplyForce(PxRigidActor* actor, XMFLOAT3 value, ForceMode mode, bool torque) {
        if (!actor) return;
        PxVec3 pxValue(value.x, value.y, value.z);
        if (gFetchingResults) {
            // ★ PhysXのコールバックの中ではSDKへの書き込みが禁止されている(PxSimulationEventCallbackの注意書き)。
            //   ここで預かって、fetchResults()が終わった直後(Update()の最後)に加える
            gPendingForces.push_back({ actor, pxValue, mode, torque });
            return;
        }
        AddForceNow(actor, pxValue, mode, torque);
    }
}
