#include "renderer.h"

int main(int argc, char *argv[]) {
  Print(PrintInfoType::WebGPUTracer, "Starting WebGPUTracer (_)=---=(_)");
  Renderer renderer;
  bool hasWindow = true;
  bool isCompute = false;
  renderer.OnInit(hasWindow);

  // RenderPipeline
  if (hasWindow) {
    while (renderer.IsRunning()) {
      renderer.OnFrame();
    }
  }

  uint32_t start_frame = 1;
  uint32_t end_frame = 1;
  // コマンドライン入力形式
  // ./WebGPUTracer.exe --frame [start] [end]
  if (argc == 4) {
    if (strcmp(argv[1], "--frame") == 0) {
      start_frame = (uint32_t) atoi(argv[2]);
      end_frame = (uint32_t) atoi(argv[3]);
    }
  }

  if (isCompute) {
    // ComputePipeline
    if (!renderer.OnCompute(start_frame, end_frame)) {
      Error(PrintInfoType::WebGPUTracer, "(_)=--.. Something went wrong");
      return 1;
    }
  }

  renderer.OnFinish();
  Print(PrintInfoType::WebGPUTracer, "(_)=---=(_) WebGPUTracer Finished");
  return 0;
}
