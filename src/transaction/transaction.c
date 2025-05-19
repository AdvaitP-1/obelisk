#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <obelisk/transaction.h>

// Internal transaction structure
struct ObeliskTransaction {
    uint64_t txn_id;
    ObeliskTransactionState state;
    ObeliskIsolationLevel isolation_level;
    struct ObeliskTransactionManager* manager;
    uint64_t* locked_resources;
    size_t num_locked_resources;
    ObeliskLogRecord* log_records;
    size_t num_log_records;
    time_t start_time;
};

// Internal transaction manager structure
struct ObeliskTransactionManager {
    char* log_directory;
    size_t log_buffer_size;
    bool sync_commit;
    uint32_t checkpoint_interval;
    int log_fd;
    uint64_t next_txn_id;
    ObeliskTransaction** active_txns;
    size_t num_active_txns;
};

ObeliskTransactionManager* txn_manager_create(const ObeliskTransactionConfig* config) {
    if (!config || !config->log_directory) return NULL;

    ObeliskTransactionManager* manager = malloc(sizeof(ObeliskTransactionManager));
    if (!manager) return NULL;

    manager->log_directory = strdup(config->log_directory);
    manager->log_buffer_size = config->log_buffer_size;
    manager->sync_commit = config->sync_commit;
    manager->checkpoint_interval = config->checkpoint_interval;
    manager->next_txn_id = 1;
    manager->active_txns = NULL;
    manager->num_active_txns = 0;

    // Create log directory if it doesn't exist
    mkdir(manager->log_directory, 0755);

    // Open log file
    char log_path[1024];
    snprintf(log_path, sizeof(log_path), "%s/txn.log", manager->log_directory);
    manager->log_fd = open(log_path, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (manager->log_fd < 0) {
        free(manager->log_directory);
        free(manager);
        return NULL;
    }

    return manager;
}

void txn_manager_destroy(ObeliskTransactionManager* manager) {
    if (!manager) return;

    // Abort all active transactions
    for (size_t i = 0; i < manager->num_active_txns; i++) {
        if (manager->active_txns[i]) {
            txn_abort(manager->active_txns[i]);
            free(manager->active_txns[i]);
        }
    }

    close(manager->log_fd);
    free(manager->log_directory);
    free(manager->active_txns);
    free(manager);
}

static ObeliskTransaction* create_transaction(ObeliskTransactionManager* manager) {
    ObeliskTransaction* txn = malloc(sizeof(ObeliskTransaction));
    if (!txn) return NULL;

    txn->txn_id = manager->next_txn_id++;
    txn->state = OBELISK_TXN_ACTIVE;
    txn->isolation_level = OBELISK_ISOLATION_READ_COMMITTED;
    txn->manager = manager;
    txn->locked_resources = NULL;
    txn->num_locked_resources = 0;
    txn->log_records = NULL;
    txn->num_log_records = 0;
    txn->start_time = time(NULL);

    return txn;
}

ObeliskTransaction* txn_begin(ObeliskTransactionManager* manager) {
    if (!manager) return NULL;

    ObeliskTransaction* txn = create_transaction(manager);
    if (!txn) return NULL;

    // Add to active transactions
    ObeliskTransaction** new_txns = realloc(manager->active_txns, 
                                          (manager->num_active_txns + 1) * sizeof(ObeliskTransaction*));
    if (!new_txns) {
        free(txn);
        return NULL;
    }

    manager->active_txns = new_txns;
    manager->active_txns[manager->num_active_txns++] = txn;

    // Write BEGIN log record
    ObeliskLogRecord log_record = {
        .type = OBELISK_LOG_BEGIN,
        .txn_id = txn->txn_id,
        .timestamp = time(NULL)
    };

    txn_write_log_record(txn, &log_record);

    return txn;
}

int txn_commit(ObeliskTransaction* txn) {
    if (!txn || txn->state != OBELISK_TXN_ACTIVE) return -1;

    // Write COMMIT log record
    ObeliskLogRecord log_record = {
        .type = OBELISK_LOG_COMMIT,
        .txn_id = txn->txn_id,
        .timestamp = time(NULL)
    };

    int result = txn_write_log_record(txn, &log_record);
    if (result != 0) return result;

    // Force log to disk if sync_commit is enabled
    if (txn->manager->sync_commit) {
        fsync(txn->manager->log_fd);
    }

    // Release all locks
    for (size_t i = 0; i < txn->num_locked_resources; i++) {
        txn_release_lock(txn, txn->locked_resources[i]);
    }

    txn->state = OBELISK_TXN_COMMITTED;
    return 0;
}

int txn_abort(ObeliskTransaction* txn) {
    if (!txn || txn->state != OBELISK_TXN_ACTIVE) return -1;

    // Write ABORT log record
    ObeliskLogRecord log_record = {
        .type = OBELISK_LOG_ABORT,
        .txn_id = txn->txn_id,
        .timestamp = time(NULL)
    };

    txn_write_log_record(txn, &log_record);

    // Undo all changes in reverse order
    for (size_t i = txn->num_log_records; i > 0; i--) {
        ObeliskLogRecord* record = &txn->log_records[i-1];
        if (record->type == OBELISK_LOG_UPDATE) {
            // Restore before image
            // TODO: Implement actual undo
        }
    }

    // Release all locks
    for (size_t i = 0; i < txn->num_locked_resources; i++) {
        txn_release_lock(txn, txn->locked_resources[i]);
    }

    txn->state = OBELISK_TXN_ABORTED;
    return 0;
}

void txn_set_isolation_level(ObeliskTransaction* txn, ObeliskIsolationLevel level) {
    if (!txn) return;
    txn->isolation_level = level;
}

ObeliskTransactionState txn_get_state(ObeliskTransaction* txn) {
    return txn ? txn->state : OBELISK_TXN_ABORTED;
}

uint64_t txn_get_id(ObeliskTransaction* txn) {
    return txn ? txn->txn_id : 0;
}

int txn_acquire_lock(ObeliskTransaction* txn, uint64_t resource_id, ObeliskLockMode mode) {
    if (!txn || txn->state != OBELISK_TXN_ACTIVE) return -1;

    // Check if already locked
    for (size_t i = 0; i < txn->num_locked_resources; i++) {
        if (txn->locked_resources[i] == resource_id) {
            return 0;  // Already locked
        }
    }

    // Add to locked resources
    uint64_t* new_resources = realloc(txn->locked_resources, 
                                    (txn->num_locked_resources + 1) * sizeof(uint64_t));
    if (!new_resources) return -1;

    txn->locked_resources = new_resources;
    txn->locked_resources[txn->num_locked_resources++] = resource_id;

    return 0;
}

int txn_release_lock(ObeliskTransaction* txn, uint64_t resource_id) {
    if (!txn) return -1;

    for (size_t i = 0; i < txn->num_locked_resources; i++) {
        if (txn->locked_resources[i] == resource_id) {
            // Remove from array by shifting remaining elements
            memmove(&txn->locked_resources[i], 
                   &txn->locked_resources[i + 1],
                   (txn->num_locked_resources - i - 1) * sizeof(uint64_t));
            txn->num_locked_resources--;
            return 0;
        }
    }

    return -1;  // Lock not found
}

bool txn_has_lock(ObeliskTransaction* txn, uint64_t resource_id, ObeliskLockMode mode) {
    if (!txn) return false;

    for (size_t i = 0; i < txn->num_locked_resources; i++) {
        if (txn->locked_resources[i] == resource_id) {
            return true;
        }
    }

    return false;
}

int txn_write_log_record(ObeliskTransaction* txn, const ObeliskLogRecord* record) {
    if (!txn || !record) return -1;

    // Write to log file
    ssize_t bytes_written = write(txn->manager->log_fd, record, sizeof(ObeliskLogRecord));
    if (bytes_written != sizeof(ObeliskLogRecord)) {
        return -1;
    }

    // Add to transaction's log records
    ObeliskLogRecord* new_records = realloc(txn->log_records,
                                          (txn->num_log_records + 1) * sizeof(ObeliskLogRecord));
    if (!new_records) return -1;

    txn->log_records = new_records;
    memcpy(&txn->log_records[txn->num_log_records++], record, sizeof(ObeliskLogRecord));

    return 0;
}

int txn_flush_log(ObeliskTransactionManager* manager) {
    if (!manager) return -1;
    return fsync(manager->log_fd);
}

// Placeholder implementations for remaining functions
int txn_prepare(ObeliskTransaction* txn) {
    // TODO: Implement two-phase commit preparation
    return -1;
}

int txn_recover(ObeliskTransactionManager* manager) {
    // TODO: Implement recovery from log
    return -1;
}

int txn_checkpoint(ObeliskTransactionManager* manager) {
    // TODO: Implement checkpointing
    return -1;
}

int txn_rollback(ObeliskTransaction* txn) {
    // TODO: Implement rollback to savepoint
    return -1;
}

ObeliskDeadlockInfo* txn_detect_deadlocks(ObeliskTransactionManager* manager, size_t* num_deadlocks) {
    // TODO: Implement deadlock detection
    if (num_deadlocks) *num_deadlocks = 0;
    return NULL;
}

int txn_resolve_deadlock(ObeliskTransactionManager* manager, const ObeliskDeadlockInfo* deadlock) {
    // TODO: Implement deadlock resolution
    return -1;
} 