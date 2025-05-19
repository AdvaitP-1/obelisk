#ifndef OBELISK_STORAGE_H
#define OBELISK_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct ObeliskStorage ObeliskStorage;
typedef struct ObeliskTableInfo ObeliskTableInfo;
typedef struct ObeliskRecord ObeliskRecord;

// Storage engine configuration
typedef struct {
    const char* data_directory;     // Directory for data files
    size_t page_size;              // Page size in bytes
    bool enable_compression;        // Enable page compression
    bool enable_encryption;         // Enable page encryption
    const char* encryption_key;     // Encryption key if enabled
} ObeliskStorageConfig;

// Table information
struct ObeliskTableInfo {
    char* table_name;
    uint32_t num_columns;
    uint32_t record_size;
    uint64_t num_records;
    uint64_t first_page;
    uint64_t last_page;
};

// Record structure
struct ObeliskRecord {
    uint64_t record_id;
    void* data;
    size_t size;
    bool is_deleted;
    uint64_t timestamp;
};

// Page header structure (stored at the beginning of each page)
typedef struct {
    uint64_t page_id;
    uint32_t num_records;
    uint32_t free_space;
    uint64_t next_page;
    uint64_t prev_page;
    uint32_t checksum;
    uint8_t flags;
} ObeliskPageHeader;

// Storage engine operations
ObeliskStorage* storage_create(const ObeliskStorageConfig* config);
void storage_destroy(ObeliskStorage* storage);

// Table operations
int storage_create_table(ObeliskStorage* storage, const char* table_name, const struct ObeliskColumn* columns, size_t num_columns);
int storage_drop_table(ObeliskStorage* storage, const char* table_name);
ObeliskTableInfo* storage_get_table_info(ObeliskStorage* storage, const char* table_name);

// Record operations
int storage_insert_record(ObeliskStorage* storage, const char* table_name, const ObeliskRecord* record);
int storage_update_record(ObeliskStorage* storage, const char* table_name, uint64_t record_id, const ObeliskRecord* record);
int storage_delete_record(ObeliskStorage* storage, const char* table_name, uint64_t record_id);
ObeliskRecord* storage_get_record(ObeliskStorage* storage, const char* table_name, uint64_t record_id);

// Page operations
void* storage_allocate_page(ObeliskStorage* storage);
int storage_free_page(ObeliskStorage* storage, uint64_t page_id);
int storage_write_page(ObeliskStorage* storage, uint64_t page_id, const void* data);
int storage_read_page(ObeliskStorage* storage, uint64_t page_id, void* data);

// Maintenance operations
int storage_vacuum(ObeliskStorage* storage, const char* table_name);
int storage_analyze(ObeliskStorage* storage, const char* table_name);
int storage_checkpoint(ObeliskStorage* storage);

// Statistics
typedef struct {
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t total_records;
    uint64_t deleted_records;
    uint64_t disk_usage;
} ObeliskStorageStats;

ObeliskStorageStats storage_get_stats(ObeliskStorage* storage);

#endif // OBELISK_STORAGE_H 