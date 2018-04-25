# ICE
Lightweight C++17 framework.

## Requirements
* [CMake](https://cmake.org/download/) version 3.10
* [LLVM](https://llvm.org/) and [libcxx](https://libcxx.llvm.org/) version 6.0.0 on FreeBSD
* [Visual Studio](https://www.visualstudio.com/downloads/) version 15.7 on Windows
* [Vcpkg](https://github.com/Microsoft/vcpkg)

## Dependencies
Install Vcpkg using the following instructions. Adjust the commands as necessary or desired.

### Windows
Set the `VCPKG_DEFAULT_TRIPLET` environment variable to `x64-windows-static-md`.<br/>
Set the `VCPKG` environment variable to `C:/Libraries/vcpkg/scripts/buildsystems/vcpkg.cmake`.<br/>
Add `C:\Libraries\vcpkg\installed\%VCPKG_DEFAULT_TRIPLET%\bin` to the `PATH` environment variable.<br/>
Add `C:\Libraries\vcpkg` to the `PATH` environment variable.

```cmd
git clone https://github.com/Microsoft/vcpkg C:\Libraries\vcpkg
cd C:\Libraries\vcpkg && bootstrap-vcpkg.bat && vcpkg integrate install
```

### FreeBSD
Set the `VCPKG_DEFAULT_TRIPLET` environment variable to `x64-freebsd`.<br/>
Set the `VCPKG` environment variable to `/opt/vcpkg/scripts/buildsystems/vcpkg.cmake`.<br/>
Add `/opt/vcpkg/bin` to the `PATH` environment variable.

```sh
git clone https://github.com/Microsoft/vcpkg /opt/vcpkg
mkdir /opt/vcpkg/bin && cd /opt/vcpkg/bin
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ../toolsrc \
  -DCMAKE_C_COMPILER=`which clang-devel | which clang` \
  -DCMAKE_CXX_COMPILER=`which clang++-devel | which clang++`
cmake --build .
cat > ../triplets/x64-freebsd.cmake <<EOF
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME FreeBSD)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "\${CMAKE_CURRENT_LIST_DIR}/toolchains/freebsd-toolchain.cmake")
EOF
mkdir ../triplets/toolchains
cat > ../triplets/toolchains/freebsd-toolchain.cmake <<EOF
set(CMAKE_CROSSCOMPILING FALSE)
set(CMAKE_SYSTEM_NAME FreeBSD CACHE STRING "")
set(CMAKE_C_COMPILER `which clang-devel | which clang` CACHE STRING "")
set(CMAKE_CXX_COMPILER `which clang++-devel | which clang++` CACHE STRING "")
set(CMAKE_CXX_FLAGS "\${CMAKE_CXX_FLAGS} -stdlib=libc++")
EOF
```

### Ports
Install Vcpkg ports.

```
vcpkg install benchmark --head
vcpkg install gtest
```

## Build
Execute [solution.cmd](solution.cmd) to configure the project with cmake and open it in Visual Studio.<br/>
Execute `make run`, `make test` or `make benchmark` in the project directory on Unix systems.
