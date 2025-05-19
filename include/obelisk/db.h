#ifndef OBELISK_DB_H
#define OBELISK_DB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // For size_t

#define OBELISK_OK 0
#define OBELISK_ERROR -1
#define OBELISK_PAGE_SIZE 4096
#define OBELISK_MAX_KEY_SIZE 1024
#define OBELISK_MAX_VALUE_SIZE 65536

// Forward declarations
typedef struct ObeliskDB ObeliskDB;
typedef struct ObeliskTransaction ObeliskTransaction;
typedef struct ObeliskResult ObeliskResult;
typedef struct ObeliskCursor ObeliskCursor;

// Database configuration
typedef struct {
    const char* db_path;
    size_t cache_size;      // Buffer pool size in pages
    bool sync_writes;       // Force sync on writes
    size_t wal_size;       // Write-ahead log size
} ObeliskConfig;

// Column types
typedef enum {
    OBELISK_TYPE_NULL,
    OBELISK_TYPE_INT,
    OBELISK_TYPE_FLOAT,
    OBELISK_TYPE_TEXT,
    OBELISK_TYPE_BLOB
} ObeliskDataType;

// Column definition
typedef struct {
    char* name;
    ObeliskDataType type;
    bool is_primary_key;
    bool is_nullable;
    bool is_unique;
} ObeliskColumn;

// Table schema
typedef struct {
    char* table_name;
    ObeliskColumn* columns;
    size_t num_columns;
} ObeliskSchema;

// Database operations
ObeliskDB* obelisk_open(const char* path);
ObeliskDB* obelisk_open_with_config(const ObeliskConfig* config);
int obelisk_close(ObeliskDB* db);

// Transaction management
ObeliskTransaction* obelisk_transaction_begin(ObeliskDB* db);
int obelisk_transaction_commit(ObeliskTransaction* txn);
int obelisk_transaction_rollback(ObeliskTransaction* txn);

// Schema operations
int obelisk_create_table(ObeliskDB* db, const ObeliskSchema* schema);
int obelisk_drop_table(ObeliskDB* db, const char* table_name);
ObeliskSchema* obelisk_get_schema(ObeliskDB* db, const char* table_name);

// Query execution
ObeliskResult* obelisk_query(ObeliskDB* db, const char* query);
int obelisk_exec(ObeliskDB* db, const char* sql);

// Result set operations
bool obelisk_result_next(ObeliskResult* result);
int obelisk_result_get_int(ObeliskResult* result, size_t column);
double obelisk_result_get_float(ObeliskResult* result, size_t column);
const char* obelisk_result_get_text(ObeliskResult* result, size_t column);
void obelisk_result_free(ObeliskResult* result);

// Cursor operations for direct table access
ObeliskCursor* obelisk_cursor_open(ObeliskDB* db, const char* table_name);
bool obelisk_cursor_next(ObeliskCursor* cursor);
int obelisk_cursor_seek(ObeliskCursor* cursor, const void* key, size_t key_size);
void obelisk_cursor_close(ObeliskCursor* cursor);

// Error handling
const char* obelisk_error_string(ObeliskDB* db);
int obelisk_error_code(ObeliskDB* db);

#endif // OBELISK_DB_H 