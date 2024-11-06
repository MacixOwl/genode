/**
 * Red Black Tree Hpp
 * by GTY
 * 2022.1
 * at Yushan County, Shangrao, Jiangxi
 */

// https://blog.csdn.net/m0_62405272/article/details/122612246
// https://blog.csdn.net/m0_62405272/article/details/122631653

// 2024.11.6: modified for Amkos

#include "RedBlackTree.h"

#include <base/allocator.h>
#include <base/exception.h>
#include <base/mutex.h>

namespace MtsysKv {

template<typename KeyType, typename DataType>
RedBlackTree<KeyType, DataType>::RedBlackTree(Genode::Allocator* allocator)
{
    this->allocator = allocator;
}


template<typename KeyType, typename DataType>
RedBlackTree<KeyType, DataType>::~RedBlackTree()
{
    this->clear();
}


template<typename KeyType, typename DataType>
void RedBlackTree<KeyType, DataType>::clear()
{
    Genode::Mutex::Guard _g{mutex};
    if (this->root != nullptr) {
        this->cleanup(this->root);
        this->root = nullptr;
    }
}


template<typename KeyType, typename DataType>
bool RedBlackTree<KeyType, DataType>::hasKey(const KeyType& queryKey)
{
    Genode::Mutex::Guard _g{mutex};


    Node* currentNode = this->root;
    
    while (currentNode != nullptr) {
        if (queryKey == currentNode->key) {
            return true; // key found.
        }
        else if (currentNode->key > queryKey) {
            currentNode = currentNode->leftChild; // target is less than curr. search from left.
        }
        else { // queryKey > currentNode->key
            currentNode = currentNode->rightChild; // target is more than curr. search from right.
        }
    }

    return false; // could not find key.
}


template<typename KeyType, typename DataType>
DataType& RedBlackTree<KeyType, DataType>::getData(const KeyType& key)
{
    Genode::Mutex::Guard _g{mutex};

    Node* currentNode = root;
    while (currentNode != nullptr) {
        if (key == currentNode->key) {
            return currentNode->data; // key found
        }
        else if (currentNode->key > key) {
            currentNode = currentNode->leftChild; // target is less than curr. search from left.
        }
        else { // key > currentNode->key
            currentNode = currentNode->rightChild; // target is more than curr. search from right.
        }
    }

#if 0
    throw std::runtime_error("could not find your key in the object."); // key not found.
#else
    throw RuntimeError {};
#endif

}


template<typename KeyType, typename DataType>
RedBlackTree<KeyType, DataType>& RedBlackTree<KeyType, DataType>::setData(
    const KeyType& key, 
    const DataType& data
)
{

    Genode::Mutex::Guard _g{mutex};


    Node* currentNode = root;
    Node* currentFather = nullptr;

    while (currentNode != nullptr) {
        if (key == currentNode->key) { // key found
            currentNode->data = data;
            return *this; // data updated. exit.
        }
        else {
            currentFather = currentNode;
            currentNode = (currentNode->key > key ? currentNode->leftChild : currentNode->rightChild);
        }
    }


    // now, we should create new node for data.

    /*
        now, currentNode points to nullptr, currentFather points to the last visited node, probably nullptr.
        new node is red, inserted to the end.
    */

    // create node
    currentNode = (Node*) allocator->alloc(sizeof(Node));
    currentNode->father = currentFather;
    currentNode->leftChild = nullptr;
    currentNode->rightChild = nullptr;
    currentNode->key = key;
    currentNode->data = data;

    // If tree is empty, just set new node as root.
    if (currentFather == nullptr) {
        currentNode->color = NodeColor::BLACK;
        this->root = currentNode;
        return *this;
    }

    
    // now, we should handle when tree is not empty.
    
    currentNode->color = NodeColor::RED;
    // bind new node to parent.
    if (currentFather->key > key) {
        currentFather->leftChild = currentNode;
    }
    else {
        currentFather->rightChild = currentNode;
    }

    this->rebalanceRedNode(currentNode);
    return *this;
}


template<typename KeyType, typename DataType>
RedBlackTree<KeyType, DataType>& RedBlackTree<KeyType, DataType>::removeKey(
    const KeyType& key
)
{
    Genode::Mutex::Guard _g{mutex};

    
    Node* currentNode = root;

    while (currentNode != nullptr) {
        if (key == currentNode->key) {
            break; // key found. matched.
        }
        else {
            currentNode = (currentNode->key > key ? currentNode->leftChild : currentNode->rightChild);
        }
    }

    if (currentNode == nullptr) {

#if 0
        throw std::runtime_error("could not find your key in the object."); // key not found.
#else
        throw RuntimeError {};
#endif
    
    }


    // Otherwise, find an alternative node.
    // As long as there is at least one child, continue to search for an alternative node.
    while (currentNode->leftChild != nullptr || currentNode->rightChild != nullptr) {
        if (currentNode->rightChild != nullptr) {
            // current node has right child.
            // replace the original deleted node with its successor.
            // note that successor must has no left child.
            Node* replacementNode = currentNode->rightChild;
            while (replacementNode->leftChild != nullptr) {
                replacementNode = replacementNode->leftChild;
            }

            struct {
                Node* leftChild;
                Node* rightChild;
                NodeColor color;
                Node* father;
            } currentNodeInfo = {
                    currentNode->leftChild, currentNode->rightChild,
                    currentNode->color, currentNode->father
            }, replacementNodeInfo = {
                    replacementNode->leftChild, replacementNode->rightChild,
                    replacementNode->color, replacementNode->father
            };

            // exchange color.
            currentNode->color = replacementNodeInfo.color;
            replacementNode->color = currentNodeInfo.color;

            // edit parent node.
            if (currentNodeInfo.father != nullptr) {
                if (currentNodeInfo.father->leftChild == currentNode) {
                    currentNodeInfo.father->leftChild = replacementNode;
                }
                else {
                    currentNodeInfo.father->rightChild = replacementNode;
                }
            }
            else {
                this->root = replacementNode;
            }

            // edit currentNode's left child's data.
            if (currentNodeInfo.leftChild != nullptr) {
                currentNodeInfo.leftChild->father = replacementNode;
            }

            // edit replacementNode's right child's data (if has)
            if (replacementNodeInfo.rightChild != nullptr) {
                replacementNodeInfo.rightChild->father = currentNode;
            }
            
            currentNode->leftChild = nullptr; 
            currentNode->rightChild = replacementNodeInfo.rightChild;
            if (replacementNodeInfo.father == currentNode) {
                currentNode->father = replacementNode;
            }
            else {
                currentNode->father = replacementNodeInfo.father;
            }

            replacementNode->leftChild = currentNodeInfo.leftChild;
            replacementNode->father = currentNodeInfo.father;
            if (currentNodeInfo.rightChild == replacementNode) {
                replacementNode->rightChild = currentNode;
            }
            else {
                replacementNode->rightChild = currentNodeInfo.rightChild;
            }

            if (currentNodeInfo.rightChild != replacementNode) {
                currentNodeInfo.rightChild->father = replacementNode;
                replacementNodeInfo.father->leftChild = currentNode;
            }
        } // if (currnode has right child)
        else { // curr child only have left child
        
            // note that prev node's right child must be nullptr.
            Node* replacementNode = currentNode->leftChild;
            while (replacementNode->rightChild != nullptr) {
                replacementNode = replacementNode->rightChild;
            }

            struct {
                Node* leftChild;
                Node* rightChild;
                NodeColor color;
                Node* father;
            } currentNodeInfo = {
                    currentNode->leftChild, currentNode->rightChild,
                    currentNode->color, currentNode->father
            }, replacementNodeInfo = {
                    replacementNode->leftChild, replacementNode->rightChild,
                    replacementNode->color, replacementNode->father
            };

            // exchange color
            currentNode->color = replacementNodeInfo.color;
            replacementNode->color = currentNodeInfo.color;

            // edit parent node
            if (currentNodeInfo.father != nullptr) {
                if (currentNodeInfo.father->leftChild == currentNode) {
                    currentNodeInfo.father->leftChild = replacementNode;
                }
                else {
                    currentNodeInfo.father->rightChild = replacementNode;
                }
            }
            else {
                this->root = replacementNode;
            }


            if (currentNodeInfo.rightChild != nullptr) {
                currentNodeInfo.rightChild->father = replacementNode;
            }

            if (replacementNodeInfo.leftChild != nullptr) {
                replacementNodeInfo.leftChild->father = currentNode;
            }

            currentNode->rightChild = nullptr;
            currentNode->leftChild = replacementNodeInfo.leftChild;
            if (replacementNodeInfo.father == currentNode) {
                currentNode->father = replacementNode;
            }
            else {
                currentNode->father = replacementNodeInfo.father;
            }

            replacementNode->rightChild = currentNodeInfo.rightChild;
            replacementNode->father = currentNodeInfo.father;
            if (currentNodeInfo.leftChild == replacementNode) {
                replacementNode->leftChild = currentNode;
            }
            else {
                replacementNode->leftChild = currentNodeInfo.leftChild;
            }

            if (currentNodeInfo.leftChild != replacementNode) {
                currentNodeInfo.leftChild->father = replacementNode;
                replacementNodeInfo.father->rightChild = currentNode;
            }
        } 
    } // while (currentNode->leftChild != nullptr || currentNode->rightChild != nullptr)


    // now, node to be deleted has no child.

    if (currentNode == this->root) { // if deleting root
        this->root = nullptr;
        allocator->free(currentNode, sizeof(Node));
    } 
    else if (currentNode->color == NodeColor::RED) {
        if (currentNode == currentNode->father->leftChild) {
            currentNode->father->leftChild = nullptr;
        }
        else {
            currentNode->father->rightChild = nullptr;
        }
        allocator->free(currentNode, sizeof(Node));
    } 
    else { 
        // target node's parent must exists.
        // because target is black, it must have sibling.
        Node* sibling = (currentNode->father->leftChild != currentNode ?
            currentNode->father->leftChild : currentNode->father->rightChild);

        Node* currentFather = currentNode->father;

        ChildSide siblingSideToFather = 
            (sibling == currentFather->leftChild ? ChildSide::LEFT : ChildSide::RIGHT);

        // now we can delete the node.
        if (currentFather->leftChild == currentNode) {
            currentFather->leftChild = nullptr;
        }
        else {
            currentFather->rightChild = nullptr;
        }
        
        allocator->free(currentNode, sizeof(Node));


        // Next, let's consider different scenarios.

        if (currentFather->color == NodeColor::RED) {
            // when parent is red, sibling must be red.

            if (sibling->leftChild != nullptr && sibling->rightChild != nullptr)
            {
                /*
                    X: Remove; R: Red; B: Black
                      R
                     / \
                    X   B
                       / \
                      R   R   Or her symmetrical counterpart
                */
                // sibling's children are red
                // note: If her black uncle has child, his child must be red
                // operation: rotate sibling, and rotate parent. set parent to black, sibling to red.
                sibling->color = NodeColor::RED;
                currentFather->color = NodeColor::BLACK;

                if (siblingSideToFather == ChildSide::RIGHT) {
                    sibling->rightChild->color = NodeColor::BLACK;
                    this->rotateLeft(currentFather);
                }
                else {
                    sibling->leftChild->color = NodeColor::BLACK;
                    this->rotateRight(currentFather);
                }
            }
            else if (siblingSideToFather == ChildSide::RIGHT && sibling->leftChild != nullptr)
            {
                /*
                    X: Remove; R: Red; B: Black
                      R
                     / \
                    X   B
                       /
                      R
                */

                currentFather->color = NodeColor::BLACK;
                this->rotateRight(sibling);
                this->rotateLeft(currentFather);
            }
            else if (siblingSideToFather == ChildSide::LEFT && sibling->rightChild != nullptr)
            {
                /*
                    X: Remove; R: Red; B: Black
                      R
                     / \
                    B   X
                     \
                      R
                */

                currentFather->color = NodeColor::BLACK;
                this->rotateLeft(sibling);
                this->rotateRight(currentFather);
            }
            else if (siblingSideToFather == ChildSide::RIGHT && sibling->rightChild != nullptr) {
                /*
                    X: Remove; R: Red; B: Black
                      R
                     / \
                    X   B
                         \
                          R
                */
                currentFather->color = NodeColor::BLACK;
                sibling->color = NodeColor::RED;
                sibling->rightChild->color = NodeColor::BLACK;
                this->rotateLeft(currentFather);
            }
            else if (siblingSideToFather == ChildSide::LEFT && sibling->leftChild != nullptr)
            {
                /*
                    X: Remove; R: Red; B: Black
                        R
                       / \
                      B   X
                     /
                    R
                */
                currentFather->color = NodeColor::BLACK;
                sibling->color = NodeColor::RED;
                sibling->leftChild->color = NodeColor::BLACK;
                this->rotateRight(currentFather);
            }
            else { // sibling has no child
                sibling->color = NodeColor::RED;
                currentFather->color = NodeColor::BLACK;
            }

        } // currentFather->color == NodeColor::RED
        else { // currentFather->color == NodeColor::BLACK
            if (sibling->color == NodeColor::BLACK
                && sibling->leftChild != nullptr && sibling->rightChild != nullptr)
            {
                // sibling is black, and it has two children. then these children must be red.
                if (siblingSideToFather == ChildSide::RIGHT) {
                    /*
                        X: Remove; R: Red; B: Black
                            B
                           / \
                          X   B
                             / \
                            R   R
                    */
                    sibling->leftChild->color = NodeColor::BLACK;
                    this->rotateRight(sibling);
                    this->rotateLeft(currentFather);
                }
                else {
                    /*
                        X: Remove; R: Red; B: Black
                            B
                           / \
                          B   X
                         / \
                        R   R
                    */
                    sibling->rightChild->color = NodeColor::BLACK;
                    this->rotateLeft(sibling);
                    this->rotateRight(currentFather);
                }
            } 
            else if (sibling->color == NodeColor::BLACK &&
                (sibling->leftChild != nullptr || sibling->rightChild != nullptr))
            {
                // sibling is black, and has one red child.
                if (siblingSideToFather == ChildSide::RIGHT && sibling->rightChild != nullptr) {
                    /*
                        X: Remove; R: Red; B: Black
                            B
                           / \
                          X   B
                               \
                                R
                    */
                    sibling->rightChild->color = NodeColor::BLACK;
                    this->rotateLeft(currentFather);
                }
                else if (siblingSideToFather == ChildSide::RIGHT && sibling->leftChild != nullptr)
                {
                    /*
                        X: Remove; R: Red; B: Black
                            B
                           / \
                          X   B
                             /
                            R
                    */
                    sibling->leftChild->color = NodeColor::BLACK;
                    this->rotateRight(sibling);
                    this->rotateLeft(currentFather);
                }
                else if (siblingSideToFather == ChildSide::LEFT && sibling->leftChild != nullptr)
                {
                    /*
                        X: Remove; R: Red; B: Black
                            B
                           / \
                          B   X
                         /
                        R
                    */
                    sibling->leftChild->color = NodeColor::BLACK;
                    this->rotateRight(currentFather);
                }
                else {
                    /*
                        X: Remove; R: Red; B: Black
                            B
                           / \
                          B   X
                           \
                            R
                    */
                    sibling->rightChild->color = NodeColor::BLACK;
                    this->rotateLeft(sibling);
                    this->rotateRight(currentFather);
                }
            } 
            else if (sibling->color == NodeColor::BLACK) {
                // black sibling without child
                /*
                       B          B
                      / \   ->   / \
                     X   B      X   R
                */
                if (currentFather->leftChild == nullptr) {
                    currentFather->rightChild->color = NodeColor::RED;
                    this->rebalanceChildren(currentFather);
                }
                else {
                    currentFather->leftChild->color = NodeColor::RED;
                    this->rebalanceChildren(currentFather);
                }
            } 
            else {
                // sibling is red. then it must has two black children.
                sibling->color = NodeColor::BLACK;
                currentFather->color = NodeColor::RED;
                if (siblingSideToFather == ChildSide::RIGHT) {
                    /*
                           B
                          / \
                         X   R
                            / \
                           B   B
                    */
                    this->rotateLeft(currentFather);
                    this->rotateLeft(currentFather);
                    if (currentFather->rightChild != nullptr) {
                        this->rebalanceRedNode(currentFather->rightChild);
                    }
                }
                else {
                    /*
                           B
                          / \
                         R   X
                        / \
                       B   B
                    */
                    this->rotateRight(currentFather);
                    this->rotateRight(currentFather);
                    if (currentFather->leftChild != nullptr) {
                        this->rebalanceRedNode(currentFather->leftChild);
                    }
                }
            }
        }
    }

    return *this;
}


template<typename KeyType, typename DataType>
void RedBlackTree<KeyType, DataType>::cleanup(Node* node)
{
    if (node->leftChild != nullptr) {
        cleanup(node->leftChild);
    }
    if (node->rightChild != nullptr) {
        cleanup(node->rightChild);
    }
    allocator->free(node, sizeof(Node));
}


template<typename KeyType, typename DataType>
void RedBlackTree<KeyType, DataType>::rotateLeft(Node* node)
{
    Node* father = node->father;
    Node* targetRoot = node->rightChild;

    // rebind root.
    if (father == nullptr) {
        this->root = targetRoot;
    }
    else {
        if (node == father->leftChild) {
            father->leftChild = targetRoot;
        }
        else {
            father->rightChild = targetRoot;
        }
    }
    targetRoot->father = father;

    node->rightChild = targetRoot->leftChild; // child might be nullptr
    if (node->rightChild != nullptr) { // only when child is not null then we can bind parent node.
        node->rightChild->father = node;
    }
    targetRoot->leftChild = node;
    node->father = targetRoot;
}


template<typename KeyType, typename DataType>
void RedBlackTree<KeyType, DataType>::rotateRight(Node* node)
{
    Node* father = node->father;
    Node* targetRoot = node->leftChild;

    if (father == nullptr) {
        this->root = targetRoot;
    }
    else {
        if (node == father->leftChild) {
            father->leftChild = targetRoot;
        }
        else {
            father->rightChild = targetRoot;
        }
    }
    targetRoot->father = father;

    node->leftChild = targetRoot->rightChild; // child might be nullptr
    if (node->leftChild != nullptr) { 
        node->leftChild->father = node;
    }
    targetRoot->rightChild = node;
    node->father = targetRoot;
}


template<typename KeyType, typename DataType>
void RedBlackTree<KeyType, DataType>::rebalanceRedNode(Node* node)
{
    Node* currentNode = node;
    
    // Only when the current node is red is it possible to violate the red-black tree rule 
    // when the parent node is also red, thus requiring a repair operation.
    while (currentNode->color == NodeColor::RED) {
        Node* currentFather = currentNode->father;
        if (currentFather == nullptr) {
            // currentNode is root
            currentNode->color = NodeColor::BLACK;
            break;
        }
        else if (currentFather->color == NodeColor::BLACK) {
            // parent is black. no problem!
            break;
        }


        // Arriving here indicates that both the parent and target nodes are colored red.

        // If a parent node is red, then it cannot be the root, so a grandparent must exist.

        Node* currentGrandpa = currentFather->father;

        // find uncle (might be null)
        Node* uncle =
            (currentGrandpa->leftChild != currentFather ?
                currentGrandpa->leftChild : currentGrandpa->rightChild);

        if (uncle != nullptr && uncle->color == NodeColor::RED) {
            
            uncle->color = NodeColor::BLACK;
            currentFather->color = NodeColor::BLACK;
            currentGrandpa->color = NodeColor::RED;

            currentNode = currentGrandpa;
        }
        else {
            
            if (currentFather == currentGrandpa->leftChild) {
                if (currentNode == currentFather->leftChild) {
                    this->rotateRight(currentGrandpa);
                    // recolor
                    currentFather->color = NodeColor::BLACK;
                    currentGrandpa->color = NodeColor::RED;
                }
                else {
                    this->rotateLeft(currentFather);
                    this->rotateRight(currentGrandpa);
                    // recolor
                    currentGrandpa->color = NodeColor::RED;
                    currentNode->color = NodeColor::BLACK;
                }
            } // if (currentFather == currentGrandpa->leftChild)
            else {

                if (currentNode == currentFather->rightChild) {
                    this->rotateLeft(currentGrandpa);
                    // recolor
                    currentFather->color = NodeColor::BLACK;
                    currentGrandpa->color = NodeColor::RED;
                }
                else {
                    this->rotateRight(currentFather);
                    this->rotateLeft(currentGrandpa);
                    // recolor
                    currentGrandpa->color = NodeColor::RED;
                    currentNode->color = NodeColor::BLACK;
                }
            } // if (currentFather != currentGrandpa->leftChild)

            return;
        }
    }
}


template<typename KeyType, typename DataType>
void RedBlackTree<KeyType, DataType>::rebalanceChildren(Node* node)
{
    Node* currentNode = node;

    while (currentNode != this->root) {
        Node* currentFather = currentNode->father;
        Node* sibling = (currentFather->leftChild != currentNode ?
            currentFather->leftChild : currentFather->rightChild);
        ChildSide siblingSideToFather = 
            (sibling == currentFather->leftChild ? ChildSide::LEFT : ChildSide::RIGHT);

        if (currentFather->color == NodeColor::RED) {
            
            
            if (siblingSideToFather == ChildSide::RIGHT
                && sibling->leftChild->color == NodeColor::BLACK)
            {
                /*
                       R
                      / \
                     X   B
                        / \
                       B   ?
                */
                this->rotateLeft(currentFather);
                break; 
            }
            else if (siblingSideToFather == ChildSide::LEFT
                && sibling->rightChild->color == NodeColor::BLACK)
            {
                /*
                        R
                       / \
                      B   X
                     / \
                    ?   B
                */
                this->rotateRight(currentFather);
                break;
            }
            else if (siblingSideToFather == ChildSide::RIGHT
                && sibling->rightChild->color == NodeColor::BLACK)
            {
                /*
                       R
                      / \
                     X   B
                        / \
                       R   B
                */
                currentFather->color = NodeColor::BLACK;
                sibling->color = NodeColor::RED;
                this->rebalanceRedNode(sibling->leftChild);
                break;
            }
            else if (siblingSideToFather == ChildSide::LEFT
                && sibling->leftChild->color == NodeColor::BLACK)
            {
                /*
                        R
                       / \
                      B   X
                     / \
                    B   R
                */
                currentFather->color = NodeColor::BLACK;
                sibling->color = NodeColor::RED;
                this->rebalanceRedNode(sibling->rightChild);
                break;
            }
            else { // sibling has two red child
                currentFather->color = NodeColor::BLACK;
                sibling->color = NodeColor::RED;
                if (siblingSideToFather == ChildSide::RIGHT) {
                    sibling->rightChild->color = NodeColor::BLACK;
                    this->rotateLeft(currentFather);
                }
                else {
                    sibling->leftChild->color = NodeColor::BLACK;
                    this->rotateRight(currentFather);
                }
                break;
            }
        } 
        else { // parent is black
            if (sibling->color == NodeColor::RED) { // sibling is red
                /*
                        B
                       / \
                      X   R
                         / \
                        B   B

                    or her symmatrical counterpart
                */
                currentFather->color = NodeColor::RED;
                sibling->color = NodeColor::BLACK;
                if (siblingSideToFather == ChildSide::RIGHT) {
                    
                    this->rotateLeft(currentFather);
                }
                else {
                    this->rotateRight(currentFather);
                }
            } 
            else { // sibling is black
                if (sibling->leftChild->color == NodeColor::BLACK 
                    && sibling->rightChild->color == NodeColor::BLACK)
                {
                    /*
                          B
                         / \
                        X   B
                           / \
                          B   B

                        or her symmatrical counterpart
                    */
                    sibling->color = NodeColor::RED;
                    currentNode = currentFather;
                }
                else if (siblingSideToFather == ChildSide::RIGHT 
                    && sibling->rightChild->color == NodeColor::RED) 
                {
                    /*
                          B
                         / \
                        X   B
                           / \
                          ?   R

                    */
                    sibling->rightChild->color = NodeColor::BLACK;
                    this->rotateLeft(currentFather);
                    break;
                }
                else if (siblingSideToFather == ChildSide::LEFT
                    && sibling->leftChild->color == NodeColor::RED)
                {
                    /*
                            B
                           / \
                          B   X
                         / \
                        R   ?

                    */
                    sibling->leftChild->color = NodeColor::BLACK;
                    this->rotateRight(currentFather);
                    break;
                }
                else if (siblingSideToFather == ChildSide::RIGHT) 
                {
                    /*
                          B
                         / \
                        X   B
                           / \
                          R   B

                    */
                    sibling->leftChild->color = NodeColor::BLACK;
                    this->rotateRight(sibling);
                    this->rotateLeft(currentFather);
                    break;
                }
                else 
                {
                    /*
                            B
                           / \
                          B   X
                         / \
                        B   R

                    */
                    sibling->rightChild->color = NodeColor::BLACK;
                    this->rotateLeft(sibling);
                    this->rotateRight(currentFather);
                    break;
                }
            } 
        } 
    }
}


}  // namespace MtsysKv

