cmake_minimum_required(VERSION 3.20)
project(IndirectEngine)

set(CMAKE_CXX_STANDARD 20)

message("Welcome to IndirectEngine CMake script - CMakeVersion ${CMAKE_VERSION} CPPStandard ${CMAKE_CXX_STANDARD}")

message("- Locating source files...")
file(GLOB_RECURSE engine "engine/*.cpp" "engine/*.hpp")
file(GLOB_RECURSE test "test/*.cpp" "test/*.hpp")

message("- Adding source files into static library")
add_library(${PROJECT_NAME}_STATIC_LIB STATIC ${engine})

message("- Adding source files into dynamic library")
add_library(${PROJECT_NAME}_SHARED_LIB SHARED ${engine})

message("- Adding source files into project...")
add_executable(${PROJECT_NAME}
        ${engine}
        ${test})

message("")

if (WIN32)
    message("- Windows environment detected...")
    message("-> Linking GLFW libraries...")
    target_link_libraries(${PROJECT_NAME} libglfw3.dll.a)
    target_link_libraries(${PROJECT_NAME}_STATIC_LIB libglfw3.dll.a)
    target_link_libraries(${PROJECT_NAME}_SHARED_LIB libglfw3.dll.a)

    message("-> Linking Vulkan libraries...")
    target_link_libraries(${PROJECT_NAME} libvulkan.dll.a)
    target_link_libraries(${PROJECT_NAME}_STATIC_LIB libvulkan.dll.a)
    target_link_libraries(${PROJECT_NAME}_SHARED_LIB libvulkan.dll.a)

    message("-> Linking PugiXML libraries...")
    target_link_libraries(${PROJECT_NAME} libpugixml.dll.a)
    target_link_libraries(${PROJECT_NAME}_STATIC_LIB libpugixml.dll.a)
    target_link_libraries(${PROJECT_NAME}_SHARED_LIB libpugixml.dll.a)

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
    target_link_libraries(${PROJECT_NAME}_STATIC_LIB libglfw.so)
    target_link_libraries(${PROJECT_NAME}_SHARED_LIB libglfw.so)

    message("-> Linking Vulkan libraries...")
    target_link_libraries(${PROJECT_NAME} libvulkan.so)
    target_link_libraries(${PROJECT_NAME}_STATIC_LIB libvulkan.so)
    target_link_libraries(${PROJECT_NAME}_SHARED_LIB libvulkan.so)

    message("-> Linking PugiXML libraries...")
    target_link_libraries(${PROJECT_NAME} libpugixml.so)
    target_link_libraries(${PROJECT_NAME}_STATIC_LIB libpugixml.so)
    target_link_libraries(${PROJECT_NAME}_SHARED_LIB libpugixml.so)
endif()

message("")

message("- Copying needed data for execution...")
file(COPY "test/Data" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "test/Languages" DESTINATION "${CMAKE_BINARY_DIR}")

message("")