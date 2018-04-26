#pragma once
#include <ice/async.h>
#include <ice/config.h>
#include <ice/context.h>
#include <ice/error.h>
#include <ice/event.h>
#include <ice/net/buffer.h>
#include <ice/net/ssh/error.h>
#include <ice/net/tcp/socket.h>
#include <array>

typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;

namespace ice::net::ssh {
namespace detail {

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

}  // namespace detail

class session {
public:
  struct close_type {
    void operator()(LIBSSH2_SESSION* handle) noexcept;
  };
  using handle_type = ice::handle<LIBSSH2_SESSION*, nullptr, close_type>;
  using handle_view = handle_type::view;

  session(ice::context& context, int family);

  ~session();

  ice::async<void> connect(const ice::net::endpoint& ep, const std::string& user, const std::string& pass);

  constexpr handle_type& handle() noexcept {
    return handle_;
  }

  constexpr const handle_type& handle() const noexcept {
    return handle_;
  }

  constexpr net::tcp::socket& socket() noexcept {
    return socket_;
  }

  constexpr const net::tcp::socket& socket() const noexcept {
    return socket_;
  }

private:
  handle_type handle_;
  ice::net::tcp::socket socket_;
  detail::transport transport_;
};

}  // namespace ice::net::ssh
