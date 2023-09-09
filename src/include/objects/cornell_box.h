#pragma once

#include "utils/wgpu_util.h"
#include "quad.h"

class CornellBox {
public:
    CornellBox();

    void PushToQuads(std::vector<Quad> &quads);

public:
    std::vector<Quad> quads_;
};
