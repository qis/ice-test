#include <ice/net/ssh/session.h>
#include <ice/net/ssh/error.h>
#include <libssh2.h>

#if ICE_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

namespace ice::net::ssh {
namespace {

static LIBSSH2_RECV_FUNC(recv_callback) {
  (void)socket;
  const auto data = reinterpret_cast<char*>(buffer);
  const auto size = static_cast<std::size_t>(length);
  if (!*abstract) {
    return ::recv(socket, data, socklen_t(size), 0);
  }
  return reinterpret_cast<ssh::transport*>(*abstract)->on_recv(data, size, flags);
}

static LIBSSH2_SEND_FUNC(send_callback) {
  (void)socket;
  const auto data = reinterpret_cast<const char*>(buffer);
  const auto size = static_cast<std::size_t>(length);
  if (!*abstract) {
    return ::send(socket, data, socklen_t(size), 0);
  }
  return reinterpret_cast<ssh::transport*>(*abstract)->on_send(data, size, flags);
}

}  // namespace

void session::close_type::operator()(LIBSSH2_SESSION* handle) noexcept {
  libssh2_session_free(handle);
}

session::session(ice::context& context, int family) :
  handle_(libssh2_session_init()), socket_(context, family), transport_(socket_) {
  if (!handle_) {
    throw ssh::domain_error(LIBSSH2_ERROR_ALLOC, "initialize session");
  }
  *libssh2_session_abstract(handle_.as<LIBSSH2_SESSION*>()) = &transport_;
  libssh2_session_callback_set(handle_, LIBSSH2_CALLBACK_RECV, reinterpret_cast<void*>(&recv_callback));
  libssh2_session_callback_set(handle_, LIBSSH2_CALLBACK_SEND, reinterpret_cast<void*>(&send_callback));
  libssh2_session_set_blocking(handle_, 0);
}

session::~session() {
  *libssh2_session_abstract(handle_) = nullptr;
  libssh2_session_disconnect(handle_, "shutdown");
}

ice::async<void> session::connect(const ice::net::endpoint& ep, const std::string& user, const std::string& pass) {
  co_await socket_.connect(ep);
  while (true) {
    const auto rc = libssh2_session_handshake(handle_, socket_.handle());
    if (rc == LIBSSH2_ERROR_NONE) {
      break;
    }
    if (rc != LIBSSH2_ERROR_EAGAIN) {
      throw ssh::domain_error(rc, "handshake");
    }
    co_await transport_;
  }
  while (true) {
    const auto rc = libssh2_userauth_password(handle_, user.data(), pass.data());
    if (rc == LIBSSH2_ERROR_NONE) {
      break;
    }
    if (rc != LIBSSH2_ERROR_EAGAIN) {
      throw ssh::domain_error(rc, "password authentication");
    }
    co_await transport_;
  }
  co_return;
}

ice::async<ssh::channel> session::open() {
  while (true) {
    const auto channel = libssh2_channel_open_session(handle_);
    if (channel) {
      co_return ssh::channel(ssh::channel::handle_type(channel), socket_, transport_);
    }
    if (const auto rc = libssh2_session_last_error(handle_, nullptr, nullptr, 0); rc != LIBSSH2_ERROR_EAGAIN) {
      throw ssh::domain_error(rc, "open channel");
    }
    co_await transport_;
  }
}

}  // namespace ice::net::ssh
