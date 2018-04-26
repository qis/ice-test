#pragma once
#include <system_error>

namespace ice::net::ssh {

const std::error_category& error_category() noexcept;

class domain_error : public std::system_error {
public:
  domain_error(int code) : std::system_error(code, error_category()) {
  }

  domain_error(int code, const std::string& what) : std::system_error(code, error_category(), what) {
  }

  domain_error(int code, const char* what) : std::system_error(code, error_category(), what) {
  }
};

}  // namespace ice::net::ssh
