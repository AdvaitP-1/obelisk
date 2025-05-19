#ifndef OBELISK_BTREE_H
#define OBELISK_BTREE_H

#include <stdint.h>
#include <stdbool.h>

// B-tree node types
typedef enum {
    OBELISK_NODE_LEAF = 0,
    OBELISK_NODE_INTERNAL = 1
} ObeliskNodeType;

// B-tree order (maximum number of children per node)
#define OBELISK_BTREE_ORDER 128

// B-tree node structure
typedef struct ObeliskNode {
    ObeliskNodeType type;
    uint32_t num_keys;
    uint64_t keys[OBELISK_BTREE_ORDER - 1];
    uint64_t children[OBELISK_BTREE_ORDER];
    struct ObeliskNode* parent;
    bool is_dirty;
    uint64_t page_id;
} ObeliskNode;

// B-tree structure
typedef struct {
    ObeliskNode* root;
    uint64_t num_nodes;
    uint64_t height;
    void* page_manager;  // Opaque pointer to page manager
} ObeliskBTree;

// B-tree operations
ObeliskBTree* btree_create(void* page_manager);
void btree_destroy(ObeliskBTree* tree);

// Search operations
bool btree_search(ObeliskBTree* tree, uint64_t key, uint64_t* value);
ObeliskNode* btree_find_leaf(ObeliskBTree* tree, uint64_t key);

// Insertion operations
int btree_insert(ObeliskBTree* tree, uint64_t key, uint64_t value);
int btree_split_child(ObeliskNode* parent, int index, ObeliskNode* child);

// Deletion operations
int btree_delete(ObeliskBTree* tree, uint64_t key);
int btree_merge_nodes(ObeliskNode* left, ObeliskNode* right);

// Utility operations
void btree_print(ObeliskBTree* tree);
bool btree_validate(ObeliskBTree* tree);

// Iterator interface
typedef struct {
    ObeliskBTree* tree;
    ObeliskNode* current_node;
    int current_pos;
} ObeliskBTreeIterator;

ObeliskBTreeIterator* btree_iterator_create(ObeliskBTree* tree);
bool btree_iterator_next(ObeliskBTreeIterator* iter, uint64_t* key, uint64_t* value);
void btree_iterator_destroy(ObeliskBTreeIterator* iter);

#endif // OBELISK_BTREE_H 