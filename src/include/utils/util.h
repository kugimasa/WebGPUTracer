#pragma once

#define _USE_MATH_DEFINES

#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <vector>
#include <array>
#include <random>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include <glm.hpp>
#include <ext.hpp>

using std::shared_ptr;
using std::make_shared;
using std::sqrt;
using glm::vec3;
using glm::vec4;
using glm::mat4x4;

constexpr double INF = std::numeric_limits<double>::infinity();

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline float Deg2Rad(float degrees) {
  // 1 / 180.0 = 0.00555555555
  return (float) (degrees * M_PI * 0.00555555555);
}

inline float Clamp(float x, float min, float max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

inline uint32_t RandSeed() {
  std::mt19937 mt;
  std::random_device rnd;
  return rnd();
}

// [0,1)の値をランダムで返す
inline float Rand() {
  return (float) rand() / ((float) RAND_MAX + 1.0f);
}

// [0,1)の値をランダムで返す
inline double RandDouble() {
  return rand() / (RAND_MAX + 1.0);
}

// [min,max)のdoubleをランダムで返す
inline double RandDouble(double min, double max) {
  return min + (max - min) * RandDouble();
}

// [min,max)のintをランダムで返す
inline int RandInt(int min, int max) {
  return static_cast<int>(RandDouble(min, max + 1));
}

// 線形補間
inline float Lerp(float a, float b, float t) {
  return a + t * (b - a);
}

inline float EaseInQuart(float t) {
  return t * t * t * t;
}

inline float EaseOutCubic(float t) {
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

inline float EaseInOutExpo(float t) {
  if (t == 0 || t == 1) {
    return t;
  }
  return t < 0.5 ? pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

inline float Sigmoid(float t) {
  return 3.0f * t * t - 2.0f * t * t * t;
}

using Point3 = vec3; // Position
using Color3 = vec3; // Color