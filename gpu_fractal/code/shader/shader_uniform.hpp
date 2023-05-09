#pragma once

#include <span>
#include <string>
#include <vector>

#include "EverydayTools/GUID.hpp"
#include "integer.hpp"
#include "name_cache/name.hpp"

class ShaderUniform {
 public:
  ShaderUniform();
  ~ShaderUniform();
  ShaderUniform(const ShaderUniform&) = delete;
  ShaderUniform(ShaderUniform&& another);
  void MoveFrom(ShaderUniform& another);
  void SendValue() const;
  void SetValue(std::span<const ui8> new_value);
  void SetType(edt::GUID type_guid);
  void SetName(Name name) { name_ = name; }
  void SetLocation(ui32 location) { location_ = location; }
  void EnsureTypeMatch(edt::GUID type_guid) const;

  [[nodiscard]] bool IsEmpty() const noexcept;
  [[nodiscard]] Name GetName() const noexcept { return name_; }
  [[nodiscard]] edt::GUID GetTypeGUID() const noexcept { return type_guid_; }
  [[nodiscard]] ui32 GetLocation() const noexcept { return location_; }

  //[[nodiscard]] std::span<ui8> GetValue() noexcept { return value_; }
  [[nodiscard]] std::span<const ui8> GetValue() const noexcept {
    return value_;
  }

  ShaderUniform& operator=(const ShaderUniform&) = delete;
  ShaderUniform& operator=(ShaderUniform&& another);

 private:
  void Clear();
  void CheckNotEmpty() const;

 private:
  std::vector<ui8> value_;
  Name name_;
  ui32 location_;
  edt::GUID type_guid_;
  mutable bool sent_ = false;
};
