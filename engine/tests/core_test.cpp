// tests/core_test.cpp
//
// Exercises every part of the core module. Not a unit test framework —
// just enough to catch obvious breakage as you keep building.

#include "../core/core.hpp"
#include <cstdio>

using namespace core;

static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("  [FAIL] %s  (line %d)\n", #cond, __LINE__);      \
            ++g_failures;                                                  \
        } else {                                                           \
            std::printf("  [ OK ] %s\n", #cond);                          \
        }                                                                  \
    } while (0)

static bool NearlyEqual(f32 a, f32 b, f32 eps = 1e-4f) {
    return std::fabs(a - b) < eps;
}

void TestLog() {
    std::printf("\n-- Log --\n");
    LOG_DEBUG("Test", "debug message, value=%d", 42);
    LOG_INFO("Test", "info message");
    LOG_WARN("Test", "warn message");
    LOG_ERROR("Test", "error message");
    std::printf("  (visual check — no crash means pass)\n");
}

void TestLinearAllocator() {
    std::printf("\n-- LinearAllocator --\n");
    LinearAllocator arena(1024);

    void* a = arena.Allocate(64);
    void* b = arena.Allocate(128);
    CHECK(a != nullptr);
    CHECK(b != nullptr);
    CHECK(a != b);
    CHECK(arena.BytesUsed() >= 192);

    arena.Reset();
    CHECK(arena.BytesUsed() == 0);

    // Exhaustion should return nullptr, not crash.
    void* huge = arena.Allocate(2048);
    CHECK(huge == nullptr);

    // Typed allocation + placement construction.
    struct Foo { int x; Foo(int v) : x(v) {} };
    Foo* foo = arena.AllocateObject<Foo>(99);
    CHECK(foo != nullptr);
    CHECK(foo->x == 99);
}

void TestPoolAllocator() {
    std::printf("\n-- PoolAllocator --\n");
    PoolAllocator pool(sizeof(int), 4);
    CHECK(pool.BlockCount() == 4);

    void* a = pool.Allocate();
    void* b = pool.Allocate();
    void* c = pool.Allocate();
    void* d = pool.Allocate();
    void* e = pool.Allocate(); // pool full

    CHECK(a && b && c && d);
    CHECK(e == nullptr);
    CHECK(pool.BlocksInUse() == 4);

    pool.Free(b);
    CHECK(pool.BlocksInUse() == 3);

    void* reused = pool.Allocate();
    CHECK(reused == b); // free list should hand back the just-freed block
}

void TestVec3() {
    std::printf("\n-- Vec3 --\n");
    Vec3 a(1, 0, 0);
    Vec3 b(0, 1, 0);

    CHECK(NearlyEqual(Dot(a, b), 0.0f));
    Vec3 c = Cross(a, b);
    CHECK(NearlyEqual(c.x, 0) && NearlyEqual(c.y, 0) && NearlyEqual(c.z, 1));

    Vec3 v(3, 4, 0);
    CHECK(NearlyEqual(Length(v), 5.0f));

    Vec3 n = Normalize(v);
    CHECK(NearlyEqual(Length(n), 1.0f));

    Vec3 lerped = Lerp(Vec3(0,0,0), Vec3(10,0,0), 0.5f);
    CHECK(NearlyEqual(lerped.x, 5.0f));
}

void TestMat4() {
    std::printf("\n-- Mat4 --\n");
    Mat4 id = Mat4::Identity();
    Mat4 t = Translate(Vec3(1, 2, 3));
    Mat4 result = id * t;

    CHECK(NearlyEqual(result.col[3].x, 1.0f));
    CHECK(NearlyEqual(result.col[3].y, 2.0f));
    CHECK(NearlyEqual(result.col[3].z, 3.0f));

    // Rotating 90 degrees about Y should send +X toward -Z (right-handed).
    Mat4 rot = RotateAxis(Vec3(0, 1, 0), Radians(90.0f));
    // Transform point (1,0,0) by rot manually (since we don't have a
    // Mat4 * Vec4 helper yet — left as an exercise for the render module).
    f32 x = rot.col[0].x * 1.0f;
    f32 z = rot.col[0].z * 1.0f;
    CHECK(NearlyEqual(x, 0.0f, 1e-3f));
    CHECK(NearlyEqual(z, -1.0f, 1e-3f));

    Mat4 view = LookAt(Vec3(0,0,5), Vec3(0,0,0), Vec3(0,1,0));
    (void)view; // visual/structural check only for now
    std::printf("  (LookAt constructed without crashing)\n");

    Mat4 proj = Perspective(Radians(60.0f), 16.0f/9.0f, 0.1f, 100.0f);
    CHECK(NearlyEqual(proj.col[3].w, 0.0f));
}

void TestQuat() {
    std::printf("\n-- Quat --\n");
    Quat identity;
    CHECK(NearlyEqual(identity.w, 1.0f));

    Quat q = Quat::FromAxisAngle(Vec3(0,1,0), Radians(90.0f));
    Mat4 m = q.ToMat4();
    f32 x = m.col[0].x * 1.0f;
    f32 z = m.col[0].z * 1.0f;
    CHECK(NearlyEqual(x, 0.0f, 1e-3f));
    CHECK(NearlyEqual(z, -1.0f, 1e-3f));

    Quat halfway = Slerp(identity, q, 0.5f);
    f32 len = std::sqrt(halfway.x*halfway.x + halfway.y*halfway.y +
                         halfway.z*halfway.z + halfway.w*halfway.w);
    CHECK(NearlyEqual(len, 1.0f, 1e-3f));
}

int main() {
    std::printf("=== core module test ===\n");

    TestLog();
    TestLinearAllocator();
    TestPoolAllocator();
    TestVec3();
    TestMat4();
    TestQuat();

    std::printf("\n=== %s (%d failures) ===\n",
                g_failures == 0 ? "ALL PASS" : "FAILURES FOUND", g_failures);

    return g_failures == 0 ? 0 : 1;
}
