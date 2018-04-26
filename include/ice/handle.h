#pragma once
#include <type_traits>
#include <utility>

namespace ice {

template <typename T>
struct is_handle {
  constexpr static auto value = std::is_integral_v<T> || std::is_pointer_v<T>;
};

template <typename T>
constexpr inline auto is_handle_v = ice::is_handle<T>::value;

template <typename T, auto I, typename C>
class handle {
public:
  using value_type = std::decay_t<T>;

  constexpr static value_type invalid_value() noexcept {
    if constexpr (std::is_pointer_v<value_type>) {
      if constexpr (std::is_null_pointer_v<decltype(I)>) {
        return I;
      } else {
        return reinterpret_cast<value_type>(I);
      }
    } else {
      if constexpr (std::is_pointer_v<decltype(I)>) {
        return reinterpret_cast<value_type>(I);
      } else {
        return static_cast<value_type>(I);
      }
    }
  }

  constexpr handle() noexcept = default;

  template <typename V, typename = std::enable_if_t<ice::is_handle_v<V>>>
  constexpr explicit handle(V value) noexcept {
    if constexpr (std::is_pointer_v<value_type> || std::is_pointer_v<V>) {
      value_ = reinterpret_cast<value_type>(value);
    } else {
      value_ = static_cast<value_type>(value);
    }
  }

  constexpr handle(handle&& other) noexcept : value_(other.release()) {
  }

  handle& operator=(handle&& other) noexcept {
    handle(std::move(other)).swap(*this);
    return *this;
  }

  handle(const handle& other) = delete;
  handle& operator=(const handle& other) = delete;

  ~handle() {
    reset();
  }

  constexpr explicit operator bool() const noexcept {
    return valid();
  }

  constexpr operator value_type() const noexcept {
    return value_;
  }

  constexpr bool valid() const noexcept {
    return value_ != invalid_value();
  }

  constexpr value_type& value() noexcept {
    return value_;
  }

  constexpr const value_type& value() const noexcept {
    return value_;
  }

  constexpr void swap(handle& other) noexcept {
    const auto value = other.value_;
    other.value_ = value_;
    value_ = value;
  }

  constexpr value_type release() noexcept {
    const auto value = value_;
    value_ = invalid_value();
    return value;
  }

  void reset() noexcept {
    reset(invalid_value());
  }

  template <typename V, typename = std::enable_if_t<ice::is_handle_v<V>>>
  void reset(V value) noexcept {
    if (valid()) {
      C{}(*this);
    }
    if constexpr (std::is_pointer_v<value_type> || std::is_pointer_v<V>) {
      value_ = reinterpret_cast<value_type>(value);
    } else {
      value_ = static_cast<value_type>(value);
    }
  }

  template <typename V, typename = std::enable_if_t<ice::is_handle_v<V>>>
  constexpr V as() const noexcept {
    if constexpr (std::is_pointer_v<value_type> || std::is_pointer_v<V>) {
      return reinterpret_cast<V>(value_);
    } else {
      return static_cast<V>(value_);
    }
  }

protected:
  value_type value_ = invalid_value();
};

template <typename T, auto I, typename C>
constexpr bool operator==(const ice::handle<T, I, C>& lhs, const ice::handle<T, I, C>& rhs) noexcept {
  return lhs.value() == rhs.value();
}

template <typename T, auto I, typename C>
constexpr bool operator!=(const ice::handle<T, I, C>& lhs, const ice::handle<T, I, C>& rhs) noexcept {
  return !(rhs == lhs);
}

template <typename T, auto I, typename C>
constexpr bool operator==(const ice::handle<T, I, C>& lhs, std::common_type_t<T> value) noexcept {
  return lhs.value() == value;
}

template <typename T, auto I, typename C>
constexpr bool operator!=(const ice::handle<T, I, C>& lhs, std::common_type_t<T> value) noexcept {
  return !(lhs == value);
}

template <typename T, auto I, typename C>
constexpr bool operator==(std::common_type_t<T> value, const ice::handle<T, I, C>& rhs) noexcept {
  return value == rhs.value();
}

template <typename T, auto I, typename C>
constexpr bool operator!=(std::common_type_t<T> value, const ice::handle<T, I, C>& rhs) noexcept {
  return !(value == rhs);
}

}  // namespace ice
