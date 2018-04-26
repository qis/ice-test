#include <ice/net/ssh/channel.h>
#include <ice/net/ssh/error.h>
#include <libssh2.h>

namespace ice::net::ssh {

void channel::close_type::operator()(LIBSSH2_CHANNEL* handle) noexcept {
  libssh2_channel_free(handle);
}

}  // namespace ice::net::ssh
