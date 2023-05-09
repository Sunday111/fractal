#include "application.hpp"

#include <chrono>

#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/opengl/debug/gl_debug_messenger.hpp"
#include "klgl/os/os.hpp"
#include "klgl/reflection/register_types.hpp"
#include "klgl/window.hpp"
#include "klgl/wrap/wrap_glfw.hpp"
#include "klgl/wrap/wrap_imgui.hpp"

namespace klgl
{

struct Application::State
{
    GlfwState glfw_;
    std::unique_ptr<Window> window_;
    std::filesystem::path executable_dir_;
};

Application::Application()
{
    state_ = std::make_unique<State>();
}

Application::~Application() = default;

int InitializeGLAD_impl()
{
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    return 42;
}

void InitializeGLAD()
{
    [[maybe_unused]] static int once = InitializeGLAD_impl();
}

void Application::Initialize()
{
    state_->executable_dir_ = os::GetExecutableDir();

    InitializeReflectionTypes();

    state_->glfw_.Initialize();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    {
        uint32_t window_width = 800;
        uint32_t window_height = 800;
        if (GLFWmonitor* monitor = glfwGetPrimaryMonitor())
        {
            float x_scale = 0.f, y_scale = 0.f;
            glfwGetMonitorContentScale(monitor, &x_scale, &y_scale);
            window_width = static_cast<uint32_t>(static_cast<float>(window_width) * x_scale);
            window_height = static_cast<uint32_t>(static_cast<float>(window_height) * y_scale);
        }

        state_->window_ = std::make_unique<Window>(window_width, window_height);
    }

    // GLAD can be initialized only when glfw has window context
    state_->window_->MakeContextCurrent();
    InitializeGLAD();
    GlDebugMessenger::Start();

    glfwSwapInterval(0);
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(state_->window_->GetGlfwWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor())
    {
        float xscale, yscale;
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);

        ImGui::GetStyle().ScaleAllSizes(2);
        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig font_config{};
        font_config.SizePixels = 13 * xscale;
        io.Fonts->AddFontDefault(&font_config);
    }
}

void Application::Run()
{
    Initialize();
    MainLoop();
}

void Application::PreTick()
{
    OpenGl::Viewport(
        0,
        0,
        static_cast<GLint>(state_->window_->GetWidth()),
        static_cast<GLint>(state_->window_->GetHeight()));

    OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::Tick() {}

void Application::PostTick()
{
    {
        ScopeAnnotation imgui_render("ImGUI");
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    state_->window_->SwapBuffers();
    glfwPollEvents();
}

void Application::MainLoop()
{
    auto prev_frame_time = std::chrono::high_resolution_clock::now();
    while (!state_->window_->ShouldClose())
    {
        ScopeAnnotation frame_annotation("Frame");
        const auto current_frame_time = std::chrono::high_resolution_clock::now();
        // const auto frame_delta_time =
        //      std::chrono::duration<float, std::chrono::seconds::period>(current_frame_time -
        //      prev_frame_time).count();

        PreTick();
        Tick();
        PostTick();

        prev_frame_time = current_frame_time;
    }
}

void Application::InitializeReflectionTypes()
{
    RegisterReflectionTypes();
}

Window& Application::GetWindow()
{
    return *state_->window_;
}

const std::filesystem::path& Application::GetExecutableDir() const
{
    return state_->executable_dir_;
}

}  // namespace klgl