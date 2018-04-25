#include <ice/error.h>
#include <libssh2.h>

namespace ice::ssh {

const std::error_category& error_category() noexcept;

class exception : public ice::system_error {
public:
  exception(std::string what, int code) : ice::system_error(std::move(what), code, error_category()) {
  }
};

namespace detail {

class error_category : public std::error_category {
public:
  const char* name() const noexcept override {
    return "ssh";
  }

  std::string message(int code) const override {
    switch (code) {
    case LIBSSH2_ERROR_NONE: return "ok";
    case LIBSSH2_ERROR_SOCKET_NONE: return "invalid socket";
    case LIBSSH2_ERROR_BANNER_RECV: return "banner recv error";
    case LIBSSH2_ERROR_BANNER_SEND: return "banner send error";
    case LIBSSH2_ERROR_INVALID_MAC: return "invalid mac";
    case LIBSSH2_ERROR_KEX_FAILURE: return "kex error";
    case LIBSSH2_ERROR_ALLOC: return "out of memory";
    case LIBSSH2_ERROR_SOCKET_SEND: return "socket send error";
    case LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE: return "key exchange error";
    case LIBSSH2_ERROR_TIMEOUT: return "timeout";
    case LIBSSH2_ERROR_HOSTKEY_INIT: return "host key importer error";
    case LIBSSH2_ERROR_HOSTKEY_SIGN: return "host key verification error";
    case LIBSSH2_ERROR_DECRYPT: return "decrypt error";
    case LIBSSH2_ERROR_SOCKET_DISCONNECT: return "disconnect error";
    case LIBSSH2_ERROR_PROTO: return "protocol error";
    case LIBSSH2_ERROR_PASSWORD_EXPIRED: return "password expired";
    case LIBSSH2_ERROR_FILE: return "file error";
    case LIBSSH2_ERROR_METHOD_NONE: return "missing method";
    case LIBSSH2_ERROR_AUTHENTICATION_FAILED: return "authentication error";
    case LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED: return "unverified public key";
    case LIBSSH2_ERROR_CHANNEL_OUTOFORDER: return "channel out of order";
    case LIBSSH2_ERROR_CHANNEL_FAILURE: return "channel error";
    case LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED: return "channel request denied";
    case LIBSSH2_ERROR_CHANNEL_UNKNOWN: return "unknown channel";
    case LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED: return "channel window size exceeded";
    case LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED: return "channel packet size exceeded";
    case LIBSSH2_ERROR_CHANNEL_CLOSED: return "channel closed";
    case LIBSSH2_ERROR_CHANNEL_EOF_SENT: return "channel eof sent";
    case LIBSSH2_ERROR_SCP_PROTOCOL: return "scp protocol error";
    case LIBSSH2_ERROR_ZLIB: return "zlib error";
    case LIBSSH2_ERROR_SOCKET_TIMEOUT: return "socket timeout";
    case LIBSSH2_ERROR_SFTP_PROTOCOL: return "sftp protocol error";
    case LIBSSH2_ERROR_REQUEST_DENIED: return "request denied";
    case LIBSSH2_ERROR_METHOD_NOT_SUPPORTED: return "method not supported";
    case LIBSSH2_ERROR_INVAL: return "invalid requested method type";
    case LIBSSH2_ERROR_INVALID_POLL_TYPE: return "invalid poll type";
    case LIBSSH2_ERROR_PUBLICKEY_PROTOCOL: return "public key protocol error";
    case LIBSSH2_ERROR_EAGAIN: return "asynchronous operation in progress";
    case LIBSSH2_ERROR_BUFFER_TOO_SMALL: return "buffer too small";
    case LIBSSH2_ERROR_BAD_USE: return "bad use";
    case LIBSSH2_ERROR_COMPRESS: return "compress error";
    case LIBSSH2_ERROR_OUT_OF_BOUNDARY: return "out of boundary";
    case LIBSSH2_ERROR_AGENT_PROTOCOL: return "agent protocol error";
    case LIBSSH2_ERROR_SOCKET_RECV: return "socket recv error";
    case LIBSSH2_ERROR_ENCRYPT: return "encrypt error";
    case LIBSSH2_ERROR_BAD_SOCKET: return "bad socket";
    case LIBSSH2_ERROR_KNOWN_HOSTS: return "known hosts error";
    }
    return "unknown error";
  }
};

}  // namespace detail

inline const std::error_category& error_category() noexcept {
  static const detail::error_category category;
  return category;
}

}  // namespace ice::ssh
