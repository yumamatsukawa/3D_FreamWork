#include "RenderStats.h"

namespace RenderStats {
    namespace {
        // 種類ごとの数。current = 数えている途中のフレーム、last = 前のフレーム(Getが返す)
        struct Counter {
            int drawCalls = 0, drawTriangles = 0;
            int shadowCalls = 0, shadowTriangles = 0;
            int outlineCalls = 0, outlineTriangles = 0;
            int lodCulled = 0;
        };
        Counter current;
        Counter last;
    }

    void BeginFrame() {
        last = current;
        current = Counter();
    }

    void AddDraw(int triangleCount) {
        current.drawCalls++;
        current.drawTriangles += triangleCount;
    }

    void AddShadowDraw(int triangleCount) {
        current.shadowCalls++;
        current.shadowTriangles += triangleCount;
    }

    void AddOutlineDraw(int triangleCount) {
        current.outlineCalls++;
        current.outlineTriangles += triangleCount;
    }

    void AddLodCulled() {
        current.lodCulled++;
    }

    int GetDrawCallCount() { return last.drawCalls; }
    int GetDrawTriangleCount() { return last.drawTriangles; }
    int GetShadowDrawCount() { return last.shadowCalls; }
    int GetShadowTriangleCount() { return last.shadowTriangles; }
    int GetOutlineDrawCount() { return last.outlineCalls; }
    int GetOutlineTriangleCount() { return last.outlineTriangles; }
    int GetDrawCallTotal() { return last.drawCalls + last.shadowCalls + last.outlineCalls; }
    int GetTriangleCount() { return last.drawTriangles + last.shadowTriangles + last.outlineTriangles; }
    int GetLodCulledCount() { return last.lodCulled; }
}
