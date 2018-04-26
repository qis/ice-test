cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(ice VERSION 0.1.0 LANGUAGES CXX)

if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "-(mt|static)$")
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER CMAKE_CXX_FLAGS_${config} name)
    string(REPLACE "/MD" "/MT" ${name} ${${name}})
  endforeach()
endif()

if(MSVC)
  string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /permissive- /await /utf-8")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifestuac:NO /ignore:4098 /ignore:4099")
  add_definitions(-D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_ATL_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_HAS_EXCEPTIONS=_CPPUNWIND)
  add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
  set(AVX2_FLAGS "/arch:AVX2")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -stdlib=libc++ -fcoroutines-ts")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-absolute-paths -fcolor-diagnostics")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
  set(AVX2_FLAGS "-march=native -mavx2")
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GS- /GL")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
  set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
else()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fuse-ld=lld -flto")
endif()

if(MSVC)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER build)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/res/cmake)

option(ENABLE_AVX2 "Enable AVX2 support" OFF)
if(ENABLE_AVX2)
  if(CMAKE_CROSSCOMPILING)
    set(AVX2 TRUE)
  else()
    include(CheckCXXSourceRuns)
    set(CMAKE_REQUIRED_FLAGS "${AVX2_FLAGS}")
    check_cxx_source_runs("
    #include <immintrin.h>
    int main() {
      const auto src = _mm256_set_epi64x(
        0x0002000200020002ULL, 0x0002000200020002ULL,
        0x0002000200020002ULL, 0x0002000200020002ULL);
      const auto sub = _mm256_set_epi64x(
        0x0001000100010001ULL, 0x0001000100010001ULL,
        0x0001000100010001ULL, 0x0001000100010001ULL);
      const auto dst = _mm256_sub_epi16(src, sub);
      alignas(32) short str[16];
      _mm256_store_si256(reinterpret_cast<__m256i*>(str), dst);
      for (auto c : str) {
        if (c != 1) {
          return -1;
        }
      }
    }" AVX2)
  endif()
  if(AVX2)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${AVX2_FLAGS}")
    add_definitions(-DICE_AVX2=1)
  endif()
endif()

try_compile(CHECK_RESULT ${CMAKE_CURRENT_BINARY_DIR} SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/res/check.cpp OUTPUT_VARIABLE CHECK_OUTPUT)
string(REPLACE "\n" ";" CHECK_OUTPUT "${CHECK_OUTPUT}")
set(CHECK_REGEX ".*check<([^,>]+), ?([0-9]+), ?([0-9]+)>.*")
foreach(CHECK_LINE ${CHECK_OUTPUT})
  if(CHECK_LINE MATCHES "${CHECK_REGEX}")
    string(REGEX REPLACE "${CHECK_REGEX}" "\\1;\\2;\\3" CHECK_ENTRY ${CHECK_LINE})
    list(GET CHECK_ENTRY 0 CHECK_TYPE)
    if(NOT ${CHECK_TYPE}_size AND NOT ${CHECK_TYPE}_alignment)
      list(GET CHECK_ENTRY 1 ${CHECK_TYPE}_size)
      list(GET CHECK_ENTRY 2 ${CHECK_TYPE}_alignment)
      message(STATUS "Checking ${CHECK_TYPE}: ${${CHECK_TYPE}_size} ${${CHECK_TYPE}_alignment}")
    endif()
  endif()
endforeach()

file(GLOB_RECURSE headers include/ice/*.h)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/ice PREFIX include FILES ${headers})

file(GLOB_RECURSE sources src/ice/*.h src/ice/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ice PREFIX src FILES ${sources})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/include/ice/config.h
  NEWLINE_STYLE LF)

source_group(include FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/include/ice/config.h)

list(APPEND headers
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/include/ice/config.h)

add_library(ice STATIC ${headers} ${sources})
target_include_directories(ice PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

find_package(LibSSH2 REQUIRED)
target_link_libraries(ice PUBLIC LibSSH2::LibSSH2)

if(WIN32)
  target_link_libraries(ice PUBLIC ws2_32 mswsock)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ice-config.cmake)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ice-config.cmake
    [[include("${CMAKE_CURRENT_LIST_DIR}/ice-targets.cmake")]])
endif()

install(TARGETS ice EXPORT ice DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ice DESTINATION include FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ice DESTINATION include FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ice-config.cmake DESTINATION lib/cmake)
install(EXPORT ice FILE ice-targets.cmake NAMESPACE ice:: DESTINATION lib/cmake)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/ssh.cpp)
  add_executable(main src/ssh.cpp)
  source_group("" FILES src/ssh.cpp)
  target_link_libraries(main PRIVATE ice)
  set_target_properties(main PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
else()
  add_executable(main src/main.cpp)
  source_group("" FILES src/main.cpp)
  target_link_libraries(main PRIVATE ice)
  set_target_properties(main PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

#find_package(benchmark REQUIRED)
#source_group("" FILES src/benchmark.cpp)
#add_executable(benchmark src/benchmark.cpp)
#target_link_libraries(benchmark PRIVATE ice benchmark::benchmark)
#set_target_properties(benchmark PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

#enable_testing()
#find_package(GTest)
#add_executable(tests ${headers} src/test.cpp)
#source_group("" FILES src/test.cpp)
#target_link_libraries(tests PRIVATE ice GTest::GTest GTest::Main)
#set_target_properties(tests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#gtest_add_tests(TARGET tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)
