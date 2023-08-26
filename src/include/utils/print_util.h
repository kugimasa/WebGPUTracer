#pragma once

#include <iostream>
#include <string>

enum class PrintInfoType {
    WebGPU,
    GLFW,
    WebGPUTracer,
};

std::string inline GetInfoTypeStr(PrintInfoType info_type) {
  switch (info_type) {
    case PrintInfoType::WebGPU:
      return "WebGPU";
    case PrintInfoType::GLFW:
      return "GLFW";
    case PrintInfoType::WebGPUTracer:
      return "WebGPUTracer";
    default:
      return "";
  }
}

void inline Print(const PrintInfoType info_type, const char *message) {
  std::cout << "[" << GetInfoTypeStr(info_type) << "] " << message << std::endl;
}

template<typename Any>
void inline Print(const PrintInfoType info_type, const char *message, Any any) {
  std::cout << "[" << GetInfoTypeStr(info_type) << "] " << message << any << std::endl;
}

void inline Error(const PrintInfoType info_type, const char *message) {
  std::cerr << "[" << GetInfoTypeStr(info_type) << "] " << message << std::endl;
}

template<typename Any>
void inline Error(const PrintInfoType info_type, const char *message, Any any) {
  std::cerr << "[" << GetInfoTypeStr(info_type) << "] " << message << any << std::endl;
}
