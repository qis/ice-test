#pragma once
#include <ice/config.h>
#include <ice/handle.h>
#include <cstdint>

#ifndef ICE_LIKELY
#ifdef __has_builtin
#if __has_builtin(__builtin_expect)
#define ICE_LIKELY(expr) __builtin_expect(expr, 1)
#else
#define ICE_LIKELY(expr) expr
#endif
#else
#define ICE_LIKELY(expr) expr
#endif
#endif

#ifndef ICE_UNLIKELY
#ifdef __has_builtin
#if __has_builtin(__builtin_expect)
#define ICE_UNLIKELY(expr) __builtin_expect(expr, 0)
#else
#define ICE_UNLIKELY(expr) expr
#endif
#else
#define ICE_UNLIKELY(expr) expr
#endif
#endif

namespace ice {

class thread_local_storage {
public:
  struct close_type {
    void operator()(std::uintptr_t handle) noexcept;
  };

  using handle_type = ice::handle<std::uintptr_t, 0, close_type>;
  using handle_view = handle_type::view;

  thread_local_storage();

  void set(void* value);

  void* get() noexcept;
  const void* get() const noexcept;

  handle_type& handle() noexcept {
    return handle_;
  }

  const handle_type& handle() const noexcept {
    return handle_;
  }

private:
  handle_type handle_;
};

}  // namespace ice
