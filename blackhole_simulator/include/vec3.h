
#pragma once

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vec3 {
    double x, y, z;

    constexpr Vec3() : x(0.0), y(0.0), z(0.0) {}
    constexpr Vec3(double x, double y, double z) : x(x), y(y), z(z) {}


    constexpr Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    constexpr Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    constexpr Vec3 operator*(double s)      const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(double s)      const { return {x / s, y / s, z / s}; }
    constexpr Vec3 operator-()              const { return {-x, -y, -z}; }

    constexpr Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    constexpr Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    constexpr Vec3& operator*=(double s)      { x *= s;   y *= s;   z *= s;   return *this; }


    constexpr double dot(const Vec3& v)   const { return x * v.x + y * v.y + z * v.z; }
    constexpr Vec3   cross(const Vec3& v) const {
        return {y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x};
    }


    constexpr double length_sq() const { return x * x + y * y + z * z; }
    double           length()    const { return std::sqrt(length_sq()); }

    Vec3 normalized() const {
        double len = length();
        return (len > 1e-12) ? (*this / len) : Vec3{0.0, 0.0, 0.0};
    }
};

constexpr Vec3   operator*(double s, const Vec3& v)          { return v * s; }
constexpr double dot(const Vec3& a, const Vec3& b)           { return a.dot(b); }
constexpr Vec3   cross(const Vec3& a, const Vec3& b)         { return a.cross(b); }
inline    Vec3   normalize(const Vec3& v)                    { return v.normalized(); }
inline    double length(const Vec3& v)                       { return v.length(); }
