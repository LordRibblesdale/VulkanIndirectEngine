cmake_minimum_required(VERSION 3.20)
project(IndirectEngine)

set(CMAKE_CXX_STANDARD 20)

message("Welcome to IndirectEngine CMake script - CMakeVersion ${CMAKE_VERSION} CPPStandard ${CMAKE_CXX_STANDARD}")

message("- Locating source files...")
file(GLOB_RECURSE engine "engine/*.cpp" "engine/*.hpp")
file(GLOB_RECURSE xml "xml/*.cpp" "xml.hpp")
file(GLOB_RECURSE system "system/*.hpp")

message("- Adding source files into project...")
add_executable(${PROJECT_NAME}
        ${engine}
        ${xml}
        ${system}
        Main.cpp)

message("")

if (WIN32)
    message("- Windows environment detected...")
    message("-> Linking GLFW libraries...")
    target_link_libraries(${PROJECT_NAME} libglfw3.dll.a)

    message("-> Linking Vulkan libraries...")
    target_link_libraries(${PROJECT_NAME} libvulkan.dll.a)

    message("-> Linking PugiXML libraries...")
    target_link_libraries(${PROJECT_NAME} libpugixml.dll.a)

    if (MSVC)
        # TODO to be tested
        message("-> Setting up logger function name parser...")
        add_compile_definitions(__PRETTY_FUNCTION__=__FUNCSIG__)
    endif()
else()
    # TODO check here
    message("- UNIX environment detected...")

    message("-> Linking GLFW library...")
    target_link_libraries(${PROJECT_NAME} libglfw.so)

    message("-> Linking Vulkan libraries...")
    target_link_libraries(${PROJECT_NAME} libvulkan.so)

    message("-> Linking PugiXML libraries...")
    target_link_libraries(${PROJECT_NAME} libpugixml.so)
endif()

message("")

message("- Copying needed data for execution...")
file(COPY "Data" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "Languages" DESTINATION "${CMAKE_BINARY_DIR}")

message("")