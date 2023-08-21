#include "renderer.h"

int main() {
  Print(PrintInfoType::Portracer, "Starting Portracer (_)=---=(_)");
  Renderer renderer;
  bool hasWindow = false;
  renderer.OnInit(hasWindow);

  // RenderPipeline
  if (hasWindow) {
    while (renderer.IsRunning()) {
      renderer.OnFrame();
    }
  }

  // ComputePipeline
  if (!renderer.OnCompute()) {
    Error(PrintInfoType::Portracer, "(_)=--.. Something went wrong");
    return 1;
  }

  renderer.OnFinish();
  Print(PrintInfoType::Portracer, "(_)=---=(_) Portracer Finished");
  return 0;
}
