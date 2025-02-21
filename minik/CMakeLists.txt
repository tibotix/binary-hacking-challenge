cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_COMPILER /usr/bin/tcc)
set(CMAKE_LINKER /usr/bin/tcc)

project(minik)

set(CMAKE_ASM_NASM_COMPILER nasm)
enable_language(ASM_NASM)
file(GLOB ASM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/asm/*.S")
set(ASM_OBJECTS)
foreach(ASM_FILE ${ASM_SOURCES})
    get_filename_component(ASM_NAME ${ASM_FILE} NAME_WE)
    set(ASM_OBJ "${CMAKE_BINARY_DIR}/${ASM_NAME}.o")
    add_custom_command(
            OUTPUT ${ASM_OBJ}
            COMMAND ${CMAKE_ASM_NASM_COMPILER} -f elf64 -o "${ASM_OBJ}" "${ASM_FILE}"
            DEPENDS ${ASM_FILE}
            COMMENT "Assembling ${ASM_NAME}.o"
    )
    list(APPEND ASM_OBJECTS ${ASM_OBJ})
endforeach()

file(GLOB minik_sources "src/**/*.c" "src/*.c")
add_library(minik STATIC ${minik_sources} ${ASM_OBJECTS})
target_include_directories(minik PUBLIC include)
target_compile_options(minik PUBLIC -std=c11)
target_link_options(minik PUBLIC -nostdlib -static)



# Build demos
option(BUILD_DEMOS "Build demo kernel images" OFF)
if (BUILD_DEMOS AND (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR))
    file(GLOB DEMOS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/demos/*.c")
    foreach(DEMO_FILE ${DEMOS_SOURCES})
        get_filename_component(DEMO_NAME ${DEMO_FILE} NAME_WE)
        add_executable(${DEMO_NAME} ${DEMO_FILE})
        target_link_libraries(${DEMO_NAME} minik)
        target_include_directories(${DEMO_NAME} PUBLIC ${minik_SOURCE_DIR}/include)
        target_compile_options(${DEMO_NAME} PUBLIC -std=c11)
        target_link_options(${DEMO_NAME} PUBLIC -nostdlib -static)
        set_target_properties(${DEMO_NAME} PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demos"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demos"
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demos"
        )
    endforeach()
endif()


# Build kernel-img
add_executable(minik-img "minik-img.c")
target_link_libraries(minik-img minik)
target_include_directories(minik-img PUBLIC ${minik_SOURCE_DIR}/include)
target_compile_options(minik-img PUBLIC -std=c11)
target_link_options(minik-img PUBLIC -nostdlib -static -Wl,-image-base=0xffffffffff000000)
