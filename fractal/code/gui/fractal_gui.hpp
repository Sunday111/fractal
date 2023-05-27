#pragma once

#include <string>

#include "fractal_settings.hpp"

class FractalGUI
{
public:
    FractalSettings& settings;
    void Draw(float dt);
};
