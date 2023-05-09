#include "window.hpp"

#include <fmt/format.h>

#include <stdexcept>

#include "GLFW/glfw3.h"

Window::Window(ui32 width, ui32 height) : id_(MakeWindowId()), width_(width), height_(height)
{
    Create();
}

Window::~Window()
{
    Destroy();
}

void Window::MakeContextCurrent()
{
    glfwMakeContextCurrent(window_);
}

bool Window::ShouldClose() const noexcept
{
    return glfwWindowShouldClose(window_);
}

// void Window::ProcessInput(float dt)
// {
//     if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window_, true);
//     const auto forward = dt * camera_->speed * camera_->front;
//     const auto right = camera_->speed * dt * camera_->front.cross(camera_->up);
//     Eigen::Vector2f k{0.0f, 0.0f};
//     if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) k.x() = 1;
//     if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) k.x() = -1;
//     if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) k.y() = -1;
//     if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) k.y() = 1;
//     camera_->eye += forward * k.x() + right * k.y();
// }

void Window::SwapBuffers() noexcept
{
    glfwSwapBuffers(window_);
}

ui32 Window::MakeWindowId()
{
    static ui32 next_id = 0;
    return next_id++;
}

Window* Window::GetWindow(GLFWwindow* glfw_window) noexcept
{
    [[likely]] if (glfw_window)
    {
        void* user_pointer = glfwGetWindowUserPointer(glfw_window);
        [[likely]] if (user_pointer)
        {
            return reinterpret_cast<Window*>(user_pointer);
        }
    }

    return nullptr;
}

void Window::FrameBufferSizeCallback(GLFWwindow* glfw_window, int width, int height)
{
    CallWndMethod<&Window::OnResize>(glfw_window, width, height);
}

void Window::MouseCallback(GLFWwindow* glfw_window, double x, double y)
{
    CallWndMethod<&Window::OnMouseMove>(glfw_window, Eigen::Vector2f{static_cast<float>(x), static_cast<float>(y)});
}

void Window::MouseButtonCallback(GLFWwindow* glfw_window, int button, int action, int mods)
{
    CallWndMethod<&Window::OnMouseButton>(glfw_window, button, action, mods);
}

void Window::MouseScrollCallback(GLFWwindow* glfw_window, double x_offset, double y_offset)
{
    CallWndMethod<&Window::OnMouseScroll>(glfw_window, static_cast<float>(x_offset), static_cast<float>(y_offset));
}

void Window::Create()
{
    window_ = glfwCreateWindow(static_cast<int>(width_), static_cast<int>(height_), "LearnOpenGL", nullptr, nullptr);

    if (!window_)
    {
        throw std::runtime_error(fmt::format("Failed to create window"));
    }

    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, FrameBufferSizeCallback);
    glfwSetCursorPosCallback(window_, MouseCallback);
    glfwSetMouseButtonCallback(window_, MouseButtonCallback);
    glfwSetScrollCallback(window_, MouseScrollCallback);

    double cursor_x;
    double cursor_y;
    glfwGetCursorPos(window_, &cursor_x, &cursor_y);
    cursor_.x() = static_cast<float>(cursor_x);
    cursor_.y() = static_cast<float>(cursor_y);
}

void Window::Destroy()
{
    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
}

void Window::OnResize(int width, int height)
{
    width_ = static_cast<ui32>(width);
    height_ = static_cast<ui32>(height);
}

void Window::OnMouseMove(Eigen::Vector2f new_cursor)
{
    float sensitivity = 0.001f;
    auto delta = (new_cursor - cursor_) * sensitivity;
    cursor_ = new_cursor;
}

void Window::OnMouseButton(int button, int action, [[maybe_unused]] int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_RIGHT:
        if (action == GLFW_PRESS)
        {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            input_mode_ = true;
        }
        else if (action == GLFW_RELEASE)
        {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            input_mode_ = false;
        }
        break;
    }
}

void Window::OnMouseScroll([[maybe_unused]] float dx, [[maybe_unused]] float dy) {}