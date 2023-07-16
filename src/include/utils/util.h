#pragma once

#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <vector>
#include <array>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include <glm.hpp>
#include <ext.hpp>


using std::shared_ptr;
using std::make_shared;
using std::sqrt;
using glm::mat4x4;
using glm::vec3;

constexpr double INF = std::numeric_limits<double>::infinity();

inline double Deg2Rad(double degrees) {
  // 1 / 180.0 = 0.00555555555
  return degrees * M_PI * 0.00555555555;
}

inline double Clamp(double x, double min, double max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
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