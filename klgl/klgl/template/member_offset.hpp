#pragma once

template <auto Member>
[[nodiscard]] size_t MemberOffset() noexcept {
  using MemberTraits = ClassMemberTraits<decltype(Member)>;
  using T = typename MemberTraits::Class;
  auto ptr = &(reinterpret_cast<T const volatile*>(NULL)->*Member);
  return reinterpret_cast<size_t>(ptr);
}