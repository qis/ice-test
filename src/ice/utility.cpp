#include <ice/utility.h>
#include <ice/error.h>

#if ICE_OS_WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace ice {

void thread_local_storage::close_type::operator()(std::uintptr_t handle) noexcept {
#if ICE_OS_WIN32
  ::TlsFree(static_cast<DWORD>(handle));
#else
  ::pthread_key_delete(static_cast<pthread_key_t>(handle));
#endif
}

thread_local_storage::thread_local_storage() {
#if ICE_OS_WIN32
  handle_.reset(::TlsAlloc());
  if (!handle_) {
    throw ice::system_error(::GetLastError(), "thread local storage");
  }
#else
  pthread_key_t handle{};
  if (const auto rc = ::pthread_key_create(&handle, nullptr)) {
    throw ice::system_error(rc, "allocate thread local storage");
  }
  handle_.reset(handle);
#endif
}

void thread_local_storage::set(void* value) {
#if ICE_OS_WIN32
  if (!::TlsSetValue(handle_.as<DWORD>(), value)) {
    throw ice::system_error(::GetLastError(), "set thread local storage value");
  }
#else
  if (const auto rc = ::pthread_setspecific(handle_.as<pthread_key_t>(), value)) {
    throw ice::system_error(rc, "set thread local storage value");
  }
#endif
}

void* thread_local_storage::get() noexcept {
#if ICE_OS_WIN32
  return ::TlsGetValue(handle_.as<DWORD>());
#else
  return ::pthread_getspecific(handle_.as<pthread_key_t>());
#endif
}

const void* thread_local_storage::get() const noexcept {
#if ICE_OS_WIN32
  return ::TlsGetValue(handle_.as<DWORD>());
#else
  return ::pthread_getspecific(handle_.as<pthread_key_t>());
#endif
}

}  // namespace ice
