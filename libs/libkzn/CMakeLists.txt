cmake_minimum_required(VERSION 3.22)
project(libkzn VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(libkzn STATIC
    src/KznFormat.cpp
    src/KznVerifier.cpp
    src/KznSigningClient.cpp
    src/KznSessionCert.cpp
    src/KznCanonicalJson.cpp
    vendor/monocypher/monocypher.c
    vendor/monocypher/monocypher-ed25519.c
)

target_include_directories(libkzn PUBLIC include)
target_include_directories(libkzn PRIVATE vendor/monocypher src)

# Tests
option(LIBKZN_BUILD_TESTS "Build libkzn tests" ON)
if(LIBKZN_BUILD_TESTS)
    add_subdirectory(tests)
endif()
