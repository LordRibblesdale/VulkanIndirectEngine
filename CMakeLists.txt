cmake_minimum_required(VERSION 3.20)
project(IndirectEngine)

set(CMAKE_CXX_STANDARD 20)
set(EXECUTABLE "${PROJECT_NAME}")

message("Welcome to VulkanIndirectEngine CMake script - CMakeVersion ${CMAKE_VERSION} C++${CMAKE_CXX_STANDARD}")

message("- Locating source files...")
include_directories(include)

file(GLOB_RECURSE src "src/*.cpp")
file(GLOB_RECURSE include "include/*.hpp")
file(GLOB_RECURSE test "test/*.cpp" "test/*.hpp")

set(STATIC_LIB ${PROJECT_NAME}_static)
set(SHARED_LIB ${PROJECT_NAME}_shared)

message("- Adding source files into static library")
add_library(${STATIC_LIB} STATIC ${src} ${include})

message("- Adding source files into dynamic library")
add_library(${SHARED_LIB} SHARED ${src} ${include})

message("- Adding source files into test project...")
add_executable(${PROJECT_NAME}
        ${src}
        ${include}
        ${test})

message("")

message("- Setting up libraries...")
if (WIN32)
    message("- Windows environment detected...")

    set(LIBRARIES_LIST libglfw3.dll.a libvulkan.dll.a libpugixml.dll.a libshaderc_shared.dll.a)
else()
    message("- UNIX environment detected...")

    set(LIBRARIES_LIST libglfw.so libvulkan.so libpugixml.so libshaderc_shared.so)
endif()

message("-> Linking libraries...")
foreach(LIB IN LISTS LIBRARIES_LIST)
    message("-- Library ${LIB}")
    target_link_libraries(${PROJECT_NAME} ${LIB})
    target_link_libraries(${STATIC_LIB} ${LIB})
    target_link_libraries(${SHARED_LIB} ${LIB})
endforeach()

message("")

message("- Copying needed data for execution (ONLY FOR TESTING PURPOSES)...")
if (WIN32)
    set(EXECUTABLE "${EXECUTABLE}.exe")
endif()

file(COPY "${CMAKE_SOURCE_DIR}/build/data" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/build/languages" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/build/shaders" DESTINATION "${CMAKE_BINARY_DIR}")