#include <ice/net/buffer.h>

#if ICE_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <type_traits>
#include <cstddef>
#include <cstdlib>
#endif

namespace ice::net {

#if ICE_OS_WIN32

class buffer_check {
public:
  buffer_check() noexcept {
    // Verify size.
    static_assert(sizeof(buffer) == sizeof(WSABUF));

    // Verify alignment.
    static_assert(alignof(buffer) == alignof(WSABUF));

    // Verify size type.
    using buffer_size_type = decltype(buffer::size);
    using wsabuf_size_type = decltype(WSABUF::len);
    static_assert(std::is_same_v<buffer_size_type, wsabuf_size_type>);

    // Verify data type.
    using buffer_data_type = std::remove_const_t<std::remove_pointer_t<decltype(buffer::data)>>;
    using wsabuf_data_type = std::remove_pointer_t<decltype(WSABUF::buf)>;
    static_assert(std::is_same_v<buffer_data_type, wsabuf_data_type>);

    // Verify size offset.
    static_assert(offsetof(buffer, size) == offsetof(WSABUF, len));

    // Verify data offset.
    if constexpr (offsetof(buffer, data) != offsetof(WSABUF, buf)) {
      std::abort();
    }
  }
};

const buffer_check g_buffer_check;

#endif

}  // namespace ice::net
