#pragma once

#include <string_view>

class ScopeAnnotation {
 public:
  ScopeAnnotation(std::string_view scope_name, size_t id = 0) noexcept;
  ScopeAnnotation(ScopeAnnotation&&) = delete;
  ScopeAnnotation(const ScopeAnnotation&) = delete;
  ~ScopeAnnotation() noexcept;

  ScopeAnnotation& operator=(ScopeAnnotation&&) = delete;
  ScopeAnnotation& operator=(const ScopeAnnotation&) = delete;
};
