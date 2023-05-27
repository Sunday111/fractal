#include "rendering_backend/rendering_backend_cpu.hpp"

#include <string>

#include "fmt/format.h"
#include "imgui.h"

void FractalRenderingBackendCPU::DrawSettings()
{
    if (!ImGui::CollapsingHeader("CPU rendering backend"))
    {
        return;
    }

    bool settings_changed = false;

    if (ImGui::Checkbox("Use regualr double", &settings_.use_double))
    {
        settings_changed = true;
    }

    auto opt_prev_frame_duration = TakePreviousFrameDuration();
    if (opt_prev_frame_duration.has_value())
    {
        cpu_frames_durations_.push_back(*opt_prev_frame_duration);
    }

    std::string tmp;
    if (ImGui::BeginListBox("Frame durations"))
    {
        for (float frame_duration : cpu_frames_durations_)
        {
            tmp.clear();
            fmt::format_to(std::back_inserter(tmp), "{}s", frame_duration);
            bool selected = false;
            ImGui::Selectable(tmp.c_str(), &selected);
        }
        ImGui::EndListBox();
    }

    if (ImGui::CollapsingHeader("Tasks progress"))
    {
        ForEachTask(
            [](const ThreadTask& task)
            {
                const size_t completed = task.rows_completed;
                const size_t total = task.region_screen_size.y();
                const float progress = static_cast<float>(completed) / static_cast<float>(total);
                ImGui::ProgressBar(progress);
            });
    }

    if (settings_changed)
    {
        settings_.settings_applied = false;
    }
}
