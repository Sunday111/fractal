#pragma once

#include <filesystem>
#include <memory>
#include <string>

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

    const std::filesystem::path& GetExecutableDir() const;

private:
    std::unique_ptr<State> state_;
};

}  // namespace klgl