#include "renderer.h"

int main() {
  Print(PrintInfoType::Portracer, "Starting Portracer (_)=---=(_)");
  Renderer renderer;
  renderer.OnInit();

  while (renderer.IsRunning()) {
    renderer.OnFrame();
  }

  renderer.OnFinish();
  return 0;
}
