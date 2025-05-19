#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <obelisk/db.h>

// Error handling helper
void check_error(int result, const char* operation) {
    if (result != OBELISK_OK) {
        fprintf(stderr, "Error during %s: %d\n", operation, result);
        exit(1);
    }
}

int main() {
    // Create database configuration
    ObeliskConfig config = {
        .db_path = "test.db",
        .cache_size = 1000,  // 1000 pages
        .sync_writes = true,
        .wal_size = 4096 * 1024  // 4MB WAL
    };

    // Open database
    ObeliskDB* db = obelisk_open_with_config(&config);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        return 1;
    }

    // Create table schema
    ObeliskColumn columns[] = {
        {
            .name = "id",
            .type = OBELISK_TYPE_INT,
            .is_primary_key = true,
            .is_nullable = false,
            .is_unique = true
        },
        {
            .name = "name",
            .type = OBELISK_TYPE_TEXT,
            .is_primary_key = false,
            .is_nullable = false,
            .is_unique = false
        },
        {
            .name = "age",
            .type = OBELISK_TYPE_INT,
            .is_primary_key = false,
            .is_nullable = true,
            .is_unique = false
        }
    };

    ObeliskSchema schema = {
        .table_name = "users",
        .columns = columns,
        .num_columns = 3
    };

    // Create table
    check_error(obelisk_create_table(db, &schema), "create table");

    // Begin transaction
    ObeliskTransaction* txn = obelisk_transaction_begin(db);
    if (!txn) {
        fprintf(stderr, "Failed to begin transaction\n");
        obelisk_close(db);
        return 1;
    }

    // Insert some data
    check_error(obelisk_exec(db, "INSERT INTO users (id, name, age) VALUES (1, 'Alice', 25)"), "insert 1");
    check_error(obelisk_exec(db, "INSERT INTO users (id, name, age) VALUES (2, 'Bob', 30)"), "insert 2");
    check_error(obelisk_exec(db, "INSERT INTO users (id, name, age) VALUES (3, 'Charlie', 35)"), "insert 3");

    // Query data
    ObeliskResult* result = obelisk_query(db, "SELECT * FROM users WHERE age > 25");
    if (!result) {
        fprintf(stderr, "Query failed\n");
        obelisk_transaction_rollback(txn);
        obelisk_close(db);
        return 1;
    }

    // Print results
    printf("Users over 25:\n");
    while (obelisk_result_next(result)) {
        int id = obelisk_result_get_int(result, 0);
        const char* name = obelisk_result_get_text(result, 1);
        int age = obelisk_result_get_int(result, 2);
        printf("ID: %d, Name: %s, Age: %d\n", id, name, age);
    }

    // Clean up
    obelisk_result_free(result);
    check_error(obelisk_transaction_commit(txn), "commit");
    obelisk_close(db);

    printf("Example completed successfully!\n");
    return 0;
} 