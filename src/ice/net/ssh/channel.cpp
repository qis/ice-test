#include <ice/net/ssh/channel.h>
#include <ice/net/ssh/error.h>
#include <libssh2.h>

namespace ice::net::ssh {

void channel::close_type::operator()(LIBSSH2_CHANNEL* handle) noexcept {
  libssh2_channel_free(handle);
}

ice::async<void> channel::exec(std::string_view command) {
  return exec("exec", command);
}

ice::async<void> channel::exec(std::string_view request, std::string_view command) {
  const auto request_size = static_cast<unsigned>(request.size());
  const auto command_size = static_cast<unsigned>(command.size());
  while (true) {
    const auto rv =
      libssh2_channel_process_startup(handle_, request.data(), request_size, command.data(), command_size);
    if (rv == 0) {
      break;
    }
    if (rv != LIBSSH2_ERROR_EAGAIN) {
      throw ssh::domain_error(static_cast<int>(rv), "read");
    }
    co_await transport();
  }
  co_return;
}

ice::async<std::string_view> channel::read(ssh::stream stream) {
  const auto id = stream == ssh::stream::out ? 0 : SSH_EXTENDED_DATA_STDERR;
  while (true) {
    const auto rv = libssh2_channel_read_ex(handle_, id, buffer_.data(), buffer_.size());
    if (rv >= 0) {
      co_return std::string_view(buffer_.data(), static_cast<std::size_t>(rv));
    }
    if (rv != LIBSSH2_ERROR_EAGAIN) {
      throw ssh::domain_error(static_cast<int>(rv), "read");
    }
    co_await transport();
  }
}

ice::async<void> channel::write(std::string_view data, ssh::stream stream) {
  const auto id = stream == ssh::stream::out ? 0 : SSH_EXTENDED_DATA_STDERR;
  std::size_t written = 0;
  while (written < data.size()) {
    const auto rv = libssh2_channel_write_ex(handle_, id, data.data(), data.size());
    if (rv >= 0) {
      written += static_cast<std::size_t>(rv);
      continue;
    }
    if (rv != LIBSSH2_ERROR_EAGAIN) {
      throw ssh::domain_error(static_cast<int>(rv), "write");
    }
    co_await transport();
  }
  co_return;
}

}  // namespace ice::net::ssh
