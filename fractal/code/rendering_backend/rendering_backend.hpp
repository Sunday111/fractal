#pragma once

#include <memory>

class FractalRenderingBackend
{
public:
    virtual void Draw() = 0;
    virtual void DrawSettings();
    virtual void PostDraw() {}

    virtual ~FractalRenderingBackend() = default;
};
