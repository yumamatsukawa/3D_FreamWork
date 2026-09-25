#include "Physics.h"
#include "Collider.h"
#include "GameObject.h"
#include <windows.h>
#include <vector>
#include <utility>
#include <PxPhysicsAPI.h>

using namespace physx;

namespace {
    // ★ PhysXの既定フィルターシェーダーは、パフォーマンスのためキネマティック同士・
    //   キネマティックと静的物体同士の接触を検出しないことがある。このゲームは
    //   Player/Enemyのようなキネマティックなオブジェクト同士の当たり判定(OnCollisionEnter2D等)
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
                | PxPairFlag::eDETECT_DISCRETE_CONTACT;
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

    // PhysXのシミュレーション結果を、GameObjectのOnTriggerEnter2D/OnCollisionEnter2Dなどの
    // コールバックへ変換して届ける。旧・自作2D当たり判定システム(Collider.cpp)が担っていた役割。
    class SimulationEventCallback : public PxSimulationEventCallback {
    public:
        void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
        void onWake(PxActor**, PxU32) override {}
        void onSleep(PxActor**, PxU32) override {}
        void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}

        void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override {
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

                PxContactPairPoint points[4];
                PxU32 n = pair.extractContacts(points, 4);
                PxVec3 normal = (n > 0) ? points[0].normal : PxVec3(0.f, 0.f, 0.f);  // Aから見てBへ向かう向き
                float depth = (n > 0) ? -points[0].separation : 0.f;
                if (depth < 0.f) depth = 0.f;

                CollisionInfo infoA{ colB, { -normal.x, -normal.y }, depth };  // Aから見て、Bから離れる方向
                CollisionInfo infoB{ colA, {  normal.x,  normal.y }, depth };  // Bから見て、Aから離れる方向

                // ★ 押し戻し: キネマティックな側だけを手動でずらす(相手がDynamicでない時に限る)。
                //   相手がDynamicなら、PhysX自身が正しく押し返してくれるのでここでは何もしない
                if (depth > 0.f) {
                    if (kinA && !colA->isStatic && !dynB) {
                        colA->owner->transform.position.x -= normal.x * depth;
                        colA->owner->transform.position.y -= normal.y * depth;
                    }
                    if (kinB && !colB->isStatic && !dynA) {
                        colB->owner->transform.position.x += normal.x * depth;
                        colB->owner->transform.position.y += normal.y * depth;
                    }
                }

                if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
                    colA->owner->OnCollisionEnter2D(infoA);
                    colB->owner->OnCollisionEnter2D(infoB);
                }
                else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
                    colA->owner->OnCollisionStay2D(infoA);
                    colB->owner->OnCollisionStay2D(infoB);
                }
                else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                    colA->owner->OnCollisionExit2D(infoA);
                    colB->owner->OnCollisionExit2D(infoB);
                }
            }
        }

        void onTrigger(PxTriggerPair* pairs, PxU32 count) override {
            for (PxU32 i = 0; i < count; i++) {
                const PxTriggerPair& pair = pairs[i];
                // アクターやシェイプがこのフレームで消えている場合は触らない
                if (pair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER)) continue;

                Collider2D* colTrigger = static_cast<Collider2D*>(pair.triggerActor->userData);
                Collider2D* colOther = static_cast<Collider2D*>(pair.otherActor->userData);
                if (!colTrigger || !colOther) continue;

                if (pair.status & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
                    colTrigger->owner->OnTriggerEnter2D(colOther);
                    colOther->owner->OnTriggerEnter2D(colTrigger);
                }
                else if (pair.status & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                    colTrigger->owner->OnTriggerExit2D(colOther);
                    colOther->owner->OnTriggerExit2D(colTrigger);
                }
            }
        }
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
    static SimulationEventCallback gEventCallback;
    static std::vector<std::pair<PxRigidActor*, bool>> gPendingSceneChanges;  // {アクター, true=追加/false=削除}

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

        gScene = gPhysics->createScene(sceneDesc);
        if (!gScene) { OutputDebugStringA("★ Physics: createScene 失敗\n"); return false; }

        // RigidbodyComponentが特に指定しなければ使う、共通のデフォルト材質
        // (静止摩擦, 動摩擦, 反発係数)
        gDefaultMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.3f);
        if (!gDefaultMaterial) { OutputDebugStringA("★ Physics: createMaterial 失敗\n"); return false; }

        return true;
    }

    void Update(float dt) {
        if (!gScene || dt <= 0.f) return;
        gScene->simulate(dt);
        gScene->fetchResults(true);

        // ★ シミュレーションコールバック(onContact/onTrigger)の中では、PhysXのAPI書き込み
        //   (addActor/removeActorなど)が禁止されている。そこから予約されていた変更を、
        //   fetchResults()が完全に終わった今、安全なタイミングでまとめて反映する
        for (auto& change : gPendingSceneChanges) {
            PxRigidActor* actor = change.first;
            bool add = change.second;
            if (add) gScene->addActor(*actor);
            else gScene->removeActor(*actor);
        }
        gPendingSceneChanges.clear();
    }

    void Uninit() {
        if (gDefaultMaterial) { gDefaultMaterial->release(); gDefaultMaterial = nullptr; }
        if (gScene) { gScene->release(); gScene = nullptr; }
        if (gDispatcher) { gDispatcher->release(); gDispatcher = nullptr; }
        if (gPhysics) { gPhysics->release(); gPhysics = nullptr; }
        if (gFoundation) { gFoundation->release(); gFoundation = nullptr; }
    }

    PxPhysics* GetPhysics() { return gPhysics; }
    PxScene* GetScene() { return gScene; }
    PxMaterial* GetDefaultMaterial() { return gDefaultMaterial; }

    void QueueSceneChange(PxRigidActor* actor, bool add) {
        gPendingSceneChanges.push_back({ actor, add });
    }
}
