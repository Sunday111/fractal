#pragma once

#include <memory>

class Window;

namespace klgl
{
class Application
{
    struct State;

public:
    Application();
    virtual ~Application();

    virtual void Initialize();
    virtual void Run();
    virtual void PreTick();
    virtual void Tick();
    virtual void PostTick();
    virtual void MainLoop();
    virtual void InitializeReflectionTypes();

    Window& GetWindow();

private:
    std::unique_ptr<State> state_;
};

}  // namespace klgl