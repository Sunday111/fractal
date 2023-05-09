#pragma once

template <typename Fn>
auto OnScopeLeave(Fn&& fn) {
  struct Guard__ {
    Guard__(Fn&& on_leave) : on_leave_(std::forward<Fn>(on_leave)) {}
    Guard__(Guard__&) = delete;
    Guard__(Guard__&& another) : on_leave_(std::move(another.on_leave_)) {
      another.empty_ = true;
    }
    ~Guard__() {
      if (!empty_) {
        on_leave_();
      }
    }

    Fn on_leave_;
    bool empty_ = false;
  } guard(std::forward<Fn>(fn));

  return guard;
}