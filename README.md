# Obelisk Database

A custom relational database management system implemented in C, designed to demonstrate deep understanding of database internals and systems programming concepts.

## Technical Implementation

### 1. B-tree Index Implementation
- Custom B-tree data structure for efficient indexing (O(log n) operations)
- Optimized node structure with configurable order
- Memory-efficient pointer management for child nodes
- Binary search implementation for fast key lookups

### 2. Storage Engine Design
- Page-based storage architecture for efficient disk I/O
- Memory-mapped file implementation for improved performance
- Custom buffer pool management with LRU eviction policy
- Row-based storage format with variable-length record support

### 3. Transaction Management
- Write-Ahead Logging (WAL) implementation for durability
- ACID compliance through:
  - Atomicity: Transaction rollback capability
  - Consistency: Constraint enforcement
  - Isolation: Lock-based concurrency control
  - Durability: Persistent storage with WAL

### 4. Memory Management
- Custom buffer pool implementation
- Efficient page replacement algorithm
- Memory-mapped I/O for improved performance
- Careful memory leak prevention and resource cleanup

## Core Components

```
obelisk/
├── src/
│   ├── btree/         # B-tree implementation with binary search
│   ├── buffer/        # LRU buffer pool management
│   ├── storage/       # Page-based storage engine
│   ├── transaction/   # ACID transaction handling
│   ├── parser/        # SQL statement parser
│   └── utils/         # Common utilities
├── include/           # Public API headers
└── examples/         # Example usage
```

## Technical Challenges & Solutions

1. **Efficient Indexing**
   - Implemented B-tree with optimal node size for disk I/O
   - Binary search within nodes for O(log n) lookups
   - Careful handling of node splits and merges

2. **Memory Efficiency**
   - Page-based storage with configurable page sizes
   - Memory-mapped I/O for reduced copying
   - Smart buffer pool management

3. **Concurrency Control**
   - Lock-based isolation implementation
   - Dead-lock detection and prevention
   - Transaction rollback capability

## Building

Requirements:
- C compiler (GCC or Clang)
- CMake (>= 3.10)
- Make

```bash
mkdir build
cd build
cmake ..
make
```

## Usage Example

```c
#include <obelisk/db.h>

// Initialize database
ObeliskDB* db = obelisk_open("mydb.db");

// Create a table
obelisk_exec(db, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT)");

// Insert data
obelisk_exec(db, "INSERT INTO users VALUES (1, 'John')");

// Query data
ObeliskResult* result = obelisk_query(db, "SELECT * FROM users");

// Close database
obelisk_close(db);
```

## Learning Outcomes

This project demonstrates understanding of:
- Database internals and architecture
- Systems programming in C
- Data structure optimization
- File I/O and memory management
- Concurrent programming concepts
- ACID transaction properties

## Features

- B-tree based indexing for efficient data retrieval
- Write-Ahead Logging (WAL) for durability
- ACID transaction support
- SQL-like query interface
- Memory-mapped I/O for improved performance
- Page-based storage engine
- Buffer pool management
- Row-based storage format

## Project Structure

```
obelisk/
├── src/
│   ├── btree/         # B-tree implementation
│   ├── buffer/        # Buffer pool management
│   ├── storage/       # Storage engine
│   ├── transaction/   # Transaction management
│   ├── parser/        # SQL parser
│   └── utils/         # Utility functions
├── include/           # Header files
└── examples/         # Example usage
```

## Design Principles

1. **Efficiency**: Uses B-tree indexing and memory-mapped I/O for fast operations
2. **Reliability**: Implements WAL and ACID transactions
3. **Simplicity**: Clean, modular codebase with clear separation of concerns
4. **Portability**: Standard C99 compliance for wide platform support

## Contributing
