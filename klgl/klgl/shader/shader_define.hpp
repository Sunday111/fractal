#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "EverydayTools/GUID.hpp"
#include "klgl/integer.hpp"
#include "klgl/name_cache/name.hpp"
#include "nlohmann/json.hpp"

class ShaderDefine
{
public:
    ShaderDefine(const ShaderDefine&) = delete;
    ShaderDefine(ShaderDefine&& another);
    void MoveFrom(ShaderDefine& another);
    ShaderDefine& operator=(const ShaderDefine&) = delete;
    ShaderDefine& operator=(ShaderDefine&& another);

    std::string GenDefine() const;
    void SetValue(std::span<const ui8> value_view);

    static ShaderDefine ReadFromJson(const nlohmann::json& json);

protected:
    ShaderDefine() = default;

public:
    std::vector<ui8> value;
    Name name;
    edt::GUID type_guid;
};
