#include <ice/context.h>
#include <ice/net/tcp/socket.h>
#include <ice/ssh/error.h>
#include <libssh2.h>

namespace ice::ssh {
namespace detail {

#if ICE_OS_WIN32

class transport : public ice::event {
public:
  enum class operation {
    none,
    recv,
    send,
  };

  transport(ice::net::tcp::socket& socket) : socket_(socket) {
    buffer_.buf = storage_.data();
    buffer_.len = 0;
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  bool await_suspend(awaiter_type awaiter) noexcept {
    reset(awaiter);
    switch (operation_) {
    case operation::recv:
      if (::WSARecv(socket_.handle(), &buffer_, 1, &bytes_, &flags_, get(), nullptr) != SOCKET_ERROR) {
        ready_ = true;
        return false;
      }
      break;
    case operation::send:
      if (::WSASend(socket_.handle(), &buffer_, 1, &bytes_, 0, get(), nullptr) != SOCKET_ERROR) {
        ready_ = true;
        return false;
      }
      break;
    default: ec_ = ice::error_code(LIBSSH2_ERROR_BAD_USE, ice::combined); return false;
    }
    if (const auto rc = ::WSAGetLastError(); rc != ERROR_IO_PENDING) {
      ec_ = rc;
      ready_ = true;
      return false;
    }
    return true;
  }

  void async_resume() noexcept override {
    if (!::GetOverlappedResult(socket_.handle().as<HANDLE>(), get(), &bytes_, FALSE)) {
      ec_ = ::GetLastError();
    }
    ready_ = true;
    resume();
  }

  void await_resume() {
    if (ec_) {
      throw ice::exception("ssh transport (" + std::to_string(static_cast<int>(operation_)) + ")", ec_);
    }
    operation_ = operation::none;
  }

  ssize_t on_recv(libssh2_socket_t, char* data, std::size_t size, int) noexcept {
    if (std::exchange(ready_, false)) {
      assert(bytes_ <= size);
      std::memcpy(data, buffer_.buf, bytes_);
      return static_cast<ssize_t>(bytes_);
    }
    bytes_ = 0;
    operation_ = operation::recv;

    buffer_ = { static_cast<ULONG>(std::min(size, storage_.size())), storage_.data() };
    return EAGAIN > 0 ? -EAGAIN : EAGAIN;
  }

  ssize_t on_send(libssh2_socket_t, const char* data, std::size_t size, int) noexcept {
    if (std::exchange(ready_, false)) {
      assert(bytes_ <= size);
      if (std::memcmp(data, buffer_.buf, bytes_) != 0) {
        assert(false);
      }
      return static_cast<ssize_t>(bytes_);
    }
    bytes_ = 0;
    operation_ = operation::send;
    buffer_.len = static_cast<ULONG>(std::min(size, storage_.size()));
    std::memcpy(buffer_.buf, data, buffer_.len);
    return EAGAIN > 0 ? -EAGAIN : EAGAIN;
  }

private:
  ice::net::tcp::socket& socket_;
  operation operation_ = operation::none;
  std::array<char, 4096> storage_;
  bool ready_ = false;
  WSABUF buffer_ = {};
  DWORD bytes_ = 0;
  DWORD flags_ = 0;
};

#else

class transport : public ice::event {
public:
  enum class operation {
    none,
    recv,
    send,
  };

  transport(ice::net::tcp::socket& socket) : socket_(socket) {
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  bool await_suspend(awaiter_type awaiter) noexcept {
    reset(awaiter);
    switch (operation_) {
    case operation::recv: return suspend(socket_.context().handle(), socket_.handle(), EVFILT_READ);
    case operation::send: return suspend(socket_.context().handle(), socket_.handle(), EVFILT_WRITE);
    default: ec_ = ice::error_code(LIBSSH2_ERROR_BAD_USE, ice::combined); break;
    }
    return false;
  }

  void async_resume() noexcept override {
    resume();
  }

  void await_resume() {
    if (ec_) {
      throw ice::exception("ssh transport (" + std::to_string(static_cast<int>(operation_)) + ")", ec_);
    }
    operation_ = operation::none;
  }

  ssize_t on_recv(libssh2_socket_t, char* data, std::size_t size, int flags) noexcept {
    do {
      if (const auto rc = ::recv(socket_.handle(), data, size, flags); rc >= 0) {
        return rc;
      }
    } while (errno == EINTR);
    operation_ = operation::recv;
    return errno > 0 ? -errno : errno;
  }

  ssize_t on_send(libssh2_socket_t, const char* data, std::size_t size, int flags) noexcept {
    do {
      if (const auto rc = ::send(socket_.handle(), data, size, flags); rc >= 0) {
        return rc;
      }
    } while (errno == EINTR);
    operation_ = operation::send;
    return errno > 0 ? -errno : errno;
  }

private:
  ice::net::tcp::socket& socket_;
  operation operation_ = operation::none;
};

#endif

}  // namespace detail

class session {
public:
  struct close_type {
    void operator()(LIBSSH2_SESSION* handle) noexcept {
      *libssh2_session_abstract(handle) = nullptr;
      libssh2_session_disconnect(handle, "shutdown");
      libssh2_session_free(handle);
    }
  };

  using handle_type = ice::handle<LIBSSH2_SESSION*, nullptr, close_type>;

  session(ice::context& context) : handle_(libssh2_session_init()), socket_(context), transport_(socket_) {
    if (!handle_) {
      throw ssh::exception("create ssh session", LIBSSH2_ERROR_ALLOC);
    }
    *libssh2_session_abstract(handle_) = this;
    libssh2_session_callback_set(handle_, LIBSSH2_CALLBACK_RECV, reinterpret_cast<void*>(&recv_callback));
    libssh2_session_callback_set(handle_, LIBSSH2_CALLBACK_SEND, reinterpret_cast<void*>(&send_callback));
    libssh2_session_set_blocking(handle_, 0);
  }

  ice::async<void> connect(
    const ice::net::endpoint& endpoint, const std::string& username, const std::string& password) {
    socket_ = ice::net::tcp::socket(socket_.context(), endpoint.family());
    co_await socket_.connect(endpoint);
    while (true) {
      const auto rc = libssh2_session_handshake(handle_, socket_.handle());
      if (rc == LIBSSH2_ERROR_NONE) {
        break;
      }
      if (rc != LIBSSH2_ERROR_EAGAIN) {
        throw ssh::exception("ssh handshake", rc);
      }
      co_await transport_;
    }
    while (true) {
      const auto rc = libssh2_userauth_password(handle_, username.data(), password.data());
      if (rc == LIBSSH2_ERROR_NONE) {
        break;
      }
      if (rc != LIBSSH2_ERROR_EAGAIN) {
        throw ssh::exception("ssh password authentication", rc);
      }
      co_await transport_;
    }
    co_return;
  }

  ice::net::tcp::socket& socket() noexcept {
    return socket_;
  }

  const ice::net::tcp::socket& socket() const noexcept {
    return socket_;
  }

  constexpr handle_type& handle() noexcept {
    return handle_;
  }

  constexpr const handle_type& handle() const noexcept {
    return handle_;
  }

private:
  static LIBSSH2_RECV_FUNC(recv_callback) {
    const auto data = reinterpret_cast<char*>(buffer);
    const auto size = static_cast<std::size_t>(length);
    if (!*abstract) {
      return ::recv(socket, data, socklen_t(size), 0);
    }
    return reinterpret_cast<session*>(*abstract)->transport_.on_recv(socket, data, size, flags);
  }

  static LIBSSH2_SEND_FUNC(send_callback) {
    const auto data = reinterpret_cast<const char*>(buffer);
    const auto size = static_cast<std::size_t>(length);
    if (!*abstract) {
      return ::send(socket, data, socklen_t(size), 0);
    }
    return reinterpret_cast<ssh::session*>(*abstract)->transport_.on_send(socket, data, size, flags);
  }

  handle_type handle_;
  ice::net::tcp::socket socket_;
  detail::transport transport_;
};

}  // namespace ice::ssh
