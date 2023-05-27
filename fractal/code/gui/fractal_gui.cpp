#include "fractal_gui.hpp"

#include <random>

#include "float.hpp"
#include "imgui.h"

void FractalGUI::Draw(float dt)
{
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
        {
            auto imgui_cursor = ImGui::GetMousePos();
            settings.MoveCameraToPixel(static_cast<size_t>(imgui_cursor.x), static_cast<size_t>(imgui_cursor.y), true);
        }

        if (ImGui::IsKeyDown(ImGuiKey_E)) settings.IncrementScale();
        if (ImGui::IsKeyDown(ImGuiKey_Q)) settings.DecrementScale();
        FractalSettings::PanCameraOpts pan_opts{.dt = dt};
        if (ImGui::IsKeyDown(ImGuiKey_W)) pan_opts.dir_y = 1;
        if (ImGui::IsKeyDown(ImGuiKey_S)) pan_opts.dir_y = -1;
        if (ImGui::IsKeyDown(ImGuiKey_D)) pan_opts.dir_x = 1;
        if (ImGui::IsKeyDown(ImGuiKey_A)) pan_opts.dir_x = -1;
        settings.PanCamera(pan_opts);
    }

    std::string tmp;
    auto float_input = [&](const char* title, const Float& v) -> std::optional<Float>
    {
        tmp = v.str(std::numeric_limits<double>::digits10 + 3, std::ios_base::fixed);
        tmp.resize(1000);
        if (ImGui::InputText(title, tmp.data(), 1000))
        {
            return Float(tmp.c_str());
        }

        return std::nullopt;
    };

    if (ImGui::CollapsingHeader("Camera"))
    {
        if (auto maybe_x = float_input("x", settings.GetCamera().x()))
        {
            auto new_camera = settings.GetCamera();
            new_camera.x() = maybe_x.value();
            settings.SetCamera(new_camera);
        }

        if (auto maybe_y = float_input("y", settings.GetCamera().y()))
        {
            auto new_camera = settings.GetCamera();
            new_camera.y() = maybe_y.value();
            settings.SetCamera(new_camera);
        }

        int zoom = static_cast<int>(settings.GetZoom());
        if (ImGui::InputInt("Zoom", &zoom))
        {
            zoom = std::max(0, zoom);
            settings.SetZoom(static_cast<uint16_t>(zoom));
        }
    }

    if (ImGui::CollapsingHeader("Colors"))
    {
        bool has_changes = false;
        for (size_t color_index = 0; color_index != settings.colors.size(); ++color_index)
        {
            if (color_index)
            {
                ImGui::SameLine();
            }

            constexpr int color_edit_flags =
                ImGuiColorEditFlags_DefaultOptions_ | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
            auto& color = settings.colors[color_index];
            ImGui::PushID(&color);
            has_changes |= ImGui::ColorEdit3("Color", color.data(), color_edit_flags);
            ImGui::PopID();
        }

        ImGui::InputInt("Color Seed: 1234", &settings.color_seed);
        ImGui::SameLine();
        if (ImGui::Button("Randomize"))
        {
            std::mt19937 rnd(static_cast<unsigned>(settings.color_seed));
            std::uniform_real_distribution<float> color_distr(0, 1.0f);
            for (auto& color : settings.colors)
            {
                for (float& v : color)
                {
                    v = color_distr(rnd);
                }
            }
            has_changes = true;
        }

        if (has_changes)
        {
            settings.settings_applied = false;
        }
    }
}
