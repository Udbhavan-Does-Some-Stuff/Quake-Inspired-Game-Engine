// core/math.hpp
//
// Minimal math library: Vec2/Vec3/Vec4, Mat4, Quat, plus the handful
// of functions a renderer and a movement system actually need
// (perspective, lookAt, transform composition, slerp).
//
// Header-only — these are small, hot-path functions that should be
// inlined, not hidden behind a translation unit boundary.
//
// Note: column-major, right-handed, matching GLSL/OpenGL conventions
// on purpose — it removes a whole class of "why is my matrix
// transposed" bugs when you start writing shaders.

#pragma once

#include "types.hpp"
#include <cmath>

namespace core {

constexpr f32 PI = 3.14159265358979323846f;

inline f32 Radians(f32 degrees) { return degrees * (PI / 180.0f); }
inline f32 Degrees(f32 radians) { return radians * (180.0f / PI); }

// ====================================================================
// Vec2
// ====================================================================
struct Vec2 {
    f32 x = 0.0f, y = 0.0f;

    Vec2() = default;
    Vec2(f32 x_, f32 y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
    Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
    Vec2 operator*(f32 s)         const { return { x * s, y * s }; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
};

// ====================================================================
// Vec3
// ====================================================================
struct Vec3 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}
    explicit Vec3(f32 s) : x(s), y(s), z(s) {}

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator-()              const { return { -x, -y, -z }; }
    Vec3 operator*(f32 s)         const { return { x * s, y * s, z * s }; }
    Vec3 operator/(f32 s)         const { return { x / s, y / s, z / s }; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(f32 s)         { x *= s; y *= s; z *= s; return *this; }
};

inline f32  Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
inline f32  LengthSq(const Vec3& v) { return Dot(v, v); }
inline f32  Length(const Vec3& v)   { return std::sqrt(LengthSq(v)); }
inline Vec3 Normalize(const Vec3& v) {
    f32 len = Length(v);
    if (len < 1e-8f) return { 0.0f, 0.0f, 0.0f };
    return v * (1.0f / len);
}
inline Vec3 Lerp(const Vec3& a, const Vec3& b, f32 t) { return a + (b - a) * t; }

// ====================================================================
// Vec4
// ====================================================================
struct Vec4 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

    Vec4() = default;
    Vec4(f32 x_, f32 y_, f32 z_, f32 w_) : x(x_), y(y_), z(z_), w(w_) {}
    Vec4(const Vec3& v, f32 w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
};

// ====================================================================
// Mat4 — column-major, stored as 4 columns of Vec4
// ====================================================================
struct Mat4 {
    // columns[c] is column c. Element access matches GLSL: m[col][row].
    Vec4 col[4];

    Mat4() = default;

    static Mat4 Identity() {
        Mat4 m;
        m.col[0] = { 1, 0, 0, 0 };
        m.col[1] = { 0, 1, 0, 0 };
        m.col[2] = { 0, 0, 1, 0 };
        m.col[3] = { 0, 0, 0, 1 };
        return m;
    }

    // Raw pointer for uploading to OpenGL via glUniformMatrix4fv.
    const f32* Data() const { return &col[0].x; }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 result;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            f32 sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                const f32* aCol = &a.col[k].x;
                const f32* bCol = &b.col[c].x;
                sum += aCol[r] * bCol[k];
            }
            (&result.col[c].x)[r] = sum;
        }
    }
    return result;
}

inline Mat4 Translate(const Vec3& t) {
    Mat4 m = Mat4::Identity();
    m.col[3] = { t.x, t.y, t.z, 1.0f };
    return m;
}

inline Mat4 Scale(const Vec3& s) {
    Mat4 m = Mat4::Identity();
    m.col[0].x = s.x;
    m.col[1].y = s.y;
    m.col[2].z = s.z;
    return m;
}

// Right-handed rotation about an arbitrary normalized axis, angle in radians.
inline Mat4 RotateAxis(const Vec3& axis, f32 angleRadians) {
    Vec3 a = Normalize(axis);
    f32 c = std::cos(angleRadians);
    f32 s = std::sin(angleRadians);
    f32 t = 1.0f - c;

    Mat4 m = Mat4::Identity();
    m.col[0] = { t*a.x*a.x + c,     t*a.x*a.y + s*a.z, t*a.x*a.z - s*a.y, 0 };
    m.col[1] = { t*a.x*a.y - s*a.z, t*a.y*a.y + c,     t*a.y*a.z + s*a.x, 0 };
    m.col[2] = { t*a.x*a.z + s*a.y, t*a.y*a.z - s*a.x, t*a.z*a.z + c,     0 };
    m.col[3] = { 0, 0, 0, 1 };
    return m;
}

// Right-handed look-at view matrix.
inline Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    Vec3 f = Normalize(target - eye);   // forward
    Vec3 s = Normalize(Cross(f, up));   // right
    Vec3 u = Cross(s, f);               // recomputed up

    Mat4 m = Mat4::Identity();
    m.col[0] = {  s.x,  u.x, -f.x, 0 };
    m.col[1] = {  s.y,  u.y, -f.y, 0 };
    m.col[2] = {  s.z,  u.z, -f.z, 0 };
    m.col[3] = { -Dot(s, eye), -Dot(u, eye), Dot(f, eye), 1 };
    return m;
}

// Right-handed perspective projection, depth range [-1, 1] (OpenGL convention).
inline Mat4 Perspective(f32 fovYRadians, f32 aspect, f32 nearZ, f32 farZ) {
    f32 tanHalfFov = std::tan(fovYRadians * 0.5f);

    Mat4 m{}; // zero-initialized
    m.col[0].x = 1.0f / (aspect * tanHalfFov);
    m.col[1].y = 1.0f / tanHalfFov;
    m.col[2].z = -(farZ + nearZ) / (farZ - nearZ);
    m.col[2].w = -1.0f;
    m.col[3].z = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    return m;
}

// ====================================================================
// Quat — used for camera/entity orientation without gimbal lock
// ====================================================================
struct Quat {
    f32 x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    Quat() = default;
    Quat(f32 x_, f32 y_, f32 z_, f32 w_) : x(x_), y(y_), z(z_), w(w_) {}

    static Quat FromAxisAngle(const Vec3& axis, f32 angleRadians) {
        Vec3 a = Normalize(axis);
        f32 half = angleRadians * 0.5f;
        f32 s = std::sin(half);
        return { a.x * s, a.y * s, a.z * s, std::cos(half) };
    }

    Mat4 ToMat4() const {
        f32 xx = x*x, yy = y*y, zz = z*z;
        f32 xy = x*y, xz = x*z, yz = y*z;
        f32 wx = w*x, wy = w*y, wz = w*z;

        Mat4 m = Mat4::Identity();
        m.col[0] = { 1 - 2*(yy+zz),     2*(xy+wz),     2*(xz-wy), 0 };
        m.col[1] = {     2*(xy-wz), 1 - 2*(xx+zz),     2*(yz+wx), 0 };
        m.col[2] = {     2*(xz+wy),     2*(yz-wx), 1 - 2*(xx+yy), 0 };
        return m;
    }
};

inline Quat Slerp(const Quat& a, const Quat& b, f32 t) {
    f32 dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;

    Quat bb = b;
    if (dot < 0.0f) { // take the shorter path
        bb = { -b.x, -b.y, -b.z, -b.w };
        dot = -dot;
    }

    if (dot > 0.9995f) { // nearly identical — lerp is fine and avoids /0
        Quat r = { a.x + (bb.x-a.x)*t, a.y + (bb.y-a.y)*t,
                   a.z + (bb.z-a.z)*t, a.w + (bb.w-a.w)*t };
        f32 len = std::sqrt(r.x*r.x + r.y*r.y + r.z*r.z + r.w*r.w);
        return { r.x/len, r.y/len, r.z/len, r.w/len };
    }

    f32 theta0 = std::acos(dot);
    f32 theta  = theta0 * t;
    f32 sinTheta0 = std::sin(theta0);
    f32 s0 = std::cos(theta) - dot * std::sin(theta) / sinTheta0;
    f32 s1 = std::sin(theta) / sinTheta0;

    return {
        a.x*s0 + bb.x*s1, a.y*s0 + bb.y*s1,
        a.z*s0 + bb.z*s1, a.w*s0 + bb.w*s1
    };
}

} // namespace core
