#include <ice/net/ssh/transport.h>
#include <libssh2.h>
#include <algorithm>
#include <cstring>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#  include <new>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#endif

namespace ice::net::ssh {

bool transport::suspend() noexcept {
#if ICE_OS_WIN32
  const auto socket = socket_.as<SOCKET>();
  const auto buffer = std::launder(reinterpret_cast<LPWSABUF>(&buffer_));
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

}  // namespace ice::net::ssh
