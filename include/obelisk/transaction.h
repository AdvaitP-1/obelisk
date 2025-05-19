#ifndef OBELISK_TRANSACTION_H
#define OBELISK_TRANSACTION_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct ObeliskTransactionManager ObeliskTransactionManager;
typedef struct ObeliskTransaction ObeliskTransaction;
typedef struct ObeliskLockManager ObeliskLockManager;

// Transaction states
typedef enum {
    OBELISK_TXN_ACTIVE,
    OBELISK_TXN_COMMITTED,
    OBELISK_TXN_ABORTED,
    OBELISK_TXN_PREPARED  // For two-phase commit
} ObeliskTransactionState;

// Lock modes
typedef enum {
    OBELISK_LOCK_SHARED,
    OBELISK_LOCK_EXCLUSIVE
} ObeliskLockMode;

// Transaction isolation levels
typedef enum {
    OBELISK_ISOLATION_READ_UNCOMMITTED,
    OBELISK_ISOLATION_READ_COMMITTED,
    OBELISK_ISOLATION_REPEATABLE_READ,
    OBELISK_ISOLATION_SERIALIZABLE
} ObeliskIsolationLevel;

// Write-ahead log record types
typedef enum {
    OBELISK_LOG_BEGIN,
    OBELISK_LOG_COMMIT,
    OBELISK_LOG_ABORT,
    OBELISK_LOG_UPDATE,
    OBELISK_LOG_INSERT,
    OBELISK_LOG_DELETE,
    OBELISK_LOG_CHECKPOINT
} ObeliskLogRecordType;

// Write-ahead log record
typedef struct {
    ObeliskLogRecordType type;
    uint64_t txn_id;
    uint64_t page_id;
    uint32_t offset;
    uint32_t length;
    void* before_image;
    void* after_image;
    uint64_t timestamp;
    uint32_t checksum;
} ObeliskLogRecord;

// Transaction manager configuration
typedef struct {
    const char* log_directory;
    size_t log_buffer_size;
    bool sync_commit;
    uint32_t checkpoint_interval;
} ObeliskTransactionConfig;

// Transaction manager operations
ObeliskTransactionManager* txn_manager_create(const ObeliskTransactionConfig* config);
void txn_manager_destroy(ObeliskTransactionManager* txn_manager);

// Transaction operations
ObeliskTransaction* txn_begin(ObeliskTransactionManager* txn_manager);
int txn_commit(ObeliskTransaction* txn);
int txn_abort(ObeliskTransaction* txn);
int txn_prepare(ObeliskTransaction* txn);  // For two-phase commit

// Transaction properties
void txn_set_isolation_level(ObeliskTransaction* txn, ObeliskIsolationLevel level);
ObeliskTransactionState txn_get_state(ObeliskTransaction* txn);
uint64_t txn_get_id(ObeliskTransaction* txn);

// Lock operations
int txn_acquire_lock(ObeliskTransaction* txn, uint64_t resource_id, ObeliskLockMode mode);
int txn_release_lock(ObeliskTransaction* txn, uint64_t resource_id);
bool txn_has_lock(ObeliskTransaction* txn, uint64_t resource_id, ObeliskLockMode mode);

// Write-ahead logging
int txn_write_log_record(ObeliskTransaction* txn, const ObeliskLogRecord* record);
int txn_flush_log(ObeliskTransactionManager* txn_manager);

// Recovery operations
int txn_recover(ObeliskTransactionManager* txn_manager);
int txn_checkpoint(ObeliskTransactionManager* txn_manager);
int txn_rollback(ObeliskTransaction* txn);

// Deadlock detection
typedef struct {
    uint64_t txn_id;
    uint64_t waiting_for_txn_id;
    uint64_t resource_id;
    uint64_t wait_start_time;
} ObeliskDeadlockInfo;

ObeliskDeadlockInfo* txn_detect_deadlocks(ObeliskTransactionManager* txn_manager, size_t* num_deadlocks);
int txn_resolve_deadlock(ObeliskTransactionManager* txn_manager, const ObeliskDeadlockInfo* deadlock);

#endif // OBELISK_TRANSACTION_H 