#include <stdlib.h>
#include <string.h>
#include <obelisk/btree.h>

ObeliskBTree* btree_create(void* page_manager) {
    ObeliskBTree* tree = malloc(sizeof(ObeliskBTree));
    if (!tree) return NULL;

    tree->root = NULL;
    tree->num_nodes = 0;
    tree->height = 0;
    tree->page_manager = page_manager;

    return tree;
}

void btree_destroy(ObeliskBTree* tree) {
    if (!tree) return;
    // TODO: Implement node cleanup with proper page deallocation
    free(tree);
}

static ObeliskNode* create_node(ObeliskNodeType type) {
    ObeliskNode* node = malloc(sizeof(ObeliskNode));
    if (!node) return NULL;

    node->type = type;
    node->num_keys = 0;
    node->parent = NULL;
    node->is_dirty = true;
    node->page_id = 0;  // Assigned by page manager during persistence

    return node;
}

bool btree_search(ObeliskBTree* tree, uint64_t key, uint64_t* value) {
    if (!tree || !tree->root) return false;

    ObeliskNode* node = btree_find_leaf(tree, key);
    if (!node) return false;

    // Binary search within node for O(log n) lookup
    int left = 0, right = node->num_keys - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        if (node->keys[mid] == key) {
            if (value) *value = node->children[mid];  // Leaf nodes store values in children array
            return true;
        }
        if (node->keys[mid] < key) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return false;
}

ObeliskNode* btree_find_leaf(ObeliskBTree* tree, uint64_t key) {
    if (!tree || !tree->root) return NULL;

    ObeliskNode* node = tree->root;
    while (node->type == OBELISK_NODE_INTERNAL) {
        int i;
        for (i = 0; i < node->num_keys; i++) {
            if (key < node->keys[i]) break;
        }
        node = (ObeliskNode*)(uintptr_t)node->children[i];
    }

    return node;
}

int btree_insert(ObeliskBTree* tree, uint64_t key, uint64_t value) {
    if (!tree) return -1;

    if (!tree->root) {
        tree->root = create_node(OBELISK_NODE_LEAF);
        if (!tree->root) return -1;
        tree->root->keys[0] = key;
        tree->root->children[0] = value;
        tree->root->num_keys = 1;
        tree->height = 1;
        tree->num_nodes = 1;
        return 0;
    }

    ObeliskNode* leaf = btree_find_leaf(tree, key);
    if (!leaf) return -1;

    // Update value if key exists
    for (uint32_t i = 0; i < leaf->num_keys; i++) {
        if (leaf->keys[i] == key) {
            leaf->children[i] = value;
            leaf->is_dirty = true;
            return 0;
        }
    }

    // Insert into leaf if space available
    if (leaf->num_keys < OBELISK_BTREE_ORDER - 1) {
        uint32_t i = leaf->num_keys;
        while (i > 0 && leaf->keys[i-1] > key) {
            leaf->keys[i] = leaf->keys[i-1];
            leaf->children[i] = leaf->children[i-1];
            i--;
        }
        leaf->keys[i] = key;
        leaf->children[i] = value;
        leaf->num_keys++;
        leaf->is_dirty = true;
        return 0;
    }

    // TODO: Implement node splitting for B-tree balancing
    return -1;
}

int btree_split_child(ObeliskNode* parent, int index, ObeliskNode* child) {
    // TODO: Implement node splitting with proper page allocation
    return -1;
}

int btree_delete(ObeliskBTree* tree, uint64_t key) {
    // TODO: Implement deletion with proper rebalancing
    return -1;
}

int btree_merge_nodes(ObeliskNode* left, ObeliskNode* right) {
    // TODO: Implement node merging for deletion rebalancing
    return -1;
}

void btree_print(ObeliskBTree* tree) {
    // TODO: Implement tree printing for debugging
}

bool btree_validate(ObeliskBTree* tree) {
    // TODO: Implement validation of B-tree properties
    return true;
}

ObeliskBTreeIterator* btree_iterator_create(ObeliskBTree* tree) {
    if (!tree) return NULL;

    ObeliskBTreeIterator* iter = malloc(sizeof(ObeliskBTreeIterator));
    if (!iter) return NULL;

    iter->tree = tree;
    iter->current_node = NULL;
    iter->current_pos = 0;

    if (tree->root) {
        ObeliskNode* node = tree->root;
        while (node->type == OBELISK_NODE_INTERNAL) {
            node = (ObeliskNode*)(uintptr_t)node->children[0];
        }
        iter->current_node = node;
    }

    return iter;
}

bool btree_iterator_next(ObeliskBTreeIterator* iter, uint64_t* key, uint64_t* value) {
    if (!iter || !iter->current_node) return false;

    if (iter->current_pos >= iter->current_node->num_keys) {
        // TODO: Implement traversal to next leaf node
        return false;
    }

    if (key) *key = iter->current_node->keys[iter->current_pos];
    if (value) *value = iter->current_node->children[iter->current_pos];
    iter->current_pos++;

    return true;
}

void btree_iterator_destroy(ObeliskBTreeIterator* iter) {
    free(iter);
} 