#include "MathUtil.h"
#include <random>
#include <utility>

namespace {
    // 乱数の元。最初に使われた時に1回だけ、random_deviceの値で初期化する(毎回違う並びになる)
    std::mt19937& GetRandomEngine() {
        static std::mt19937 engine{ std::random_device{}() };
        return engine;
    }
}

namespace Math {
    float RandomRange(float minValue, float maxValue) {
        // 逆に渡されても動くように入れ替える(distributionはmin<=maxでないといけない)
        if (minValue > maxValue) std::swap(minValue, maxValue);
        std::uniform_real_distribution<float> dist(minValue, maxValue);
        return dist(GetRandomEngine());
    }

    int RandomRange(int minValue, int maxValue) {
        if (minValue > maxValue) std::swap(minValue, maxValue);
        std::uniform_int_distribution<int> dist(minValue, maxValue);   // maxValueも含む
        return dist(GetRandomEngine());
    }

    XMFLOAT2 RandomOnCircle(float radius) {
        float angle = RandomRange(0.f, XM_2PI);
        return { cosf(angle) * radius, sinf(angle) * radius };
    }
}
