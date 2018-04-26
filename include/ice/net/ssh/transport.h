#pragma once
#include <ice/config.h>
#include <ice/event.h>
#include <ice/error.h>
#include <ice/net/tcp/socket.h>

#if ICE_OS_WIN32
#include <ice/net/buffer.h>
#include <array>
#endif

namespace ice::net::ssh {

class transport final : public ice::event {
public:
  enum class operation {
    none,
    recv,
    send,
  };

#if ICE_OS_WIN32
  transport(net::tcp::socket& socket) noexcept :
    context_(socket.context().handle()), socket_(socket.handle()), buffer_(storage_.data(), storage_.size()) {
  }
#else
  transport(net::tcp::socket& socket) noexcept : context_(socket.context().handle()), socket_(socket.handle()) {
  }
#endif

  constexpr bool await_ready() noexcept {
    return false;
  }

  bool suspend() noexcept override;
  bool resume() noexcept override;

  void await_resume() {
    if (ec_) {
      throw ice::system_error(ec_, "tcp recv");
    }
  }

  long long on_recv(char* data, std::size_t size, int flags) noexcept;
  long long on_send(const char* data, std::size_t size, int flags) noexcept;

private:
  operation operation_ = operation::none;
  ice::context::handle_view context_;
  net::socket::handle_view socket_;
#if ICE_OS_WIN32
  std::array<char, 4096> storage_;
  net::buffer buffer_;
  bool ready_ = false;
  unsigned long bytes_ = 0;
  unsigned long flags_ = 0;
#endif
};

}  // namespace ice::net::ssh
