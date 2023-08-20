// Using the code of Ray Tracing in One Weekend
// https://raytracing.github.io/books/RayTracingInOneWeekend.html
#pragma once

#include <math.h>
#include <stdlib.h>
#include <iostream>
#include "util.h"

class Vec3 {
public:
    Vec3() {}

    Vec3(float e0, float e1, float e2) {
      e_[0] = e0;
      e_[1] = e1;
      e_[2] = e2;
    }

    inline float X() const { return e_[0]; }

    inline float Y() const { return e_[1]; }

    inline float Z() const { return e_[2]; }

    inline const Vec3 &operator+() const { return *this; }

    inline Vec3 operator-() const { return Vec3(-e_[0], -e_[1], -e_[2]); }

    inline float operator[](int i) const { return e_[i]; }

    inline float &operator[](int i) { return e_[i]; };

    inline Vec3 &operator+=(const Vec3 &v2);

    inline Vec3 &operator-=(const Vec3 &v2);

    inline Vec3 &operator*=(const Vec3 &v2);

    inline Vec3 &operator/=(const Vec3 &v2);

    inline Vec3 &operator*=(const float t);

    inline Vec3 &operator/=(const float t);

    float Length() const {
      return sqrt(e_[0] * e_[0] + e_[1] * e_[1] + e_[2] * e_[2]);
    }

    float SquaredLength() const {
      return e_[0] * e_[0] + e_[1] * e_[1] + e_[2] * e_[2];
    }

    bool NearZero() const {
      const auto s = 1e-8;
      return (fabs(e_[0]) < s) && (fabs(e_[1]) < s) && (fabs(e_[2]) < s);
    }

    inline static Vec3 Rand() {
      return Vec3(rand(), rand(), rand());
    }

    inline static Vec3 Rand(float min, float max) {
      return Vec3(RandDouble(min, max), RandDouble(min, max), RandDouble(min, max));
    }

    float e_[3];
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
  return Vec3(v1.e_[0] + v2.e_[0], v1.e_[1] + v2.e_[1], v1.e_[2] + v2.e_[2]);
}

inline Vec3 operator-(const Vec3 &v1, const Vec3 &v2) {
  return Vec3(v1.e_[0] - v2.e_[0], v1.e_[1] - v2.e_[1], v1.e_[2] - v2.e_[2]);
}

inline Vec3 operator*(const Vec3 &v1, const Vec3 &v2) {
  return Vec3(v1.e_[0] * v2.e_[0], v1.e_[1] * v2.e_[1], v1.e_[2] * v2.e_[2]);
}

inline Vec3 operator/(const Vec3 &v1, const Vec3 &v2) {
  return Vec3(v1.e_[0] / v2.e_[0], v1.e_[1] / v2.e_[1], v1.e_[2] / v2.e_[2]);
}

inline Vec3 operator*(float t, const Vec3 &v) {
  return Vec3(t * v.e_[0], t * v.e_[1], t * v.e_[2]);
}

inline Vec3 operator/(Vec3 v, float t) {
  return Vec3(v.e_[0] / t, v.e_[1] / t, v.e_[2] / t);
}

inline Vec3 operator*(const Vec3 &v, float t) {
  return Vec3(t * v.e_[0], t * v.e_[1], t * v.e_[2]);
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
  float k = 1.0 / t;

  e_[0] *= k;
  e_[1] *= k;
  e_[2] *= k;
  return *this;
}

inline float Dot(const Vec3 &v1, const Vec3 &v2) {
  return v1.e_[0] * v2.e_[0] + v1.e_[1] * v2.e_[1] + v1.e_[2] * v2.e_[2];
}

inline Vec3 Cross(const Vec3 &v1, const Vec3 &v2) {
  return Vec3((v1.e_[1] * v2.e_[2] - v1.e_[2] * v2.e_[1]),
              (-v1.e_[0] * v2.e_[2] + v1.e_[2] * v2.e_[0]),
              (v1.e_[0] * v2.e_[1] - v1.e_[1] * v2.e_[0]));
}

inline Vec3 Unit(Vec3 v) {
  return v / v.Length();
}

inline Vec3 Inv(Vec3 v) {
  const float s = 1 / v.SquaredLength();
  return v * s;
}

inline Vec3 RandInUnitDisk() {
  while (true) {
    auto p = Vec3(RandDouble(-1, 1), RandDouble(-1, 1), 0);
    if (p.SquaredLength() >= 1) continue;
    return p;
  }
}

inline Vec3 RandInUnitSphere() {
  while (true) {
    auto p = Vec3::Rand(-1, 1);
    if (p.SquaredLength() >= 1) continue;
    return p;
  }
}

inline Vec3 RandUnit() {
  return Unit(RandInUnitSphere());
}

inline Vec3 RandInHemisphere(const Vec3 &normal) {
  Vec3 in_unit_sphere = RandInUnitSphere();
  if (Dot(in_unit_sphere, normal) > 0.0) {
    return in_unit_sphere;
  } else {
    return -in_unit_sphere;
  }
}

using Point3 = Vec3;   // 位置座標
using Color3 = Vec3;    // カラー

static const Vec3 ZERO_Vec3(0.0, 0.0, 0.0);
static const Vec3 X_UP(1.0, 0.0, 0.0);
static const Vec3 Y_UP(0.0, 1.0, 0.0);
static const Vec3 Z_UP(0.0, 0.0, 1.0);