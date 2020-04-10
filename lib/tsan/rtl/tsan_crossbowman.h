#ifndef TSAN_CROSSBOWMAN_H
#define TSAN_CROSSBOWMAN_H

#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_common.h"

#define ASSERT(statement, msg, ...)             \
    do {                                        \
        if (!(statement)) {                     \
            Printf(msg, ##  __VA_ARGS__ );      \
            Die();                              \
        }                                       \
    } while(0)                            

namespace __tsan {

typedef enum ompt_mapping_op_t {
    ompt_mapping_alloc                   = 1,
    ompt_mapping_transfer_to_device      = 2,
    ompt_mapping_transfer_from_device    = 3,
    ompt_mapping_delete                  = 4,
    ompt_mapping_associate               = 5,
    ompt_mapping_disassociate            = 6
} ompt_mapping_op_t;

struct Interval {
    uptr left_end;   // included
    uptr right_end;  // excluded

    bool operator<(const Interval &other) const {
        if (this->right_end <= other.left_end) {
            return true; 
        } else {
            return false;
        }
    }

    bool operator>(const Interval &other) const {
        if (this->left_end >= other.right_end) {
            return true; 
        } else {
            return false;
        }
    }

    bool contains(const Interval &other) const {
        return this->left_end <= other.left_end && this->right_end >= other.right_end;
    }

    bool operator==(const Interval &other) const {
        return this->left_end == other.left_end && this->right_end == other.right_end;
    }
};

struct MapInfo {
    uptr start;
    uptr size;
};

struct Node {
    Interval interval;
    MapInfo info;
    Node *left_child;
    Node *right_child;
    Node *parent;

    Node(const Interval &interval, const MapInfo &info) :
        interval(interval),
        info(info),
        left_child(nullptr),
        right_child(nullptr), 
        parent(nullptr) {}

    bool insert(Node *n);

    Node* find(const Interval &i);

    Node* removeCurrentNode(const Interval &i, bool left_child_for_parent);

    Node* remove(const Interval &i, bool left_child_for_parent);
};

class IntervalTree {
 private:
    Node *root;
    u32 size; 

 public:
    class Iterator {
     private:
        u32 idx;
        u32 size;
        Node *next_node;
        Node **stack;
        friend class IntervalTree;
     public:
        Iterator(Node *root, u32 size);

        Iterator(const Iterator &i);

        Iterator() = default;

        ~Iterator();

        Iterator& operator++() {
            next_node = stack[idx++];
            return *this;
        }
        
        Node* operator*() const {
            return next_node;
        }

        bool operator==(const Iterator &i) const {
            return this->next_node == i.next_node; 
        }

        bool operator!=(const Iterator &i) const {
            return !(*this == i);
        }
    };

    IntervalTree() : root(nullptr), size(0) {}

    Node* getRoot() {
        return root;
    }

    Node* find(const Interval &i);

    bool insert(const Interval &interval, const MapInfo &info);

    void remove(const Interval &i);

    ~IntervalTree();

    Iterator begin() {
        return Iterator(root->parent, size + 1);
    }

    Iterator end() {
        Iterator i{};
        i.next_node = root->parent;
        return i;
    }

    Node* find(uptr begin, uptr size) {
        return find({begin, begin + size});
    }
};

} // namespace __tsan
#endif // TSAN_CROSSBOWMAN_H
