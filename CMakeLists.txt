cmake_minimum_required(VERSION 3.20)
project(IndirectEngine)

set(CMAKE_CXX_STANDARD 20)

message("Welcome to IndirectEngine CMake script - CMakeVersion ${CMAKE_VERSION} CPPStandard ${CMAKE_CXX_STANDARD}")

message("- Locating source files...")
file(GLOB_RECURSE engine "engine/*.cpp" "engine/*.hpp")
file(GLOB_RECURSE test "test/*.cpp" "test/*.hpp")

set(STATIC_LIB ${PROJECT_NAME}_static)
set(SHARED_LIB ${PROJECT_NAME}_shared)

message("- Adding source files into static library")
add_library(${STATIC_LIB} STATIC ${engine})

message("- Adding source files into dynamic library")
add_library(${SHARED_LIB} SHARED ${engine})

message("- Adding source files into test project...")
add_executable(${PROJECT_NAME}
        ${engine}
        ${test})

message("")

if (WIN32)
    message("- Windows environment detected...")
    message("- Setting up libraries...")

    set(LIBRARIES_LIST libglfw3.dll.a libvulkan.dll.a libpugixml.dll.a libshaderc_shared.dll.a)

    if (MSVC)
        # TODO to be tested
        message("-> Setting up logger function name parser...")
        add_compile_definitions(__PRETTY_FUNCTION__=__FUNCSIG__)
    endif()
else()
    # TODO check here
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
file(COPY "test/Data" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "test/Languages" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "shaders" DESTINATION "${CMAKE_BINARY_DIR}")

message("")