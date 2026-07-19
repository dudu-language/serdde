cmake_minimum_required(VERSION 3.20)
project(serdde_native_benchmarks LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json 3.11.3 REQUIRED)

set(GLAZE_INCLUDE_DIR "" CACHE PATH "Path containing glaze/glaze.hpp")
if(NOT EXISTS "${GLAZE_INCLUDE_DIR}/glaze/glaze.hpp")
  message(FATAL_ERROR "Set GLAZE_INCLUDE_DIR to Glaze's include directory")
endif()

set(SIMDJSON_DIR "" CACHE PATH "Path to the simdjson source tree")
if(NOT EXISTS "${SIMDJSON_DIR}/src/simdjson.cpp")
  message(FATAL_ERROR "Set SIMDJSON_DIR to simdjson's source directory")
endif()

set(RAPIDJSON_INCLUDE_DIR "" CACHE PATH "Path containing rapidjson/reader.h")
if(NOT EXISTS "${RAPIDJSON_INCLUDE_DIR}/rapidjson/reader.h")
  message(FATAL_ERROR "Set RAPIDJSON_INCLUDE_DIR to RapidJSON's include directory")
endif()

set(YYJSON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/yyjson")
add_library(benchmark_yyjson_c STATIC "${YYJSON_DIR}/yyjson.c")
target_include_directories(benchmark_yyjson_c PUBLIC "${YYJSON_DIR}")
target_compile_options(benchmark_yyjson_c PRIVATE -O3 -DNDEBUG)

add_library(benchmark_support OBJECT benchmark_alloc.cpp benchmark_fixture_json.cpp)
target_compile_options(benchmark_support PRIVATE -O3 -DNDEBUG)

add_library(benchmark_simdjson_lib STATIC "${SIMDJSON_DIR}/src/simdjson.cpp")
target_include_directories(benchmark_simdjson_lib
  PUBLIC "${SIMDJSON_DIR}/include"
  PRIVATE "${SIMDJSON_DIR}/src"
)
target_compile_options(benchmark_simdjson_lib PRIVATE -O3 -DNDEBUG)

function(configure_benchmark target)
  target_sources(${target} PRIVATE $<TARGET_OBJECTS:benchmark_support>)
  target_compile_options(${target} PRIVATE -O3 -DNDEBUG)
  if(UNIX AND NOT APPLE)
    target_link_options(${target} PRIVATE
      -Wl,--wrap=malloc
      -Wl,--wrap=calloc
      -Wl,--wrap=realloc
      -Wl,--wrap=free
    )
  endif()
endfunction()

add_executable(benchmark_nlohmann nlohmann_backend.cpp)
target_link_libraries(benchmark_nlohmann PRIVATE nlohmann_json::nlohmann_json)
configure_benchmark(benchmark_nlohmann)

add_executable(benchmark_glaze glaze_backend.cpp)
target_include_directories(benchmark_glaze PRIVATE "${GLAZE_INCLUDE_DIR}")
set_property(TARGET benchmark_glaze PROPERTY CXX_STANDARD 23)
configure_benchmark(benchmark_glaze)

add_executable(benchmark_yyjson yyjson_backend.cpp yyjson_codec.cpp)
target_link_libraries(benchmark_yyjson PRIVATE benchmark_yyjson_c)
configure_benchmark(benchmark_yyjson)

add_executable(benchmark_simdjson simdjson_backend.cpp simdjson_codec.cpp)
target_link_libraries(benchmark_simdjson PRIVATE benchmark_simdjson_lib)
configure_benchmark(benchmark_simdjson)

add_executable(benchmark_rapidjson rapidjson_backend.cpp rapidjson_decode.cpp
               rapidjson_encode.cpp)
target_include_directories(benchmark_rapidjson PRIVATE "${RAPIDJSON_INCLUDE_DIR}")
configure_benchmark(benchmark_rapidjson)
