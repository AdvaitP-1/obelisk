#ifndef OBELISK_BUFFER_POOL_H
#define OBELISK_BUFFER_POOL_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct ObeliskBufferPool ObeliskBufferPool;
typedef struct ObeliskPage ObeliskPage;

// Page states
typedef enum {
    OBELISK_PAGE_CLEAN = 0,
    OBELISK_PAGE_DIRTY = 1
} ObeliskPageState;

// Page structure
struct ObeliskPage {
    uint64_t page_id;
    void* data;
    ObeliskPageState state;
    uint32_t pin_count;
    uint64_t last_accessed;
};

// Buffer pool configuration
typedef struct {
    size_t pool_size;           // Number of pages in pool
    size_t page_size;           // Size of each page in bytes
    const char* data_file;      // Path to data file
    bool use_direct_io;         // Use O_DIRECT for I/O
    size_t prefetch_size;       // Number of pages to prefetch
} ObeliskBufferPoolConfig;

// Buffer pool operations
ObeliskBufferPool* buffer_pool_create(const ObeliskBufferPoolConfig* config);
void buffer_pool_destroy(ObeliskBufferPool* pool);

// Page operations
ObeliskPage* buffer_pool_get_page(ObeliskBufferPool* pool, uint64_t page_id);
int buffer_pool_pin_page(ObeliskBufferPool* pool, uint64_t page_id);
int buffer_pool_unpin_page(ObeliskBufferPool* pool, uint64_t page_id, bool is_dirty);
int buffer_pool_flush_page(ObeliskBufferPool* pool, uint64_t page_id);

// Bulk operations
int buffer_pool_flush_all(ObeliskBufferPool* pool);
int buffer_pool_prefetch_pages(ObeliskBufferPool* pool, uint64_t* page_ids, size_t count);

// Statistics and monitoring
typedef struct {
    uint64_t hits;              // Buffer pool hits
    uint64_t misses;            // Buffer pool misses
    uint64_t evictions;         // Number of pages evicted
    uint64_t flushes;          // Number of pages flushed to disk
    double hit_ratio;          // Hit ratio (hits / (hits + misses))
} ObeliskBufferPoolStats;

ObeliskBufferPoolStats buffer_pool_get_stats(ObeliskBufferPool* pool);
void buffer_pool_reset_stats(ObeliskBufferPool* pool);

// Memory management
int buffer_pool_resize(ObeliskBufferPool* pool, size_t new_size);
size_t buffer_pool_get_memory_usage(ObeliskBufferPool* pool);

// Replacement policy configuration
typedef enum {
    OBELISK_POLICY_LRU,
    OBELISK_POLICY_CLOCK,
    OBELISK_POLICY_LFU
} ObeliskReplacementPolicy;

int buffer_pool_set_policy(ObeliskBufferPool* pool, ObeliskReplacementPolicy policy);

#endif // OBELISK_BUFFER_POOL_H 