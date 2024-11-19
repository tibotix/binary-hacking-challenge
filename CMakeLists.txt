cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(CPUE)

set(CAPSTONE_ARCHITECTURE_DEFAULT 0)
set(CAPSTONE_X86_SUPPORT 1)
set(CAPSTONE_BUILD_STATIC_LIBS 1)
add_subdirectory("deps/capstone")

add_subdirectory("deps/ELFIO")


file(GLOB cpue_sources "src/**/*.cpp")
list(APPEND cpue_sources "src/main.cpp")
# Note: To build a shared library, use -DBUILD_SHARED_LIBS=ON when configuring
add_executable(CPUE ${cpue_sources})
target_include_directories(CPUE PUBLIC include)
target_include_directories(CPUE PUBLIC ${capstone_SOURCE_DIR}/include)
target_link_libraries(CPUE PRIVATE capstone elfio)
target_link_options(CPUE PUBLIC -Wl,-z,relro,-z,now)

