#pragma once

#include <limits>
#include <string_view>

#include "klgl/integer.hpp"

class Name
{
public:
    using NameId = ui32;
    static constexpr NameId kInvalidNameId = std::numeric_limits<NameId>::max();

public:
    Name() = default;
    Name(const std::string_view& view);
    Name(const std::string& string);
    Name(const char* strptr);

    std::string_view GetView() const;

    [[nodiscard]] friend inline bool operator==(const Name& a, const Name& b) noexcept
    {
        return a.id_ == b.id_;
    }

    [[nodiscard]] friend inline bool operator<(const Name& a, const Name& b) noexcept
    {
        return a.id_ < b.id_;
    }

private:
    [[nodiscard]] bool IsValid() const noexcept;

private:
    NameId id_ = kInvalidNameId;
};
