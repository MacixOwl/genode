// SPDX-License-Identifier: MulanPSL-2.0

// Forked from YurongOS: github.com/FlowerBlackG/YurongOS

/*

    空闲内存管理器

    创建于2023年1月14日 上饶市玉山县

*/

#include "./FreeMemoryManager.h"
#include "./MemoryManager.h"

#include <monkey/tycoon/Tycoon.h>


namespace yros {
namespace memory {

namespace FreeMemoryManager {
    PageLinkNode* currentPage;
    PageLinkNode* nextPage;

    DoubleLinkedTree addressIdxTree;
    DoubleLinkedTree sizeIdxTree;
    
    adl::uint64_t totalFreeMemory;
    monkey::Tycoon* tycoon = nullptr;
}

/* ------------ Free Memory Manager ------------ */

void FreeMemoryManager::init(void* firstPageOfLink) {
    totalFreeMemory = 0;
    
    currentPage = (PageLinkNode*) (firstPageOfLink);
    nextPage = currentPage + 1;
    
    currentPage->nodeUsed = 0;
    currentPage->nextNode = nextPage;
    currentPage->prevNode = nullptr;

    nextPage->nodeUsed = 0;
    nextPage->prevNode = currentPage;
    nextPage->nextNode = nullptr;

    sizeIdxTree.root = nullptr;
    addressIdxTree.root = nullptr;
}

namespace FreeMemoryManager {

void adjustPageLinkNodes() {
    if (currentPage->nodeUsed < FREE_PAGE_NODE_THREHOLD 
        && nextPage
        && currentPage->prevNode
    ) {
        free(
            ((adl::uint64_t) nextPage), 
            sizeof(PageLinkNode),
            true
        );

        currentPage = currentPage->prevNode;
        nextPage = currentPage->nextNode;
        nextPage->nextNode = nullptr;

    } else if (nextPage->nodeUsed > ALLOC_PAGE_NODE_THREHOLD) {

        adl::uint64_t allocRes = alloc(sizeof(PageLinkNode), true);
        
        nextPage = (PageLinkNode*) allocRes;
        nextPage->nextNode = nullptr;
        nextPage->nodeUsed = 0;
        nextPage->prevNode = currentPage->nextNode;
        currentPage = currentPage->nextNode;
        currentPage->nextNode = nextPage; 

    }
}

}


int FreeMemoryManager::free(adl::uint64_t address, adl::uint64_t size, bool dontAdjustPage, bool init) {

    if (size % 4096 != 0 || size == 0) {
        return 0;
    }

    // 假设不存在覆盖问题。

    const auto addrIdx =DoubleLinkedTree::NODE_DATA_ADDRESS_IDX;
    const auto sizeIdx = DoubleLinkedTree::NODE_DATA_SIZE_IDX;
    

    auto lessNode = addressIdxTree.findNode(
        address, addrIdx, 
        DoubleLinkedTree::FindNodeStrategy::MATCH_OR_LESS
    );

    auto moreNode = addressIdxTree.findNode(
        address, addrIdx, 
        DoubleLinkedTree::FindNodeStrategy::MATCH_OR_MORE
    );

    // 先修理覆盖问题，让目标区间与现有空闲内存不覆盖。
    if (lessNode) {
        uint64_t lessNodeUpBound = lessNode->addressIdxData().value + lessNode->sizeIdxData().value;
        if (lessNodeUpBound >= address + size) {
            
            return 0;
        } else if (lessNodeUpBound > address) {
            size -= lessNodeUpBound - address;
            address = lessNodeUpBound;
        }
    }

    if (moreNode) {
        uint64_t moreNodeAddr = moreNode->addressIdxData().value;

        if (address == moreNodeAddr) {
            
            return 0;
        } else if (address + size > moreNodeAddr) {
            size = moreNodeAddr - address;
        }
        
    }

    totalFreeMemory += size;

    DoubleLinkedTreeNode* target = nullptr;

    if (lessNode 
        && lessNode->addressIdxData().value + lessNode->sizeIdxData().value == address
    ) {
        
        // 可以和左侧节点合并。

        lessNode->sizeIdxData().value += size;
        sizeIdxTree.removeNode(lessNode, sizeIdx);
        sizeIdxTree.addNode(lessNode, sizeIdx);
        target = lessNode;

    } else if (
        moreNode && moreNode->addressIdxData().value == address + size
    ) {
        
        // 可以和右侧节点合并。

        moreNode->sizeIdxData().value += size;
        moreNode->addressIdxData().value = address;

        sizeIdxTree.removeNode(moreNode, sizeIdx);
        sizeIdxTree.addNode(moreNode, sizeIdx);
        addressIdxTree.removeNode(moreNode, addrIdx);
        addressIdxTree.addNode(moreNode, addrIdx);

        target = moreNode;

    } else {

        // 创建新节点。
        DoubleLinkedTreeNode* node = currentPage->allocNode();
        if (node == nullptr) {
            node = nextPage->allocNode();
        }

        node->addressIdxData().value = address;
        node->sizeIdxData().value = size;

        sizeIdxTree.addNode(node, sizeIdx);
        addressIdxTree.addNode(node, addrIdx);

        target = node;
    }

    // 尝试合并。
    
    
    bool merged = false;
    if (lessNode && lessNode != target) { // 尝试向左

        uint64_t lessNodeUpBound = lessNode->addressIdxData().value + lessNode->sizeIdxData().value;
        if (lessNodeUpBound == target->addressIdxData().value) { // 可以合并
            lessNode->sizeIdxData().value += target->sizeIdxData().value;
            sizeIdxTree.removeNode(lessNode, sizeIdx);
            sizeIdxTree.addNode(lessNode, sizeIdx);

            merged = true;
        }

    } else if (moreNode && target != moreNode) { // 尝试向右

        uint64_t targetUpBound = target->addressIdxData().value + target->sizeIdxData().value;
        if (moreNode->addressIdxData().value == targetUpBound) {
            moreNode->addressIdxData().value = target->addressIdxData().value;
            moreNode->sizeIdxData().value += target->sizeIdxData().value;

            sizeIdxTree.removeNode(moreNode, sizeIdx);
            sizeIdxTree.addNode(moreNode, sizeIdx);
            addressIdxTree.removeNode(moreNode, addrIdx);
            addressIdxTree.addNode(moreNode, addrIdx);

            merged = true;
        }

    }

    if (merged) { // 合并后的善后工作

        // 善后工作：
        //   删除节点 和 填补删出来的空位。

        sizeIdxTree.removeNode(target, sizeIdx);
        addressIdxTree.removeNode(target, addrIdx);

        DoubleLinkedTreeNode* back;
        if (nextPage->nodeUsed) {
            back = &nextPage->nodes[--nextPage->nodeUsed];
        } else {
            back = &currentPage->nodes[--currentPage->nodeUsed];
        }

        sizeIdxTree.moveNode(target, back);
        if (sizeIdxTree.root == back) {
            sizeIdxTree.root = target;
        }

        if (addressIdxTree.root == back) {
            addressIdxTree.root = target;
        }
    }

    if (!dontAdjustPage) {
        adjustPageLinkNodes();
    }
    

    // For Monkey Tycoon.
    if (tycoon != nullptr && !init) {
        for (adl::size_t off = 0; off < size; off += 4096) {
            tycoon->freePage(address + off);
        }
    }

    return 0;
}

adl::uint64_t FreeMemoryManager::alloc(adl::uint64_t size, bool dontAdjustPage) {

    if (size % 4096 != 0 || size == 0) {
        return 0; // Only 4KB aligned size is allowed.
    }

    auto addrIdx =DoubleLinkedTree::NODE_DATA_ADDRESS_IDX;
    auto sizeIdx = DoubleLinkedTree::NODE_DATA_SIZE_IDX;
    


    DoubleLinkedTreeNode* node = sizeIdxTree.findNode(
        size, 
        sizeIdx, 
        DoubleLinkedTree::FindNodeStrategy::MATCH_OR_MORE
    );

    if (node == nullptr) {
        
        return 0; // 找不到合适的节点。
    }

    uint64_t result;
    bool nodeRemoved = false;

    if (node->sizeIdxData().value == size) {
        // 容量刚好匹配。
        result = node->addressIdxData().value;
        sizeIdxTree.removeNode(node, sizeIdx);
        addressIdxTree.removeNode(node, addrIdx);

        nodeRemoved = true;

        DoubleLinkedTreeNode* back;
        if (nextPage->nodeUsed > 0) {
            back = &nextPage->nodes[nextPage->nodeUsed - 1];
        } else {
            back = &currentPage->nodes[currentPage->nodeUsed - 1];
        }

        sizeIdxTree.moveNode(node, back);

        if (back == sizeIdxTree.root) {
            sizeIdxTree.root = node;
        }
        if (back == addressIdxTree.root) {
            addressIdxTree.root = node;
        }

        if (nextPage->nodeUsed > 0) {
            nextPage->popNode();
        } else {
            currentPage->popNode();
        }

    } else { // 节点容量比所需的大。
        result = node->addressIdxData().value + node->sizeIdxData().value - size;
        node->sizeIdxData().value -= size;
        sizeIdxTree.removeNode(node, sizeIdx);
        sizeIdxTree.addNode(node, sizeIdx);
    }

    totalFreeMemory -= size;

    // 调整节点页。
    if (nodeRemoved && !dontAdjustPage) {
        adjustPageLinkNodes();
        
    }


    return result;

}

adl::uint64_t FreeMemoryManager::getTotalFreeMemory() {
    return totalFreeMemory;
}

adl::uint64_t FreeMemoryManager::getMaxAllocatableMemorySize() {

    if (sizeIdxTree.root == nullptr) {
        return 0;
    }

    auto curr = sizeIdxTree.root;
    while (curr->sizeIdxData().rightChild) {
        curr = curr->sizeIdxData().rightChild;
    }

    return curr->sizeIdxData().value;
}

/* ------------ Double Linked Tree Node ------------ */

int FreeMemoryManager::DoubleLinkedTreeNode::compareWith(
    const DoubleLinkedTreeNode& node, int index
) const {
    auto& myVal = this->nodeData[index].value;
    auto& itsVal = node.nodeData[index].value;

    if (myVal == itsVal) {
        return 0;
    } else if (myVal > itsVal) {
        return 1;
    } else {
        return -1;
    }
}


int FreeMemoryManager::DoubleLinkedTreeNode::compareWith(
    const DoubleLinkedTreeNode* pNode, int index
) const {
    return this->compareWith(*pNode, index);
}

/* ------------ Double Linked Tree ------------ */

int FreeMemoryManager::DoubleLinkedTree::removeNode(
    DoubleLinkedTreeNode* node, int index
) {

    if (node->nodeData[index].leftChild == nullptr
        && node->nodeData[index].rightChild == nullptr
    ) {
        auto parent = node->nodeData[index].parent;
        if (parent) {
            if (parent->nodeData[index].leftChild == node) {
                parent->nodeData[index].leftChild = nullptr;
            } else {
                parent->nodeData[index].rightChild = nullptr;
            }
        } else {
            this->root = nullptr;
        }

    } else if (node == root && node->nodeData[index].leftChild == nullptr) {

        auto rc = node->nodeData[index].rightChild;
        rc->nodeData[index].parent = nullptr;
        this->root = rc;

    } else if (node == root && node->nodeData[index].rightChild == nullptr) {

        auto lc = node->nodeData[index].leftChild;
        lc->nodeData[index].parent = nullptr;
        this->root = lc;

    } else if (node == root) {

        auto rc = node->nodeData[index].rightChild;
        auto lc = node->nodeData[index].leftChild;

        this->root = rc;
        rc->nodeData[index].parent = nullptr;

        auto curr = rc;
        while (curr->nodeData[index].leftChild) {
            curr = curr->nodeData[index].leftChild;
        }

        curr->nodeData[index].leftChild = lc;
        lc->nodeData[index].parent = curr;

    } else if (node->nodeData[index].leftChild == nullptr) {
        
        auto rc = node->nodeData[index].rightChild;
        auto parent = node->nodeData[index].parent;

        rc->nodeData[index].parent = parent;
        if (parent->nodeData[index].leftChild == node) {
            parent->nodeData[index].leftChild = rc;
        } else {
            parent->nodeData[index].rightChild = rc;
        }
        
    } else if (node->nodeData[index].rightChild == nullptr) {
        
        auto lc = node->nodeData[index].leftChild;
        auto parent = node->nodeData[index].parent;

        lc->nodeData[index].parent = parent;
        if (parent->nodeData[index].leftChild == node) {
            parent->nodeData[index].leftChild = lc;
        } else {
            parent->nodeData[index].rightChild = lc;
        }
        
    } else {
        // node 不是根节点，且具有2个孩子。

        auto parent = node->nodeData[index].parent;
        auto lc = node->nodeData[index].leftChild;
        auto rc = node->nodeData[index].rightChild;

        if (parent->nodeData[index].leftChild == node) {
            parent->nodeData[index].leftChild = rc;
        } else {
            parent->nodeData[index].rightChild = rc;
        }

        auto curr = rc;
        while (curr->nodeData[index].leftChild) {
            curr = curr->nodeData[index].leftChild;
        }

        curr->nodeData[index].leftChild = lc;
        lc->nodeData[index].parent = curr;

    }

    return 0;

}

int FreeMemoryManager::DoubleLinkedTree::addNode(
    DoubleLinkedTreeNode* node, int index
) {

    node->nodeData[index].parent = nullptr;
    node->nodeData[index].leftChild = nullptr;
    node->nodeData[index].rightChild = nullptr;

    if (this->root == nullptr) {
        this->root = node;
        return 0;
    }

    auto curr = this->root;
    while (true) {

        int cmpResult = curr->compareWith(node, index);
        if (cmpResult == 0) {
            // 刚好匹配。

            auto rc = curr->nodeData[index].rightChild; // right child

            if (rc != nullptr) {
                rc->nodeData[index].parent = node;
            }
            
            curr->nodeData[index].rightChild = node;
            node->nodeData[index].parent = curr;
            
            break;

        } else if (cmpResult > 0) {

            auto lc = curr->nodeData[index].leftChild;
            if (lc) {
                curr = lc;
            } else {
                node->nodeData[index].parent = curr;
                curr->nodeData[index].leftChild = node;
                break;
            }

        } else {
            // cmpResult > 0

            auto rc = curr->nodeData[index].rightChild;
            if (rc) {
                curr = rc;
            } else {
                node->nodeData[index].parent = curr;
                curr->nodeData[index].rightChild = node;
                break;
            }

        }

    }

    return 0;
}


int FreeMemoryManager::DoubleLinkedTree::moveNode(
    DoubleLinkedTreeNode* dest, DoubleLinkedTreeNode* src
) {

    if (dest == src) {
        return 0; // 自己移动到自己。不需要处理。
    }

    *dest = *src; // 先做内存拷贝。
    
    for (int i = 0; i < 2; i++) {
        
        auto parent = src->nodeData[i].parent;
        auto lc = src->nodeData[i].leftChild;
        auto rc = src->nodeData[i].rightChild;
        if (parent) {
            if (parent->nodeData[i].leftChild == src) {
                parent->nodeData[i].leftChild = dest;
            } else {
                parent->nodeData[i].rightChild = dest;
            }
        }

        if (lc) {
            lc->nodeData[i].parent = dest;
        }

        if (rc) {
            rc->nodeData[i].parent = dest;
        }

    }

    return 0;
}

FreeMemoryManager::DoubleLinkedTreeNode* 
FreeMemoryManager::DoubleLinkedTree::findNode(
    adl::uint64_t value, int index, FindNodeStrategy strategy
) {

    if (root == nullptr) {
        return nullptr;
    }

    auto curr = root;
    auto candidate = root;

    while (true) {
        if (curr == nullptr) {
            break;
        }

        auto currValue = curr->nodeData[index].value;

        if (currValue == value) {

            candidate = curr;
            break;

        } else if (currValue > value) {

            if (strategy == FindNodeStrategy::MATCH_OR_MORE) {
                candidate = curr;
            } 

            curr = curr->nodeData[index].leftChild;

        } else {
            // currValue < value

            if (strategy == FindNodeStrategy::MATCH_OR_LESS) {
                candidate = curr;
            }

            curr = curr->nodeData[index].rightChild;
        }

    }

    if (candidate->nodeData[index].value == value) {

        return candidate;

    } else if (
        candidate->nodeData[index].value > value && strategy == FindNodeStrategy::MATCH_OR_MORE
    ) {

        return candidate;

    } else if (
        candidate->nodeData[index].value < value && strategy == FindNodeStrategy::MATCH_OR_LESS
    ) {

        return candidate;

    } else {

        return nullptr;

    }

}


/* ------------ Page Link Node ------------ */

FreeMemoryManager::DoubleLinkedTreeNode* 
FreeMemoryManager::PageLinkNode::allocNode() {
    
    if (nodeUsed == sizeof(this->nodes) / sizeof(DoubleLinkedTreeNode)) {
        return nullptr; // 本页节点内没有空闲的树节点。
    }

    return nodes + (nodeUsed++);
}

int FreeMemoryManager::PageLinkNode::popNode(int count) {
    if (count > nodeUsed) {
        count = (int) nodeUsed;
    } else if (count <= 0) {
        return 0;
    }

    nodeUsed -= count;
    return count;
}

}
}
