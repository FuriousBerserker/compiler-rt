#include "tsan_mman.h"
#include "tsan_crossbowman.h"
#include "sanitizer_common/sanitizer_vector.h"
#include "sanitizer_common/sanitizer_placement_new.h"

namespace __tsan {

bool Node::insert(Node *n) {
    if (interval > n->interval) {
        if (left_child) {
            return left_child->insert(n);
        } else {
            left_child = n;
            n->parent = this;
            return true;
        }
    } else if (interval < n->interval) {
        if (right_child) {
            return right_child->insert(n);
        } else {
            right_child = n;
            n->parent = this;
            return true;
        }
    } else {
        return false;
    }
}

Node* Node::find(const Interval &i) {
    if (this->interval.contains(i)) {
        return this;
    } else {
        Node *next = (this->interval > i) ? left_child : right_child;
        if (next) {
            return next->find(i);
        } else {
            return nullptr;
        }
    }
}

Node* Node::removeCurrentNode(const Interval &i, bool left_child_for_parent) {
    ASSERT(this->interval == i, "Two interval are not same");
    if (left_child && right_child) {
        // find rightmost subtree in the left_child, use that to replace the current node
        //Printf("remove node with two child\n");
        Node *rightmost_in_left = left_child;
        while (rightmost_in_left->right_child) {
            rightmost_in_left = rightmost_in_left->right_child;
        }
        if (rightmost_in_left == left_child) {
            left_child->right_child = right_child;
            left_child->parent = parent;
            right_child->parent = left_child;
            if (left_child_for_parent) {
                parent->left_child = left_child;
            } else {
                parent->right_child = left_child;
            }
            return this;
        } else {
            Node *substitute = rightmost_in_left->remove(rightmost_in_left->interval, false);
            substitute->left_child = left_child;
            substitute->right_child = right_child;
            substitute->parent = parent;
            left_child->parent = substitute;
            right_child->parent = substitute;
            if (left_child_for_parent) {
                parent->left_child = substitute;
            } else {
                parent->right_child = substitute;
            }
            return this;
        }
    } else {
        if (left_child) {
            //Printf("remove node with left child\n");
            left_child->parent = parent;
            if (left_child_for_parent) {
                parent->left_child = left_child;
            } else {
                parent->right_child = left_child;
            }
            return this;
        } else if (right_child) {
            //Printf("remove node with right child\n");
            right_child->parent = parent;
            if (left_child_for_parent) {
                parent->left_child = right_child;
            } else {
                parent->right_child = right_child;
            }
            return this;
        } else {    
            //Printf("remove node with no child\n");
            if (left_child_for_parent) {
                parent->left_child = nullptr;
            } else {
                parent->right_child = nullptr;
            }
            return this;
        }
    }
}

Node* Node::remove(const Interval &i, bool left_child_for_parent) {
    // FIXME: we need to tackle aggregate data operations
    if (this->interval.contains(i)) {
        return removeCurrentNode(i, left_child_for_parent);
    } else {
        if (this->interval > i) {
            if (left_child) {
                return left_child->remove(i, true);
            } else {
                return nullptr;
            }
        } else {
            if (right_child) {
                return right_child->remove(i, false);
            } else {
                return nullptr;
            }
        }
    }
}

Node* IntervalTree::find(const Interval &i) {
    if (root) {
        return root->find(i);
    } else {
        return nullptr;
    }        
} 

void IntervalTree::insert(const Interval &interval, const MapInfo &info) {
    void *ptr = internal_alloc(MBlockNode, sizeof(Node));
    Node *n = new(ptr) Node(interval, info);
    if (root) {
        if (root->insert(n)) {
            size++;
        } else {
            internal_free(n);
        }
    } else {
        root = n; 
        // fake parent for root
        void *ptr_fakeroot = internal_alloc(MBlockNode, sizeof(Node));
        Node *fakeroot = new(ptr_fakeroot) Node({0, 0}, {0, 0});
        root->parent = fakeroot;
        fakeroot->left_child = root;
        size++;
    }
}

void IntervalTree::remove(const Interval &i) {
    if (root) {
        // root is the left child for its fake parent
        Node *n = root->remove(i, true);
        if (n == root) {
            root = root->parent->left_child; 
        }

        if (n) {
            internal_free(n);
            size--;
        }
    }
}

IntervalTree::~IntervalTree() {
    if (root) {
        internal_free(root->parent);
        Vector<Node *> stack;
        stack.PushBack(root);
        while (stack.Size()) {
            Node *next = stack.back();
            stack.PopBack();
            if (next->left_child) {
                stack.PushBack(next->left_child);
            }
            if (next->right_child) {
                stack.PushBack(next->right_child);
            }
            internal_free(next);
        }
    }
}


int sortHelper(Node *n, Node **result, int next_idx) {
    if (n->left_child) {
        next_idx = sortHelper(n->left_child, result, next_idx);
    }
    result[next_idx++] = n;
    if (n->right_child) {
        next_idx = sortHelper(n->right_child, result, next_idx);
    }
    return next_idx;
}


IntervalTree::Iterator::Iterator(Node *root, u32 size) : 
    idx(0), 
    size(size),
    next_node(nullptr), 
    stack(nullptr) {
    stack = reinterpret_cast<Node **>(internal_alloc(MBlockPointer, sizeof(Node *) * size));
    sortHelper(root, stack, 0);
    next_node = stack[idx++];
}

IntervalTree::Iterator::Iterator(const Iterator &i) :
    idx(i.idx),
    size(i.size), 
    next_node(nullptr), 
    stack(nullptr) {
    if (i.stack) {
        stack = reinterpret_cast<Node **>(internal_alloc(MBlockPointer, sizeof(Node *) * size));
        internal_memcpy(stack, i.stack, sizeof(Node *) * size);
        next_node = stack[idx - 1];
    }
}

IntervalTree::Iterator::~Iterator() {
    if (stack) {
        internal_free(stack);
    }
}

} // namespace __tsan
