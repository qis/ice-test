MAKEFLAGS += --no-print-directory

CC	!= which clang-devel || which clang
CXX	!= which clang++-devel || which clang++
FORMAT	!= which clang-format-devel || which clang-format
DBG	!= which lldb || which gdb
CMAKE	:= CC=$(CC) CXX=$(CXX) cmake -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON #-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
BUILD	!= echo $(PWD) | tr '/' '-' | sed 's|^-|/var/build/|'
PROJECT	!= grep "^project" CMakeLists.txt | cut -c9- | cut -d " " -f1 | tr "[:upper:]" "[:lower:]"
HEADERS	!= find include -type f -name '*.h'
SOURCES	!= find src -type f -name '*.h' -or -name '*.cpp'

all: debug

benchmark: build/llvm/release/CMakeCache.txt
	@cd build/llvm/release && cmake --build . --target benchmark && ./benchmark

run: build/llvm/debug/CMakeCache.txt
	@@cd build/llvm/debug && cmake --build . --target main && ./main

dbg: build/llvm/debug/CMakeCache.txt
	@@cd build/llvm/debug && cmake --build . --target main && $(DBG) ./main

test: build/llvm/debug/CMakeCache.txt
	@cd build/llvm/debug && cmake --build . --target tests && ctest

tidy:
	@clang-tidy -p build/llvm/debug $(SOURCES) -header-filter=src

format:
	@$(FORMAT) -i $(HEADERS) $(SOURCES)

install: release
	@cmake --build build/llvm/release --target install

debug: build/llvm/debug/CMakeCache.txt $(HEADERS) $(SOURCES)
	@cmake --build build/llvm/debug

release: build/llvm/release/CMakeCache.txt $(HEADERS) $(SOURCES)
	@cmake --build build/llvm/release

build/llvm/debug/CMakeCache.txt: CMakeLists.txt build/llvm/debug
	@cd build/llvm/debug && $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
	  -DCMAKE_TOOLCHAIN_FILE:PATH=${VCPKG} -DVCPKG_TARGET_TRIPLET=${VCPKG_DEFAULT_TRIPLET} \
	  -DCMAKE_INSTALL_PREFIX:PATH=$(PWD) $(PWD)

build/llvm/release/CMakeCache.txt: CMakeLists.txt build/llvm/release
	@cd build/llvm/release && $(CMAKE) -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_TOOLCHAIN_FILE:PATH=${VCPKG} -DVCPKG_TARGET_TRIPLET=${VCPKG_DEFAULT_TRIPLET} \
	  -DCMAKE_INSTALL_PREFIX:PATH=$(PWD) $(PWD)

build/llvm/debug:
	@mkdir -p build/llvm/debug

build/llvm/release:
	@mkdir -p build/llvm/release

clean:
	@rm -rf build/llvm bin lib

.PHONY: all run dbg test tidy format install debug release clean
