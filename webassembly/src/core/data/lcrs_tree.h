#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace core::data {

enum class IconState {
    VISIBLE = 0,
    HIDDEN = 1,
};

class TreeNode {
public:
    TreeNode(int32_t id, const char* label);
    ~TreeNode();

    int32_t GetId() const { return id_; }
    const char* GetLabel() const { return label_.c_str(); }
    IconState GetIconState() const { return iconState_; }
    void SetIconState(IconState state) { iconState_ = state; }

    const TreeNode* GetParent() const { return parent_; }
    TreeNode* GetParentMutable() { return parent_; }
    const TreeNode* GetLeftChild() const { return leftChild_; }
    TreeNode* GetLeftChildMutable() { return leftChild_; }
    const TreeNode* GetRightSibling() const { return rightSibling_; }
    TreeNode* GetRightSiblingMutable() { return rightSibling_; }
    const TreeNode* GetLeftSibling() const { return leftSibling_; }

    int32_t GetChildCount() const;
    int32_t GetDepth() const;

private:
    int32_t id_ = -1;
    std::string label_;
    TreeNode* parent_ = nullptr;
    TreeNode* leftChild_ = nullptr;
    TreeNode* rightSibling_ = nullptr;
    TreeNode* leftSibling_ = nullptr;
    IconState iconState_ = IconState::VISIBLE;

    friend class LcrsTree;
};

class LcrsTree {
public:
    using Ptr = std::unique_ptr<LcrsTree>;

    static Ptr New(int32_t id, const char* label);
    ~LcrsTree();

    const TreeNode* GetRoot() const { return root_; }
    TreeNode* GetRootMutable() { return root_; }

    const TreeNode* GetTreeNodeById(int32_t id) const;
    TreeNode* GetTreeNodeByIdMutable(int32_t id);

    TreeNode* InsertItem(int32_t id, const char* label, TreeNode* parent = nullptr);
    bool DeleteItem(int32_t id);

    void TraverseTree(std::function<void(const TreeNode*, void*)> callback,
                      const TreeNode* startNode = nullptr,
                      void* userData = nullptr) const;

    void TraverseTreeMutable(std::function<void(TreeNode*, void*)> callback,
                             TreeNode* startNode = nullptr,
                             void* userData = nullptr);

private:
    LcrsTree() = default;

    bool createRoot(int32_t id, const char* label);
    void deleteItemRecursive(TreeNode* node);
    void traverseTreeRecursive(std::function<void(const TreeNode*, void*)> callback,
                               const TreeNode* node,
                               void* userData) const;
    void traverseTreeRecursiveMutable(std::function<void(TreeNode*, void*)> callback,
                                      TreeNode* node,
                                      void* userData);
    bool isExistingId(int32_t id) const;

    TreeNode* root_ = nullptr;
};

} // namespace core::data
