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
add_subdirectory("deps/spdlog")


option(NO_LOGGING "Disable CPUE logging" OFF)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(NO_LOGGING ON CACHE BOOL "Disable logging functionality on Release builds" FORCE)
endif ()


file(GLOB cpue_sources "src/**/*.cpp" "src/*.cpp")
# Note: To build a shared library, use -DBUILD_SHARED_LIBS=ON when configuring
add_library(cpue STATIC ${cpue_sources})
target_include_directories(cpue PUBLIC include)
target_include_directories(cpue PUBLIC ${capstone_SOURCE_DIR}/include)
target_include_directories(cpue PUBLIC ${spdlog_SOURCE_DIR}/include)
target_link_libraries(cpue PRIVATE capstone elfio spdlog::spdlog)
target_link_options(cpue PUBLIC -Wl,-z,relro,-z,now)
target_compile_definitions(cpue PRIVATE $<$<BOOL:${NO_LOGGING}>:CPUE_NO_LOGGING>)


set(cpue_main_sources "./main.cpp")
add_executable(cpue_main ${cpue_main_sources})
target_link_libraries(cpue_main cpue)
set_target_properties(cpue_main PROPERTIES OUTPUT_NAME "cpue")
target_compile_definitions(cpue_main PRIVATE $<$<BOOL:${NO_LOGGING}>:CPUE_NO_LOGGING>)


if (BUILD_TESTING AND (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    include(CTest)
    add_subdirectory("deps/catch2")
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)

    file(GLOB test_sources "tests/src/**/*_test.cpp" "tests/src/*_test.cpp")
    add_executable(cpue_test ${test_sources})
    target_include_directories(cpue_test PUBLIC include)
    target_include_directories(cpue_test PUBLIC tests/include)

    target_link_libraries(cpue_test PRIVATE Catch2::Catch2WithMain)
    target_link_libraries(cpue_test PRIVATE cpue)
    target_compile_definitions(cpue_test PRIVATE $<$<BOOL:${NO_LOGGING}>:CPUE_NO_LOGGING>)

    catch_discover_tests(cpue_test)
endif ()