add_library(obelisk_core
    btree/btree.c
    buffer/buffer_pool.c
    storage/storage_engine.c
    transaction/transaction.c
    parser/parser.c
    utils/utils.c
)

target_include_directories(obelisk_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add compiler definitions
target_compile_definitions(obelisk_core
    PRIVATE
        OBELISK_VERSION="0.1.0"
        OBELISK_PAGE_SIZE=4096
)

# Add dependencies if needed
find_package(Threads REQUIRED)
target_link_libraries(obelisk_core
    PRIVATE
        Threads::Threads
)

# Create subdirectories for organization
add_subdirectory(btree)
add_subdirectory(buffer)
add_subdirectory(storage)
add_subdirectory(transaction)
add_subdirectory(parser)
add_subdirectory(utils) 