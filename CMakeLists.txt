cmake_minimum_required(VERSION 3.10)
project(obelisk C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add source directory
add_subdirectory(src)

# Create the library
add_library(obelisk
    src/btree/btree.c
    src/buffer/buffer_pool.c
    src/storage/storage_engine.c
    src/transaction/transaction.c
    src/parser/parser.c
    src/utils/utils.c
)

target_include_directories(obelisk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Create examples
add_executable(example examples/main.c)
target_link_libraries(example PRIVATE obelisk) 