#include "type_id_widget.hpp"

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "CppReflection/TypeRegistry.hpp"
#include "integer.hpp"
#include "reflection/eigen_reflect.hpp"
#include "wrap/wrap_eigen.hpp"
#include "wrap/wrap_imgui.hpp"

template <typename T>
bool ScalarProperty(
    edt::GUID type_guid,
    std::string_view name,
    void* address,
    bool& value_changed,
    T min = std::numeric_limits<T>::lowest(),
    T max = std::numeric_limits<T>::max())
{
    constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
    if (type_info.guid == type_guid)
    {
        T* value = reinterpret_cast<T*>(address);
        const bool c = ImGui::DragScalar(name.data(), CastDataType<T>(), value, 1.0f, &min, &max);
        value_changed = c;
        return true;
    }

    return false;
}

template <typename T, int N>
bool VectorProperty(
    std::string_view title,
    Eigen::Matrix<T, N, 1>& value,
    T min = std::numeric_limits<T>::lowest(),
    T max = std::numeric_limits<T>::max()) noexcept
{
    return ImGui::DragScalarN(title.data(), CastDataType<T>(), value.data(), N, 0.01f, &min, &max, "%.3f");
}

template <typename T>
bool VectorProperty(edt::GUID type_guid, std::string_view name, void* address, bool& value_changed)
{
    constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
    if (type_info.guid == type_guid)
    {
        T& member_ref = *reinterpret_cast<T*>(address);
        value_changed |= VectorProperty(name, member_ref);
        return true;
    }

    return false;
}

template <typename T, int C, int R>
bool MatrixProperty(
    const std::string_view title,
    Eigen::Matrix<T, R, C>& value,
    T min = std::numeric_limits<T>::lowest(),
    T max = std::numeric_limits<T>::max()) noexcept
{
    bool changed = false;
    if (ImGui::TreeNode(title.data()))
    {
        for (int row_index = 0; row_index < R; ++row_index)
        {
            auto row_view = value.template block<1, C>(row_index, 0);
            Eigen::Matrix<T, R, 1> row = row_view;
            ImGui::PushID(row_index);
            const bool row_changed =
                ImGui::DragScalarN("", CastDataType<T>(), row.data(), C, 0.01f, &min, &max, "%.3f");
            ImGui::PopID();
            [[unlikely]] if (row_changed)
            {
                row_view = row;
            }
            changed = changed || row_changed;
        }
        ImGui::TreePop();
    }
    return changed;
}

template <typename T>
bool MatrixProperty(edt::GUID type_guid, std::string_view name, void* address, bool& value_changed)
{
    constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
    if (type_info.guid == type_guid)
    {
        T& member_ref = *reinterpret_cast<T*>(address);
        value_changed |= MatrixProperty(name, member_ref);
        return true;
    }

    return false;
}

void SimpleTypeWidget(edt::GUID type_guid, std::string_view name, void* value, bool& value_changed)
{
    value_changed = false;
    [[maybe_unused]] const bool found_type = ScalarProperty<float>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<double>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<ui8>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<ui16>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<ui32>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<ui64>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<i8>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<i16>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<i32>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<i64>(type_guid, name, value, value_changed) ||
                                             VectorProperty<Eigen::Vector2f>(type_guid, name, value, value_changed) ||
                                             VectorProperty<Eigen::Vector3f>(type_guid, name, value, value_changed) ||
                                             VectorProperty<Eigen::Vector4f>(type_guid, name, value, value_changed) ||
                                             MatrixProperty<Eigen::Matrix4f>(type_guid, name, value, value_changed);
}

void TypeIdWidget(edt::GUID type_guid, void* base, bool& value_changed)
{
    const cppreflection::Type* type_info = cppreflection::GetTypeRegistry()->FindType(type_guid);
    for (const cppreflection::Field* field : type_info->GetFields())
    {
        void* pmember = field->GetValue(base);
        bool member_changed = false;
        SimpleTypeWidget(field->GetType()->GetGuid(), field->GetName(), pmember, member_changed);
        value_changed |= member_changed;
    }
}
