// Using the code of Ray Tracing in One Weekend
// https://raytracing.github.io/books/RayTracingInOneWeekend.html
////// NOT USED //////
// Using glm instead
#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>

class Vec3 {
public:
    Vec3() = default;

    Vec3(float e0, float e1, float e2) {
      e_[0] = e0;
      e_[1] = e1;
      e_[2] = e2;
    }

    [[nodiscard]] inline float X() const { return e_[0]; }

    [[nodiscard]] inline float Y() const { return e_[1]; }

    [[nodiscard]] inline float Z() const { return e_[2]; }

    inline const Vec3 &operator+() const { return *this; }

    inline Vec3 operator-() const { return Vec3{-e_[0], -e_[1], -e_[2]}; }

    inline float operator[](int i) const { return e_[i]; }

    inline float &operator[](int i) { return e_[i]; };

    inline Vec3 &operator+=(const Vec3 &v2);

    inline Vec3 &operator-=(const Vec3 &v2);

    inline Vec3 &operator*=(const Vec3 &v2);

    inline Vec3 &operator/=(const Vec3 &v2);

    inline Vec3 &operator*=(float t);

    inline Vec3 &operator/=(float t);

    [[nodiscard]] float Length() const {
      return sqrt(e_[0] * e_[0] + e_[1] * e_[1] + e_[2] * e_[2]);
    }

    float e_[3]{};
};

inline std::istream &operator>>(std::istream &is, Vec3 &t) {
  is >> t.e_[0] >> t.e_[1] >> t.e_[2];
  return is;
}

inline std::ostream &operator<<(std::ostream &os, const Vec3 &t) {
  os << t.e_[0] << " " << t.e_[1] << " " << t.e_[2];
  return os;
}

inline Vec3 operator+(const Vec3 &v1, const Vec3 &v2) {
  return Vec3{v1.e_[0] + v2.e_[0], v1.e_[1] + v2.e_[1], v1.e_[2] + v2.e_[2]};
}

inline Vec3 operator-(const Vec3 &v1, const Vec3 &v2) {
  return Vec3{v1.e_[0] - v2.e_[0], v1.e_[1] - v2.e_[1], v1.e_[2] - v2.e_[2]};
}

inline Vec3 operator*(const Vec3 &v1, const Vec3 &v2) {
  return Vec3{v1.e_[0] * v2.e_[0], v1.e_[1] * v2.e_[1], v1.e_[2] * v2.e_[2]};
}

inline Vec3 operator/(const Vec3 &v1, const Vec3 &v2) {
  return Vec3{v1.e_[0] / v2.e_[0], v1.e_[1] / v2.e_[1], v1.e_[2] / v2.e_[2]};
}

inline Vec3 operator*(float t, const Vec3 &v) {
  return Vec3{t * v.e_[0], t * v.e_[1], t * v.e_[2]};
}

inline Vec3 operator/(Vec3 v, float t) {
  return Vec3{v.e_[0] / t, v.e_[1] / t, v.e_[2] / t};
}

inline Vec3 operator*(const Vec3 &v, float t) {
  return Vec3{t * v.e_[0], t * v.e_[1], t * v.e_[2]};
}

inline Vec3 &Vec3::operator+=(const Vec3 &v) {
  e_[0] += v.e_[0];
  e_[1] += v.e_[1];
  e_[2] += v.e_[2];
  return *this;
}

inline Vec3 &Vec3::operator*=(const Vec3 &v) {
  e_[0] *= v.e_[0];
  e_[1] *= v.e_[1];
  e_[2] *= v.e_[2];
  return *this;
}

inline Vec3 &Vec3::operator/=(const Vec3 &v) {
  e_[0] /= v.e_[0];
  e_[1] /= v.e_[1];
  e_[2] /= v.e_[2];
  return *this;
}

inline Vec3 &Vec3::operator-=(const Vec3 &v) {
  e_[0] -= v.e_[0];
  e_[1] -= v.e_[1];
  e_[2] -= v.e_[2];
  return *this;
}

inline Vec3 &Vec3::operator*=(const float t) {
  e_[0] *= t;
  e_[1] *= t;
  e_[2] *= t;
  return *this;
}

inline Vec3 &Vec3::operator/=(const float t) {
  float k = 1.0f / t;

  e_[0] *= k;
  e_[1] *= k;
  e_[2] *= k;
  return *this;
}

inline float Dot(const Vec3 &v1, const Vec3 &v2) {
  return v1.e_[0] * v2.e_[0] + v1.e_[1] * v2.e_[1] + v1.e_[2] * v2.e_[2];
}

inline Vec3 Cross(const Vec3 &v1, const Vec3 &v2) {
  return Vec3{(v1.e_[1] * v2.e_[2] - v1.e_[2] * v2.e_[1]),
              (-v1.e_[0] * v2.e_[2] + v1.e_[2] * v2.e_[0]),
              (v1.e_[0] * v2.e_[1] - v1.e_[1] * v2.e_[0])};
}

inline Vec3 Unit(Vec3 v) {
  return v / v.Length();
}

using Point3 = Vec3;   // Position
using Color3 = Vec3;    // Color

static const Vec3 ZERO_Vec3(0.0, 0.0, 0.0);