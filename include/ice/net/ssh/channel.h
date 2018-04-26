#pragma once
#include <ice/async.h>
#include <ice/context.h>
#include <ice/handle.h>
#include <ice/net/endpoint.h>
#include <ice/net/ssh/transport.h>
#include <ice/net/tcp/socket.h>
#include <functional>
#include <string_view>
#include <vector>

typedef struct _LIBSSH2_CHANNEL LIBSSH2_CHANNEL;

namespace ice::net::ssh {

enum class stream {
  out = 0,
  err = 1,
};

class channel {
public:
  struct close_type {
    void operator()(LIBSSH2_CHANNEL* handle) noexcept;
  };
  using handle_type = ice::handle<LIBSSH2_CHANNEL*, nullptr, close_type>;
  using handle_view = handle_type::view;

  channel(handle_type handle, ice::net::tcp::socket& socket, ssh::transport& transport) noexcept :
    handle_(std::move(handle)), socket_(socket), transport_(transport) {
    buffer_.resize(1024);
  }

  channel(channel&& other) noexcept = default;
  channel& operator=(channel&& other) noexcept = default;

  channel(const channel& other) = delete;
  channel& operator=(const channel& other) = delete;

  void resize(std::size_t size) {
    buffer_.resize(size);
  }

  ice::async<void> exec(std::string_view command);
  ice::async<void> exec(std::string_view request, std::string_view command);

  ice::async<std::string_view> read(ssh::stream stream = ssh::stream::out);
  ice::async<void> write(std::string_view data, ssh::stream stream = ssh::stream::out);

  constexpr handle_type& handle() noexcept {
    return handle_;
  }

  constexpr const handle_type& handle() const noexcept {
    return handle_;
  }

  net::tcp::socket& socket() noexcept {
    return socket_.get();
  }

  const net::tcp::socket& socket() const noexcept {
    return socket_.get();
  }

  ssh::transport& transport() noexcept {
    return transport_.get();
  }

  const ssh::transport& transport() const noexcept {
    return transport_.get();
  }

private:
  handle_type handle_;
  std::reference_wrapper<ice::net::tcp::socket> socket_;
  std::reference_wrapper<ssh::transport> transport_;
  std::vector<char> buffer_;
};

}  // namespace ice::net::ssh
