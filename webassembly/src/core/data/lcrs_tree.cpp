#include "lcrs_tree.h"

namespace core::data {

TreeNode::TreeNode(int32_t id, const char* label)
    : id_(id), label_(label != nullptr ? label : "") {
    assert(id >= 0);
    assert(label != nullptr);
}

TreeNode::~TreeNode() {
    parent_ = nullptr;
    leftChild_ = nullptr;
    rightSibling_ = nullptr;
    leftSibling_ = nullptr;
}

int32_t TreeNode::GetChildCount() const {
    int32_t count = 0;
    const TreeNode* child = leftChild_;
    while (child != nullptr) {
        ++count;
        child = child->rightSibling_;
    }
    return count;
}

int32_t TreeNode::GetDepth() const {
    int32_t depth = 0;
    const TreeNode* current = parent_;
    while (current != nullptr) {
        ++depth;
        current = current->parent_;
    }
    return depth;
}

LcrsTree::Ptr LcrsTree::New(int32_t id, const char* label) {
    if (label == nullptr) {
        return nullptr;
    }

    Ptr tree(new LcrsTree());
    if (!tree->createRoot(id, label)) {
        return nullptr;
    }
    return tree;
}

LcrsTree::~LcrsTree() {
    deleteItemRecursive(root_);
    root_ = nullptr;
}

bool LcrsTree::createRoot(int32_t id, const char* label) {
    if (root_ != nullptr || isExistingId(id)) {
        return false;
    }
    root_ = new TreeNode(id, label);
    return true;
}

TreeNode* LcrsTree::InsertItem(int32_t id, const char* label, TreeNode* parent) {
    if (label == nullptr || isExistingId(id)) {
        return nullptr;
    }

    if (parent == nullptr) {
        parent = root_;
    }
    if (parent == nullptr) {
        return nullptr;
    }

    TreeNode* newNode = new TreeNode(id, label);
    if (parent->leftChild_ == nullptr) {
        parent->leftChild_ = newNode;
        newNode->parent_ = parent;
        return newNode;
    }

    TreeNode* sibling = parent->leftChild_;
    while (sibling->rightSibling_ != nullptr) {
        sibling = sibling->rightSibling_;
    }
    sibling->rightSibling_ = newNode;
    newNode->leftSibling_ = sibling;
    newNode->parent_ = parent;
    return newNode;
}

bool LcrsTree::DeleteItem(int32_t id) {
    TreeNode* node = GetTreeNodeByIdMutable(id);
    if (node == nullptr || node == root_) {
        return false;
    }

    TreeNode* parent = node->parent_;
    TreeNode* rightSibling = node->rightSibling_;
    TreeNode* leftSibling = node->leftSibling_;

    if (parent != nullptr && parent->leftChild_ == node) {
        parent->leftChild_ = rightSibling;
    }
    if (leftSibling != nullptr) {
        leftSibling->rightSibling_ = rightSibling;
    }
    if (rightSibling != nullptr) {
        rightSibling->leftSibling_ = leftSibling;
    }

    deleteItemRecursive(node);
    return true;
}

void LcrsTree::TraverseTree(std::function<void(const TreeNode*, void*)> callback,
                            const TreeNode* startNode,
                            void* userData) const {
    if (!callback) {
        return;
    }
    const TreeNode* node = (startNode != nullptr) ? startNode : root_;
    traverseTreeRecursive(std::move(callback), node, userData);
}

void LcrsTree::TraverseTreeMutable(std::function<void(TreeNode*, void*)> callback,
                                   TreeNode* startNode,
                                   void* userData) {
    if (!callback) {
        return;
    }
    TreeNode* node = (startNode != nullptr) ? startNode : root_;
    traverseTreeRecursiveMutable(std::move(callback), node, userData);
}

void LcrsTree::traverseTreeRecursive(std::function<void(const TreeNode*, void*)> callback,
                                     const TreeNode* node,
                                     void* userData) const {
    if (node == nullptr || !callback) {
        return;
    }

    callback(node, userData);
    const TreeNode* child = node->leftChild_;
    while (child != nullptr) {
        traverseTreeRecursive(callback, child, userData);
        child = child->rightSibling_;
    }
}

void LcrsTree::traverseTreeRecursiveMutable(std::function<void(TreeNode*, void*)> callback,
                                            TreeNode* node,
                                            void* userData) {
    if (node == nullptr || !callback) {
        return;
    }

    callback(node, userData);
    TreeNode* child = node->leftChild_;
    while (child != nullptr) {
        traverseTreeRecursiveMutable(callback, child, userData);
        child = child->rightSibling_;
    }
}

void LcrsTree::deleteItemRecursive(TreeNode* node) {
    if (node == nullptr) {
        return;
    }

    TreeNode* child = node->leftChild_;
    while (child != nullptr) {
        TreeNode* next = child->rightSibling_;
        deleteItemRecursive(child);
        child = next;
    }

    delete node;
}

const TreeNode* LcrsTree::GetTreeNodeById(int32_t id) const {
    const TreeNode* found = nullptr;
    traverseTreeRecursive([&found, id](const TreeNode* node, void*) {
        if (node->GetId() == id) {
            found = node;
        }
    }, root_, nullptr);
    return found;
}

TreeNode* LcrsTree::GetTreeNodeByIdMutable(int32_t id) {
    TreeNode* found = nullptr;
    traverseTreeRecursiveMutable([&found, id](TreeNode* node, void*) {
        if (node->GetId() == id) {
            found = node;
        }
    }, root_, nullptr);
    return found;
}

bool LcrsTree::isExistingId(int32_t id) const {
    return GetTreeNodeById(id) != nullptr;
}

} // namespace core::data
