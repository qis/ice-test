#include <ice/net/endpoint.h>
#include <ice/error.h>
#include <new>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#endif

namespace ice::net {

endpoint::endpoint() noexcept {
  new (&storage_) sockaddr_storage;
}

endpoint::endpoint(const endpoint& other) noexcept {
  new (static_cast<void*>(&storage_)) sockaddr_storage{ reinterpret_cast<const sockaddr_storage&>(other.storage_) };
}

endpoint& endpoint::operator=(const endpoint& other) noexcept {
  reinterpret_cast<sockaddr_storage&>(storage_) = reinterpret_cast<const sockaddr_storage&>(other.storage_);
  return *this;
}

endpoint::~endpoint() {
  reinterpret_cast<sockaddr_storage&>(storage_).~sockaddr_storage();
}

endpoint::endpoint(const std::string& host, std::uint16_t port) {
  int error = 0;
  if (host.find(":", 0, 5) == std::string::npos) {
    auto& addr = sockaddr_in();
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    size_ = sizeof(::sockaddr_in);
    error = ::inet_pton(AF_INET, host.data(), &addr.sin_addr);
  } else {
    auto& addr = sockaddr_in6();
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    size_ = sizeof(::sockaddr_in6);
    error = ::inet_pton(AF_INET6, host.data(), &addr.sin6_addr);
  }
  if (error != 1) {
#if ICE_OS_WIN32
    throw ice::system_error(error == 0 ? EINVAL : ::WSAGetLastError(), "invalid address");
#else
    throw ice::system_error(error == 0 ? EINVAL : errno, "invalid address");
#endif
  }
}

std::string endpoint::host() const {
  std::string buffer;
  switch (size_) {
  case sizeof(::sockaddr_in):
    buffer.resize(INET_ADDRSTRLEN);
    if (!inet_ntop(AF_INET, &sockaddr_in().sin_addr, buffer.data(), static_cast<socklen_t>(buffer.size()))) {
      return {};
    }
    break;
  case sizeof(::sockaddr_in6):
    buffer.resize(INET6_ADDRSTRLEN);
    if (!inet_ntop(AF_INET6, &sockaddr_in6().sin6_addr, buffer.data(), static_cast<socklen_t>(buffer.size()))) {
      return {};
    }
    break;
  default: return {};
  }
#if ICE_OS_WIN32
  buffer.resize(::strnlen_s(buffer.data(), buffer.size()));
#else
  buffer.resize(::strnlen(buffer.data(), buffer.size()));
#endif
  return buffer;
}

std::uint16_t endpoint::port() const noexcept {
  switch (size_) {
  case sizeof(::sockaddr_in): return ntohs(sockaddr_in().sin_port);
  case sizeof(::sockaddr_in6): return ntohs(sockaddr_in6().sin6_port);
  }
  return 0;
}

int endpoint::family() const noexcept {
  switch (size_) {
  case sizeof(::sockaddr_in): return AF_INET;
  case sizeof(::sockaddr_in6): return AF_INET6;
  }
  return 0;
}

}  // namespace ice::net
