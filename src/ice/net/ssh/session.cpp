#include <ice/net/ssh/session.h>
#include <libssh2.h>

#if ICE_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace ice::net::ssh {
namespace detail {

bool transport::suspend() noexcept {
#if ICE_OS_WIN32
  const auto socket = socket_.as<SOCKET>();
  const auto buffer = reinterpret_cast<LPWSABUF>(&buffer_);
  switch (operation_) {
  case operation::recv:
    if (::WSARecv(socket, buffer, 1, &bytes_, &flags_, get(), nullptr) != SOCKET_ERROR) {
      ready_ = true;
      return false;
    }
    break;
  case operation::send:
    if (::WSASend(socket, buffer, 1, &bytes_, 0, get(), nullptr) != SOCKET_ERROR) {
      ready_ = true;
      return false;
    }
    break;
  default: ec_ = ice::error_code(std::errc::invalid_argument); return false;
  }
  if (const auto rc = ::WSAGetLastError(); rc != ERROR_IO_PENDING) {
    ec_ = rc;
    ready_ = true;
    return false;
  }
  return true;
#else
  switch (operation_) {
  case operation::recv: return queue_recv(context_, socket_);
  case operation::send: return queue_send(context_, socket_);
  default: ec_ = ice::error_code(std::errc::invalid_argument); break;
  }
  return false;
#endif
}

bool transport::resume() noexcept {
#if ICE_OS_WIN32
  const auto socket = socket_.as<HANDLE>();
  if (!::GetOverlappedResult(socket, get(), &bytes_, FALSE)) {
    ec_ = ::GetLastError();
  }
  ready_ = true;
#endif
  return true;
}

long long transport::on_recv(char* data, std::size_t size, int flags) noexcept {
#if ICE_OS_WIN32
  (void)flags;
  if (std::exchange(ready_, false)) {
    assert(bytes_ <= size);
    std::memcpy(data, buffer_.data, bytes_);
    return static_cast<ssize_t>(bytes_);
  }
  operation_ = operation::recv;
  buffer_.size = static_cast<unsigned long>(std::min(size, storage_.size()));
  return EAGAIN > 0 ? -EAGAIN : EAGAIN;
#else
  do {
    if (const auto rc = ::recv(socket_, data, size, flags); rc >= 0) {
      return rc;
    }
  } while (errno == EINTR);
  operation_ = operation::recv;
  return errno > 0 ? -errno : errno;
#endif
}

long long transport::on_send(const char* data, std::size_t size, int flags) noexcept {
#if ICE_OS_WIN32
  (void)flags;
  if (std::exchange(ready_, false)) {
    assert(bytes_ <= size);
    if (std::memcmp(data, buffer_.data, bytes_) != 0) {
      assert(false);
    }
    return static_cast<ssize_t>(bytes_);
  }
  operation_ = operation::send;
  buffer_.size = static_cast<ULONG>(std::min(size, storage_.size()));
  std::memcpy(buffer_.data, data, buffer_.size);
  return EAGAIN > 0 ? -EAGAIN : EAGAIN;
#else
  do {
    if (const auto rc = ::send(socket_, data, size, flags); rc >= 0) {
      return rc;
    }
  } while (errno == EINTR);
  operation_ = operation::send;
  return errno > 0 ? -errno : errno;
#endif
}

static LIBSSH2_RECV_FUNC(recv_callback) {
  (void)socket;
  const auto data = reinterpret_cast<char*>(buffer);
  const auto size = static_cast<std::size_t>(length);
  if (!*abstract) {
    return ::recv(socket, data, socklen_t(size), 0);
  }
  return reinterpret_cast<detail::transport*>(*abstract)->on_recv(data, size, flags);
}

static LIBSSH2_SEND_FUNC(send_callback) {
  (void)socket;
  const auto data = reinterpret_cast<const char*>(buffer);
  const auto size = static_cast<std::size_t>(length);
  if (!*abstract) {
    return ::send(socket, data, socklen_t(size), 0);
  }
  return reinterpret_cast<detail::transport*>(*abstract)->on_send(data, size, flags);
}

}  // namespace detail

void session::close_type::operator()(LIBSSH2_SESSION* handle) noexcept {
  libssh2_session_free(handle);
}

session::session(ice::context& context, int family) :
  handle_(libssh2_session_init()), socket_(context, family), transport_(socket_) {
  if (!handle_) {
    throw ssh::domain_error(LIBSSH2_ERROR_ALLOC, "initialize session");
  }
  *libssh2_session_abstract(handle_.as<LIBSSH2_SESSION*>()) = &transport_;
  libssh2_session_callback_set(handle_, LIBSSH2_CALLBACK_RECV, reinterpret_cast<void*>(&detail::recv_callback));
  libssh2_session_callback_set(handle_, LIBSSH2_CALLBACK_SEND, reinterpret_cast<void*>(&detail::send_callback));
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

}  // namespace ice::net::ssh
