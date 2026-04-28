#include "lcrs_tree.h"


// Implementation of TreeNode

TreeNode::TreeNode(int32_t id, const char* label)
    : m_Id(id)
    , m_Label(label)
    , m_Parent(nullptr)
    , m_LeftChild(nullptr)
    , m_RightSibling(nullptr)
    , m_LeftSibling(nullptr)
{
    assert(id >= 0 && "Id cannot be negative.");
    assert(label && "Label cannot be empty.");

    SPDLOG_DEBUG("Creating TreeNode - [Id]: {}, [Label]: {}", m_Id, m_Label);
}

TreeNode::~TreeNode() {
    // NOTE:
    // The node links are managed by the LcrsTree class,
    // so it is not need to rearrange the links here.
    SPDLOG_DEBUG("Destroying TreeNode - [Id]: {}, [Label]: {}", m_Id, m_Label);

    // Clear the node data
    m_Parent = nullptr;
    m_LeftChild = nullptr;
    m_RightSibling = nullptr;
    m_LeftSibling = nullptr;
    m_Label.clear();
    m_Id = -1;
}

int32_t TreeNode::GetChildCount() const {
    int32_t count = 0;
    TreeNode* child = m_LeftChild;

    // Traverse the right siblings to count the children
    while (child != nullptr) {
        ++count;
        child = child->m_RightSibling;
    }

    return count;
}

int32_t TreeNode::GetDepth() const {
    int32_t depth = 0;
    TreeNode* parent = m_Parent;

    // Traverse up the tree to count the depth
    while (parent != nullptr) {
        ++depth;
        parent = parent->m_Parent;
    }

    return depth;
}


// Implementation of LcrsTree

LcrsTreeUPtr LcrsTree::New(int32_t id, const char* label) {
    if (label == nullptr) {
        return nullptr;
    }

    // Create a new LcrsTree instance
    LcrsTreeUPtr tree = LcrsTreeUPtr(new LcrsTree());
    if (!tree->createRoot(id, label)) {
        return nullptr;
    }
    
    return std::move(tree);
}

LcrsTree::LcrsTree()
    : m_Root(nullptr)
{
}

LcrsTree::~LcrsTree() {
    if (m_Root) {
        // Delete all nodes starting from the root
        deleteItemRecursive(m_Root);
        m_Root = nullptr;
    }
}

bool LcrsTree::createRoot(int32_t id, const char* label) {
    assert(m_Root == nullptr && "Root already exists.");

    if (isExistingId(id)) {
        return false;
    }

    m_Root = new TreeNode(id, label);

    return true;
}

TreeNode* LcrsTree::InsertItem(int32_t id, const char* label, 
    TreeNode* parent) {
    if (isExistingId(id) || label == nullptr) {
        return nullptr;
    }

    // If the parent is nullptr, insert item under the root.
    if (parent == nullptr) {
        parent = m_Root;
    }

    TreeNode* newNode = new TreeNode(id, label);

    if (parent->m_LeftChild == nullptr) {
        // If the parent has no children, set the new node as the left child
        parent->m_LeftChild = newNode;
        newNode->m_Parent = parent;
    }
    else {
        // Otherwise, find the rightmost sibling and set the new node as the right sibling
        TreeNode* sibling = parent->m_LeftChild;
        while (sibling->m_RightSibling != nullptr) {
            sibling = sibling->m_RightSibling;
        }
        sibling->m_RightSibling = newNode;
        newNode->m_LeftSibling = sibling;
        newNode->m_Parent = parent;
    }

    return newNode;
}

bool LcrsTree::DeleteItem(int32_t id) {
    TreeNode* node = GetTreeNodeByIdMutable(id);
    if (node == nullptr) {
        return false;
    }

    if (node == m_Root) {
        // Cannot delete the root node
        return false;
    }

    // Update links
    TreeNode* parent = node->m_Parent;
    TreeNode* rightSibling = node->m_RightSibling;
    TreeNode* leftSibling = node->m_LeftSibling;

    if (parent->m_LeftChild == node) {
        // If the node is the left child of its parent
        parent->m_LeftChild = rightSibling;
    }

    if (leftSibling) {
        // If the node has a left sibling
        leftSibling->m_RightSibling = rightSibling;
    }

    if (rightSibling) {
        // If the node has a right sibling
        rightSibling->m_LeftSibling = leftSibling;
    }

    deleteItemRecursive(node);

    return true;
}

void LcrsTree::TraverseTree(std::function<void(const TreeNode*, void*)> callback, 
    const TreeNode* startNode, void* userData) const {
    if (callback == nullptr) {
        return;
    }

    // If startNode is nullptr, start from the root.
    if (startNode == nullptr) {
        startNode = m_Root;
    }

    // Traverse the tree recursively
    traverseTreeRecursive(callback, startNode, userData);
}

void LcrsTree::TraverseTreeMutable(std::function<void(TreeNode*, void*)> callback, 
    TreeNode* startNode, void* userData) {
    if (callback == nullptr) {
        return;
    }

    // If startNode is nullptr, start from the root.
    if (startNode == nullptr) {
        startNode = m_Root;
    }

    // Traverse the tree recursively
    traverseTreeRecursiveMutable(callback, startNode, userData);
}

void LcrsTree::traverseTreeRecursive(
    std::function<void(const TreeNode*, void*)> callback,
    const TreeNode* node, void* userData) const {
    if (!node || !callback) {
        return;
    }

    // Call the callback function for the current node
    callback(node, userData);

    // Traverse all children
    const TreeNode* child = node->m_LeftChild;
    while (child != nullptr) {
        traverseTreeRecursive(callback, child, userData);
        child = child->m_RightSibling;
    }
}

void LcrsTree::traverseTreeRecursiveMutable(
    std::function<void(TreeNode*, void*)> callback,
    TreeNode* node, void* userData) {
    if (!node || !callback) {
        return;
    }

    // Call the callback function for the current node
    callback(node, userData);

    // Traverse all children
    TreeNode* child = node->m_LeftChild;
    while (child != nullptr) {
        traverseTreeRecursiveMutable(callback, child, userData);
        child = child->m_RightSibling;
    }
}

void LcrsTree::deleteItemRecursive(TreeNode* node) {
    if (node == nullptr) {
        return;
    }

    // Recursively delete all children
    TreeNode* child = node->m_LeftChild;
    while (child != nullptr) {
        TreeNode* nextChild = child->m_RightSibling;
        deleteItemRecursive(child);
        child = nextChild;
    }

    // Delete the current node
    delete node;
}

const TreeNode* LcrsTree::GetTreeNodeById(int32_t id) const {
    if (m_Root == nullptr) {
        return nullptr;
    }

    const TreeNode* foundNode = nullptr;

    // Traverse the tree to find the node with the specified id
    traverseTreeRecursive([&foundNode, id](const TreeNode* node, void*) {
        if (node->GetId() == id) {
            foundNode = node;
            return;
        }
    }, m_Root, nullptr);

    return foundNode;
}

TreeNode* LcrsTree::GetTreeNodeByIdMutable(int32_t id) {
    if (m_Root == nullptr) {
        return nullptr;
    }

    TreeNode* foundNode = nullptr;

    // Traverse the tree to find the node with the specified id
    traverseTreeRecursiveMutable([&foundNode, id](TreeNode* node, void*) {
        if (node->GetId() == id) {
            foundNode = node;
            return;
        }
    }, m_Root, nullptr);

    return foundNode;
}

bool LcrsTree::isExistingId(int32_t id) const {
    if (GetTreeNodeById(id) != nullptr) {
        assert(false && "Id already exists.");
        return true;
    }
    return false;
}
