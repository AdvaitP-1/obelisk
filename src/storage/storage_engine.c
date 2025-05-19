#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <obelisk/storage.h>
#include <obelisk/db.h>

// Internal storage structure
struct ObeliskStorage {
    char* data_directory;
    size_t page_size;
    bool enable_compression;
    bool enable_encryption;
    char* encryption_key;
    
    // File descriptors for data files
    int* table_fds;
    size_t num_tables;
    
    // Statistics
    ObeliskStorageStats stats;
};

ObeliskStorage* storage_create(const ObeliskStorageConfig* config) {
    if (!config || !config->data_directory) return NULL;

    ObeliskStorage* storage = malloc(sizeof(ObeliskStorage));
    if (!storage) return NULL;

    storage->data_directory = strdup(config->data_directory);
    storage->page_size = config->page_size;
    storage->enable_compression = config->enable_compression;
    storage->enable_encryption = config->enable_encryption;
    storage->encryption_key = config->encryption_key ? strdup(config->encryption_key) : NULL;

    storage->table_fds = NULL;
    storage->num_tables = 0;

    // Create data directory if it doesn't exist
    mkdir(storage->data_directory, 0755);

    // Initialize statistics
    memset(&storage->stats, 0, sizeof(ObeliskStorageStats));

    return storage;
}

void storage_destroy(ObeliskStorage* storage) {
    if (!storage) return;

    // Close all open files
    for (size_t i = 0; i < storage->num_tables; i++) {
        if (storage->table_fds[i] > 0) {
            close(storage->table_fds[i]);
        }
    }

    free(storage->table_fds);
    free(storage->data_directory);
    free(storage->encryption_key);
    free(storage);
}

static char* get_table_path(ObeliskStorage* storage, const char* table_name) {
    size_t path_len = strlen(storage->data_directory) + strlen(table_name) + 6;  // +6 for '/' and '.dat'
    char* path = malloc(path_len);
    if (!path) return NULL;

    snprintf(path, path_len, "%s/%s.dat", storage->data_directory, table_name);
    return path;
}

int storage_create_table(ObeliskStorage* storage, const char* table_name, 
                        const struct ObeliskColumn* columns, size_t num_columns) {
    if (!storage || !table_name || !columns) return -1;

    char* table_path = get_table_path(storage, table_name);
    if (!table_path) return -1;

    // Create table file
    int fd = open(table_path, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        free(table_path);
        return -1;
    }

    // Write table metadata
    ObeliskTableInfo info = {
        .table_name = (char*)table_name,
        .num_columns = num_columns,
        .record_size = 0,  // Calculate based on columns
        .num_records = 0,
        .first_page = 1,
        .last_page = 1
    };

    // Calculate record size
    for (size_t i = 0; i < num_columns; i++) {
        switch (columns[i].type) {
            case OBELISK_TYPE_INT:
                info.record_size += sizeof(int);
                break;
            case OBELISK_TYPE_FLOAT:
                info.record_size += sizeof(double);
                break;
            case OBELISK_TYPE_TEXT:
                info.record_size += 256;  // Fixed size for simplicity
                break;
            case OBELISK_TYPE_BLOB:
                info.record_size += 1024;  // Fixed size for simplicity
                break;
            default:
                break;
        }
    }

    // Write table info
    write(fd, &info, sizeof(ObeliskTableInfo));

    // Add file descriptor to storage
    int* new_fds = realloc(storage->table_fds, (storage->num_tables + 1) * sizeof(int));
    if (!new_fds) {
        close(fd);
        free(table_path);
        return -1;
    }

    storage->table_fds = new_fds;
    storage->table_fds[storage->num_tables++] = fd;

    free(table_path);
    return 0;
}

int storage_drop_table(ObeliskStorage* storage, const char* table_name) {
    if (!storage || !table_name) return -1;

    char* table_path = get_table_path(storage, table_name);
    if (!table_path) return -1;

    // Close file descriptor if open
    for (size_t i = 0; i < storage->num_tables; i++) {
        if (storage->table_fds[i] > 0) {
            close(storage->table_fds[i]);
            storage->table_fds[i] = -1;
        }
    }

    // Delete file
    unlink(table_path);
    free(table_path);

    return 0;
}

ObeliskTableInfo* storage_get_table_info(ObeliskStorage* storage, const char* table_name) {
    if (!storage || !table_name) return NULL;

    char* table_path = get_table_path(storage, table_name);
    if (!table_path) return NULL;

    int fd = open(table_path, O_RDONLY);
    if (fd < 0) {
        free(table_path);
        return NULL;
    }

    ObeliskTableInfo* info = malloc(sizeof(ObeliskTableInfo));
    if (!info) {
        close(fd);
        free(table_path);
        return NULL;
    }

    // Read table info
    ssize_t bytes_read = read(fd, info, sizeof(ObeliskTableInfo));
    if (bytes_read != sizeof(ObeliskTableInfo)) {
        free(info);
        close(fd);
        free(table_path);
        return NULL;
    }

    close(fd);
    free(table_path);
    return info;
}

int storage_insert_record(ObeliskStorage* storage, const char* table_name, const ObeliskRecord* record) {
    if (!storage || !table_name || !record) return -1;

    ObeliskTableInfo* info = storage_get_table_info(storage, table_name);
    if (!info) return -1;

    char* table_path = get_table_path(storage, table_name);
    if (!table_path) {
        free(info);
        return -1;
    }

    int fd = open(table_path, O_WRONLY | O_APPEND);
    if (fd < 0) {
        free(info);
        free(table_path);
        return -1;
    }

    // Write record
    ssize_t bytes_written = write(fd, record, sizeof(ObeliskRecord));
    if (bytes_written != sizeof(ObeliskRecord)) {
        close(fd);
        free(info);
        free(table_path);
        return -1;
    }

    // Update table info
    info->num_records++;
    lseek(fd, 0, SEEK_SET);
    write(fd, info, sizeof(ObeliskTableInfo));

    close(fd);
    free(info);
    free(table_path);
    storage->stats.total_records++;

    return 0;
}

// Placeholder implementations for remaining functions
int storage_update_record(ObeliskStorage* storage, const char* table_name, 
                         uint64_t record_id, const ObeliskRecord* record) {
    // TODO: Implement record update
    return -1;
}

int storage_delete_record(ObeliskStorage* storage, const char* table_name, uint64_t record_id) {
    // TODO: Implement record deletion
    return -1;
}

ObeliskRecord* storage_get_record(ObeliskStorage* storage, const char* table_name, uint64_t record_id) {
    // TODO: Implement record retrieval
    return NULL;
}

void* storage_allocate_page(ObeliskStorage* storage) {
    if (!storage) return NULL;
    return malloc(storage->page_size);
}

int storage_free_page(ObeliskStorage* storage, uint64_t page_id) {
    // TODO: Implement page deallocation
    return -1;
}

int storage_write_page(ObeliskStorage* storage, uint64_t page_id, const void* data) {
    // TODO: Implement page writing
    return -1;
}

int storage_read_page(ObeliskStorage* storage, uint64_t page_id, void* data) {
    // TODO: Implement page reading
    return -1;
}

int storage_vacuum(ObeliskStorage* storage, const char* table_name) {
    // TODO: Implement table vacuuming
    return -1;
}

int storage_analyze(ObeliskStorage* storage, const char* table_name) {
    // TODO: Implement table analysis
    return -1;
}

int storage_checkpoint(ObeliskStorage* storage) {
    // TODO: Implement checkpointing
    return -1;
}

ObeliskStorageStats storage_get_stats(ObeliskStorage* storage) {
    ObeliskStorageStats empty_stats = {0};
    return storage ? storage->stats : empty_stats;
} 