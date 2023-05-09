#pragma once

#include <string_view>

#include "EverydayTools/GUID.hpp"
#include "klgl/integer.hpp"

void SimpleTypeWidget(edt::GUID type_guid, std::string_view name, void* value, bool& value_changed);
void TypeIdWidget(edt::GUID type_guid, void* base, bool& value_changed);