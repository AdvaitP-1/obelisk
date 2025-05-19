#include <stdlib.h>
#include <string.h>
#include <obelisk/buffer_pool.h>

// Internal buffer pool structure
struct ObeliskBufferPool {
    ObeliskPage* pages;
    size_t pool_size;
    size_t page_size;
    const char* data_file;
    bool use_direct_io;
    size_t prefetch_size;
    ObeliskReplacementPolicy policy;
    
    // Statistics
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    uint64_t flushes;
};

ObeliskBufferPool* buffer_pool_create(const ObeliskBufferPoolConfig* config) {
    if (!config) return NULL;

    ObeliskBufferPool* pool = malloc(sizeof(ObeliskBufferPool));
    if (!pool) return NULL;

    pool->pages = calloc(config->pool_size, sizeof(ObeliskPage));
    if (!pool->pages) {
        free(pool);
        return NULL;
    }

    pool->pool_size = config->pool_size;
    pool->page_size = config->page_size;
    pool->data_file = strdup(config->data_file);
    pool->use_direct_io = config->use_direct_io;
    pool->prefetch_size = config->prefetch_size;
    pool->policy = OBELISK_POLICY_LRU;

    // Initialize statistics
    pool->hits = 0;
    pool->misses = 0;
    pool->evictions = 0;
    pool->flushes = 0;

    // Initialize pages
    for (size_t i = 0; i < pool->pool_size; i++) {
        pool->pages[i].data = malloc(pool->page_size);
        if (!pool->pages[i].data) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                free(pool->pages[j].data);
            }
            free(pool->pages);
            free((void*)pool->data_file);
            free(pool);
            return NULL;
        }
        pool->pages[i].page_id = 0;
        pool->pages[i].state = OBELISK_PAGE_CLEAN;
        pool->pages[i].pin_count = 0;
        pool->pages[i].last_accessed = 0;
    }

    return pool;
}

void buffer_pool_destroy(ObeliskBufferPool* pool) {
    if (!pool) return;

    if (pool->pages) {
        for (size_t i = 0; i < pool->pool_size; i++) {
            free(pool->pages[i].data);
        }
        free(pool->pages);
    }

    free((void*)pool->data_file);
    free(pool);
}

static ObeliskPage* find_page(ObeliskBufferPool* pool, uint64_t page_id) {
    for (size_t i = 0; i < pool->pool_size; i++) {
        if (pool->pages[i].page_id == page_id) {
            return &pool->pages[i];
        }
    }
    return NULL;
}

static ObeliskPage* find_victim_page(ObeliskBufferPool* pool) {
    // Simple clock algorithm
    static size_t clock_hand = 0;
    size_t start = clock_hand;

    do {
        ObeliskPage* page = &pool->pages[clock_hand];
        if (page->pin_count == 0) {
            if (page->state == OBELISK_PAGE_CLEAN) {
                return page;
            }
        }
        clock_hand = (clock_hand + 1) % pool->pool_size;
    } while (clock_hand != start);

    return NULL;  // No victim found
}

ObeliskPage* buffer_pool_get_page(ObeliskBufferPool* pool, uint64_t page_id) {
    if (!pool) return NULL;

    // Check if page is in pool
    ObeliskPage* page = find_page(pool, page_id);
    if (page) {
        pool->hits++;
        page->last_accessed = pool->hits + pool->misses;
        return page;
    }

    pool->misses++;

    // Find a victim page
    page = find_victim_page(pool);
    if (!page) {
        return NULL;  // No available pages
    }

    // If victim is dirty, write it back
    if (page->state == OBELISK_PAGE_DIRTY) {
        // TODO: Write page to disk
        pool->flushes++;
    }

    // Load new page
    page->page_id = page_id;
    page->state = OBELISK_PAGE_CLEAN;
    // TODO: Read page from disk

    return page;
}

int buffer_pool_pin_page(ObeliskBufferPool* pool, uint64_t page_id) {
    if (!pool) return -1;

    ObeliskPage* page = find_page(pool, page_id);
    if (!page) return -1;

    page->pin_count++;
    return 0;
}

int buffer_pool_unpin_page(ObeliskBufferPool* pool, uint64_t page_id, bool is_dirty) {
    if (!pool) return -1;

    ObeliskPage* page = find_page(pool, page_id);
    if (!page || page->pin_count == 0) return -1;

    page->pin_count--;
    if (is_dirty) {
        page->state = OBELISK_PAGE_DIRTY;
    }
    return 0;
}

int buffer_pool_flush_page(ObeliskBufferPool* pool, uint64_t page_id) {
    if (!pool) return -1;

    ObeliskPage* page = find_page(pool, page_id);
    if (!page) return -1;

    if (page->state == OBELISK_PAGE_DIRTY) {
        // TODO: Write page to disk
        page->state = OBELISK_PAGE_CLEAN;
        pool->flushes++;
    }

    return 0;
}

int buffer_pool_flush_all(ObeliskBufferPool* pool) {
    if (!pool) return -1;

    for (size_t i = 0; i < pool->pool_size; i++) {
        if (pool->pages[i].state == OBELISK_PAGE_DIRTY) {
            // TODO: Write page to disk
            pool->pages[i].state = OBELISK_PAGE_CLEAN;
            pool->flushes++;
        }
    }

    return 0;
}

int buffer_pool_prefetch_pages(ObeliskBufferPool* pool, uint64_t* page_ids, size_t count) {
    if (!pool || !page_ids) return -1;

    for (size_t i = 0; i < count; i++) {
        buffer_pool_get_page(pool, page_ids[i]);
    }

    return 0;
}

ObeliskBufferPoolStats buffer_pool_get_stats(ObeliskBufferPool* pool) {
    ObeliskBufferPoolStats stats = {0};
    if (!pool) return stats;

    stats.hits = pool->hits;
    stats.misses = pool->misses;
    stats.evictions = pool->evictions;
    stats.flushes = pool->flushes;
    stats.hit_ratio = (double)pool->hits / (pool->hits + pool->misses);

    return stats;
}

void buffer_pool_reset_stats(ObeliskBufferPool* pool) {
    if (!pool) return;

    pool->hits = 0;
    pool->misses = 0;
    pool->evictions = 0;
    pool->flushes = 0;
}

int buffer_pool_resize(ObeliskBufferPool* pool, size_t new_size) {
    if (!pool || new_size == 0) return -1;

    // TODO: Implement pool resizing
    return -1;
}

size_t buffer_pool_get_memory_usage(ObeliskBufferPool* pool) {
    if (!pool) return 0;

    return pool->pool_size * pool->page_size + sizeof(ObeliskBufferPool);
}

int buffer_pool_set_policy(ObeliskBufferPool* pool, ObeliskReplacementPolicy policy) {
    if (!pool) return -1;

    pool->policy = policy;
    return 0;
} 