#pragma once

#include <memory>

#include "klgl/wrap/wrap_eigen.hpp"

struct GLFWwindow;

namespace klgl
{

class Window
{
public:
    Window(uint32_t width = 800, uint32_t height = 600);
    ~Window();

    void MakeContextCurrent();

    [[nodiscard]] bool ShouldClose() const noexcept;
    [[nodiscard]] uint32_t GetWidth() const noexcept
    {
        return width_;
    }
    [[nodiscard]] uint32_t GetHeight() const noexcept
    {
        return height_;
    }

    Eigen::Vector2<uint32_t> GetSize() const
    {
        return {width_, height_};
    }
    Eigen::Vector2f GetSize2f() const
    {
        return GetSize().cast<float>();
    }
    [[nodiscard]] GLFWwindow* GetGlfwWindow() const noexcept
    {
        return window_;
    }
    [[nodiscard]] float GetAspect() const noexcept
    {
        return static_cast<float>(GetWidth()) / static_cast<float>(GetHeight());
    }

    void SetSize(size_t width, size_t height);
    void SetTitle(const char* title);

    void SwapBuffers() noexcept;

    bool IsKeyPressed(int key) const;

private:
    static uint32_t MakeWindowId();
    static Window* GetWindow(GLFWwindow* glfw_window) noexcept;

    template <auto method, typename... Args>
    static void CallWndMethod(GLFWwindow* glfw_window, Args&&... args)
    {
        [[likely]] if (Window* window = GetWindow(glfw_window))
        {
            (window->*method)(std::forward<Args>(args)...);
        }
    }

    static void FrameBufferSizeCallback(GLFWwindow* glfw_window, int width, int height);
    static void MouseCallback(GLFWwindow* glfw_window, double x, double y);
    static void MouseButtonCallback(GLFWwindow* glfw_window, int button, int action, int mods);
    static void MouseScrollCallback(GLFWwindow* glfw_window, double x_offset, double y_offset);

    void Create();
    void Destroy();
    void OnResize(int width, int height);
    void OnMouseMove(Eigen::Vector2f new_cursor);
    void OnMouseButton(int button, int action, [[maybe_unused]] int mods);
    void OnMouseScroll([[maybe_unused]] float dx, float dy);

private:
    GLFWwindow* window_ = nullptr;
    Eigen::Vector2f cursor_;
    uint32_t id_;
    uint32_t width_;
    uint32_t height_;
    bool input_mode_ = false;
};

}  // namespace klgl
