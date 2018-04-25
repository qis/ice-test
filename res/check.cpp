#include <cstddef>

#ifdef WIN32
# ifdef _MSC_VER
#  pragma warning(push, 0)
# endif
# define NOMINMAX 1
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
# include <winsock2.h>
# ifdef _MSC_VER
#  pragma warning(pop)
# endif
#else
# include <sys/types.h>
# include <sys/socket.h>
#endif

#if defined(WIN32)
struct native_event : OVERLAPPED {};
#elif defined(__linux__)
#include <sys/epoll.h>
struct native_event : epoll_event {};
#elif defined(__FreeBSD__)
#include <sys/event.h>
using native_event_base = struct kevent;
struct native_event : native_event_base {};
#endif

template <typename T, std::size_t Size = sizeof(T), std::size_t Alignment = alignof(T)>
struct check;

int main() {
  check<native_event>{};
  check<sockaddr_storage>{};
  check<linger>{};
}
