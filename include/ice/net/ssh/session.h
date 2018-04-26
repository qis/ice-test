#pragma once
#include <ice/async.h>
#include <ice/context.h>
#include <ice/handle.h>
#include <ice/net/endpoint.h>
#include <ice/net/ssh/transport.h>
#include <ice/net/tcp/socket.h>
#include <string>

typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;

namespace ice::net::ssh {

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
  ssh::transport transport_;
};

}  // namespace ice::net::ssh
