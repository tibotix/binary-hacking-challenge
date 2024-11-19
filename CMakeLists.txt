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
file(GLOB cpue_sources "src/*.cpp") # TODO: reiterate this
# Note: To build a shared library, use -DBUILD_SHARED_LIBS=ON when configuring
add_library(CPUE STATIC ${cpue_sources})
target_include_directories(CPUE PUBLIC include)
target_include_directories(CPUE PUBLIC ${capstone_SOURCE_DIR}/include)
target_link_libraries(CPUE PRIVATE capstone elfio)
target_link_options(CPUE PUBLIC -Wl,-z,relro,-z,now)


file(GLOB cpue_main_sources "./main.cpp") # TODO: reiterate this
add_executable(CPUE_MAIN ${cpue_main_sources})
target_link_libraries(CPUE_MAIN CPUE)


include(CTest)
if(BUILD_TESTING AND (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    add_subdirectory("deps/catch2")
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)

    file(GLOB test_sources "tests/**/*_test.cpp")
    file(GLOB test_sources "tests/*_test.cpp") # TODO: reiterate this
    add_executable(CPUE_TEST ${test_sources})
    target_include_directories(CPUE_TEST PUBLIC src)

    target_link_libraries(CPUE_TEST PRIVATE Catch2::Catch2WithMain)
    target_link_libraries(CPUE_TEST PRIVATE CPUE)

    catch_discover_tests(CPUE_TEST)
endif()