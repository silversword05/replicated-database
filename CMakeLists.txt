cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(replicated-database)

find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Threads)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(COMPILE_FEATURES cxx_std_20)
set(COMPILE_OPTIONS -Wall -Wextra -g3 -pedantic)

add_subdirectory(proto)
add_subdirectory(test)

set(SERVER_EXECUTABLE_NAME raft-service)
add_executable(${SERVER_EXECUTABLE_NAME} raft-grpc.cc)
target_compile_options(${SERVER_EXECUTABLE_NAME} PRIVATE ${COMPILE_OPTIONS})
target_compile_features(${SERVER_EXECUTABLE_NAME} PRIVATE ${COMPILE_FEATURES})
target_include_directories(${SERVER_EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(${SERVER_EXECUTABLE_NAME} PRIVATE myproto)