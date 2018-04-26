// Suggested by @ThePhD on cpplang.slack.com.

#include <type_traits>
#include <memory>

namespace lib {

namespace detail {
template <typename T>
struct has_typedef_pointer {
  typedef char yes[1];
  typedef char no[2];

  template <typename C>
  static yes& test(typename C::pointer*);

  template <typename>
  static no& test(...);

  static const bool value = sizeof(test<T>(0)) == sizeof(yes);
};

template <bool b, typename T, typename Fallback>
struct pointer_typedef_enable_if {};

template <typename T, typename Fallback>
struct pointer_typedef_enable_if<false, T, Fallback> {
  typedef Fallback type;
};

template <typename T, typename Fallback>
struct pointer_typedef_enable_if<true, T, Fallback> {
  typedef typename T::pointer type;
};
}  // namespace detail

template <typename T, typename Dx>
struct pointer_type {
  typedef typename detail::pointer_typedef_enable_if<detail::has_typedef_pointer<Dx>::value, Dx, T>::type type;
};

template <typename T, typename D>
using pointer_type_t = typename pointer_type<T, D>::type;

template <typename T, typename = void>
struct ebco {
  T value;

  ebco() = default;
  ebco(const ebco&) = default;
  ebco(ebco&&) = default;
  ebco& operator=(const ebco&) = default;
  ebco& operator=(ebco&&) = default;
  ebco(const T& v) : value(v){};
  ebco(T&& v) : value(std::move(v)){};
  ebco& operator=(const T& v) {
    value = v;
  }
  ebco& operator=(T&& v) {
    value = std::move(v);
  };
  template <typename Arg, typename... Args,
    typename = std::enable_if_t<!std::is_same_v<std::remove_reference_t<std::remove_cv_t<Arg>>, ebco> &&
      !std::is_same_v<std::remove_reference_t<std::remove_cv_t<Arg>>, T>>>
  ebco(Arg&& arg, Args&&... args) : T(std::forward<Arg>(arg), std::forward<Args>(args)...){};

  T& get_value() {
    return value;
  }

  T const& get_value() const {
    return value;
  }
};

template <typename T>
struct ebco<T, std::enable_if_t<std::is_class_v<T>>> : T {
  ebco() = default;
  ebco(const ebco&) = default;
  ebco(ebco&&) = default;
  ebco(const T& v) : T(v){};
  ebco(T&& v) : T(std::move(v)){};
  template <typename Arg, typename... Args,
    typename = std::enable_if_t<!std::is_same_v<std::remove_reference_t<std::remove_cv_t<Arg>>, ebco> &&
      !std::is_same_v<std::remove_reference_t<std::remove_cv_t<Arg>>, T>>>
  ebco(Arg&& arg, Args&&... args) : T(std::forward<Arg>(arg), std::forward<Args>(args)...){};

  ebco& operator=(const ebco&) = default;
  ebco& operator=(ebco&&) = default;
  ebco& operator=(const T& v) {
    static_cast<T&>(*this) = v;
  }
  ebco& operator=(T&& v) {
    static_cast<T&>(*this) = std::move(v);
  };

  T& get_value() {
    return static_cast<T&>(*this);
  }

  T const& get_value() const {
    return static_cast<T const&>(*this);
  }
};

template <typename T>
struct default_handle_deleter {
  using pointer = T;

  static void write_null(pointer& p) {
    p = T();
  }

  static bool is_null(const pointer& p) {
    return p == T();
  }

  void operator()(const pointer&) const {
    // no-op: override the default deleter
    // for handle-specific behavior
  }
};

template <typename T>
struct default_handle_deleter<T*> : std::default_delete<T> {
  using pointer = T*;

  static void write_null(pointer& p) {
    p = T();
  }

  static bool is_null(const pointer& p) {
    return p == T();
  }
};

template <typename T, typename Dx = default_handle_deleter<T>>
struct handle : ebco<Dx> {
public:
  typedef pointer_type_t<T, Dx> pointer;
  typedef Dx deleter_type;

private:
  typedef ebco<deleter_type> deleter_base;

  pointer res;

public:
  handle() {
  }

  handle(pointer h) : res(h) {
  }

  handle(std::nullptr_t) {
    deleter_type& deleter = get_deleter();
    deleter.write_null(res);
  }

  handle(pointer h, deleter_type d) : deleter_base(std::move(d)), res(h) {
  }

  handle(std::nullptr_t, deleter_type d) : deleter_base(std::move(d)) {
    deleter_type& deleter = get_deleter();
    deleter.write_null(res);
  }

  template <typename... DxArgs>
  handle(pointer h, DxArgs&&... dx_args) : deleter_base(std::forward<DxArgs>(dx_args)...), res(h) {
  }

  template <typename... DxArgs>
  handle(std::nullptr_t, DxArgs&&... dx_args) : deleter_base(std::forward<Dx>(dx_args)...) {
    deleter_type& deleter = get_deleter();
    deleter.write_null(res);
  }

  handle(const handle& nocopy) = delete;
  handle(handle&& mov) : deleter_base(std::move(mov)), res(std::move(mov.res)) {
    deleter_type& deleter = get_deleter();
    mov.reset(mov.get_null());
  }

  handle& operator=(const handle&) = delete;
  handle& operator=(handle&& right) {
    reset(right.release());
    return *this;
  }
  handle& operator=(pointer right) {
    reset(right);
    return *this;
  }
  handle& operator=(std::nullptr_t) {
    reset(nullptr);
    return *this;
  }

  explicit operator bool() const {
    return is_null();
  }

  pointer get_null() const {
    pointer p;
    const deleter_type& deleter = get_deleter();
    deleter.write_null(p);
    return p;
  }

  bool is_null() const {
    const deleter_type& deleter = get_deleter();
    return deleter.is_null(res);
  }

  pointer get() const {
    return res;
  }

  void reset(pointer h) {
    deleter_type& deleter = get_deleter();
    if (!is_null())
      deleter(res);
    res = h;
  }

  void reset(std::nullptr_t) {
    deleter_type& deleter = get_deleter();
    if (!is_null())
      deleter(res);
    deleter.write_null(res);
  }

  pointer release() {
    pointer rel = std::move(res);
    deleter_type& deleter = get_deleter();
    deleter.write_null(res);
    return rel;
  }

  void swap(handle& other) {
    std::swap(other.get_deleter(), get_deleter());
    std::swap(other.res, res);
  }

  deleter_type& get_deleter() {
    deleter_type& deleter = deleter_base::get_value();
    return deleter;
  }

  const deleter_type& get_deleter() const {
    const deleter_type& deleter = deleter_base::get_value();
    return deleter;
  }

  pointer operator*() {
    return get();
  }

  pointer operator->() {
    return get();
  }

  ~handle() {
    const deleter_type& deleter = get_deleter();
    if (!is_null())
      deleter(res);
    deleter.write_null(res);
  }
};

template <typename T, typename Dx>
inline bool operator==(const handle<T, Dx>& left, const handle<T, Dx>& right) {
  return left.get() == right.get();
}
template <typename T, typename Dx>
inline bool operator==(std::nullptr_t right, const handle<T, Dx>& left) {
  return left.get() == left.get_null();
}
template <typename T, typename Dx>
inline bool operator==(const handle<T, Dx>& left, std::nullptr_t right) {
  return left.get() == right;
}
template <typename T, typename Dx>
inline bool operator==(typename handle<T, Dx>::pointer right, const handle<T, Dx>& left) {
  return left.get() == right;
}
template <typename T, typename Dx>
inline bool operator==(const handle<T, Dx>& left, typename handle<T, Dx>::pointer right) {
  return left.get() == right;
}

template <typename T, typename Dx>
inline bool operator!=(const handle<T, Dx>& left, const handle<T, Dx>& right) {
  return left.get() != right.get();
}
template <typename T, typename Dx>
inline bool operator!=(std::nullptr_t right, const handle<T, Dx>& left) {
  return left.get() != left.get_null();
}
template <typename T, typename Dx>
inline bool operator!=(const handle<T, Dx>& left, std::nullptr_t right) {
  return left.get() != left.get_null();
}
template <typename T, typename Dx>
inline bool operator!=(typename handle<T, Dx>::pointer right, const handle<T, Dx>& left) {
  return left.get() != right;
}
template <typename T, typename Dx>
inline bool operator!=(const handle<T, Dx>& left, typename handle<T, Dx>::pointer right) {
  return left.get() != right;
}

}  // namespace lib

#include <iostream>

struct my_empty_int_deleter {
  static void write_null(int& h) {
    h = 0;
  }

  static bool is_null(const int& h) {
    return h == 0;
  }

  void operator()(const int&) const {
    // nothing
  }
};

struct my_stateful_int_deleter {
  int the_null_value;

  my_stateful_int_deleter(int stateful_null_value = 24) : the_null_value(stateful_null_value) {
  }

  void write_null(int& h) const {
    h = the_null_value;
  }

  bool is_null(const int& h) const {
    return h == the_null_value;
  }

  void operator()(const int& h) const {
    if (h < the_null_value) {
      std::cout << "Less than the null value on deletion" << std::endl;
    }
    // nothing
  }
};

int main(int, char* []) {
  lib::handle<int> h1{};
  lib::handle<int, my_empty_int_deleter> h2{};
  lib::handle<int, my_stateful_int_deleter> h3{ 0, 25 };

  std::cout << "sizeof(handle<int>) == " << sizeof(lib::handle<int, my_empty_int_deleter>) << std::endl;
  std::cout << "sizeof(handle<int, my_empty_int_deleter>) == " << sizeof(lib::handle<int, my_empty_int_deleter>)
            << std::endl;
  std::cout << "sizeof(handle<int, my_stateful_int_deleter>) == " << sizeof(lib::handle<int, my_stateful_int_deleter>)
            << std::endl;

  return 0;
}
