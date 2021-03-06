#pragma once
#include <ice/config.h>
#include <ice/context.h>
#include <ice/error.h>
#include <ice/event.h>
#include <ice/net/buffer.h>
#include <ice/net/socket.h>
#include <utility>
#include <cstddef>

namespace ice::net::tcp {

class accept;
class connect;
class recv;
class send;
class send_some;

class socket : public net::socket {
public:
  explicit socket(ice::context& context) noexcept : net::socket(context) {
  }

  socket(ice::context& context, int family);
  socket(ice::context& context, int family, int protocol);

  void listen(std::size_t backlog = 0);

  tcp::accept accept();
  tcp::connect connect(const net::endpoint& endpoint);
  tcp::recv recv(char* data, std::size_t size);
  tcp::send send(const char* data, std::size_t size);
  tcp::send_some send_some(const char* data, std::size_t size);
};

class accept final : public ice::event {
public:
#if ICE_OS_WIN32
  accept(tcp::socket& socket) noexcept :
    context_(socket.context().handle()), socket_(socket.handle()),
    client_(socket.context(), socket.family(), socket.protocol()) {
  }
#else
  accept(tcp::socket& socket) noexcept :
    context_(socket.context().handle()), socket_(socket.handle()), client_(socket.context()) {
  }
#endif

  bool await_ready() noexcept;
  bool suspend() noexcept override;
  bool resume() noexcept override;

  tcp::socket await_resume() {
    if (ec_) {
      throw ice::system_error(ec_, "accept tcp socket");
    }
    return std::move(client_);
  }

private:
  ice::context::handle_view context_;
  net::socket::handle_view socket_;
  tcp::socket client_;
#if ICE_OS_WIN32
  constexpr static unsigned long buffer_size = sockaddr_storage_size + 16;
  char buffer_[buffer_size * 2];
  unsigned long bytes_ = 0;
#endif
};

class connect final : public ice::event {
public:
  connect(tcp::socket& socket, const net::endpoint& endpoint) noexcept;

  bool await_ready() noexcept;
  bool suspend() noexcept override;
  bool resume() noexcept override;

  void await_resume() {
    if (ec_) {
      throw ice::system_error(ec_, "connect");
    }
  }

private:
  ice::context::handle_view context_;
  net::socket::handle_view socket_;
  const net::endpoint endpoint_;
};

class recv final : public ice::event {
public:
  recv(tcp::socket& socket, char* data, std::size_t size) noexcept :
    context_(socket.context().handle()), socket_(socket.handle()), buffer_(data, size) {
  }

  bool await_ready() noexcept;
  bool suspend() noexcept override;
  bool resume() noexcept override;

  std::size_t await_resume() {
    if (ec_) {
      throw ice::system_error(ec_, "tcp recv");
    }
    return static_cast<std::size_t>(buffer_.size);
  }

private:
  ice::context::handle_view context_;
  net::socket::handle_view socket_;
  net::buffer buffer_;
#if ICE_OS_WIN32
  unsigned long bytes_ = 0;
  unsigned long flags_ = 0;
#endif
};

class send final : public ice::event {
public:
  send(tcp::socket& socket, const char* data, std::size_t size) noexcept :
    context_(socket.context().handle()), socket_(socket.handle()), buffer_(data, size) {
  }

  bool await_ready() noexcept;
  bool suspend() noexcept override;
  bool resume() noexcept override;

  std::size_t await_resume() {
    if (ec_) {
      throw ice::system_error(ec_, "tcp send");
    }
    return size_;
  }

private:
  ice::context::handle_view context_;
  net::socket::handle_view socket_;
  net::const_buffer buffer_;
  std::size_t size_ = 0;
#if ICE_OS_WIN32
  unsigned long bytes_ = 0;
#endif
};

class send_some final : public ice::event {
public:
  send_some(tcp::socket& socket, const char* data, std::size_t size) noexcept :
    context_(socket.context().handle()), socket_(socket.handle()), buffer_(data, size) {
  }

  bool await_ready() noexcept;
  bool suspend() noexcept override;
  bool resume() noexcept override;

  std::size_t await_resume() {
    if (ec_) {
      throw ice::system_error(ec_, "tcp send some");
    }
    return size_;
  }

private:
  ice::context::handle_view context_;
  net::socket::handle_view socket_;
  net::const_buffer buffer_;
  std::size_t size_ = 0;
#if ICE_OS_WIN32
  unsigned long bytes_ = 0;
#endif
};

inline tcp::accept socket::accept() {
  return { *this };
}

inline tcp::connect socket::connect(const net::endpoint& endpoint) {
  return { *this, endpoint };
}

inline tcp::recv socket::recv(char* data, std::size_t size) {
  return { *this, data, size };
}

inline tcp::send socket::send(const char* data, std::size_t size) {
  return { *this, data, size };
}

inline tcp::send_some socket::send_some(const char* data, std::size_t size) {
  return { *this, data, size };
}

}  // namespace ice::net::tcp
