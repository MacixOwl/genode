/**
 * Red Black Tree H
 * by GTY
 * 2022.1
 * at Yushan County, Shangrao, Jiangxi
 */

// 2024.11.6: modified for Amkos

// To use RBTree, you should include RedBlackTree.hpp

#pragma once

#include <base/stdint.h>

namespace Genode {
	class Allocator;
	class Exception;
	class Mutex;
}


namespace MtsysKv {

template <typename KeyType, typename DataType>
class RedBlackTree {

public:
	/** Object's life-management methods */
	RedBlackTree(Genode::Allocator*);
	~RedBlackTree();

	/**
	 * Clear all elements in tree.
	 */
	void clear();

public:
	/** Basic query methods */

	/**
	 * Determine whether a key is in the tree.
	 * 
	 * @param queryKey
	 */
	bool hasKey(const KeyType& queryKey);

	/**
	 * Get data by key.
	 * 
	 * @param key 
	 */
	DataType& getData(const KeyType& key);

	/**
	 * Set data. If data with same key already exists, it would be overwritten.
	 * 
	 * @param key 
	 * @param data 
	 */
	RedBlackTree<KeyType, DataType>& setData(const KeyType& key, const DataType& data);

	/**
	 * Delete key.
	 * 
	 * @param key
	 */
	RedBlackTree<KeyType, DataType>& removeKey(const KeyType& key);

	/**
	 * Range scan. This method will search for keys inside [lhs, rhs],
	 * and calls collector with data for each node discovered.
	 * 
	 * @param lhs inclusive
	 * @param rhs inclusive
	 * @param data to be passed to collector
	 * @param collector 
	 * @return How many elements collected.
	 */
	Genode::size_t rangeScan(
		const KeyType& lhs, 
		const KeyType& rhs, 
		void* data,
		bool (*collector) (void* data, const KeyType&, const DataType&)
	);


public:
	struct RuntimeError : Genode::Exception {};


protected:
	enum class NodeColor {
		RED, BLACK
	};
	enum class ChildSide {
		LEFT, RIGHT
	};
	struct Node {
		KeyType key;
		DataType data;
		NodeColor color = NodeColor::RED;
		Node* father = nullptr;
		Node* leftChild = nullptr;
		Node* rightChild = nullptr;
	};

protected:
	/**
	 * Release one node with all of its children, recursively.
	 * 
	 * @param node first node to be released recursively
	 */
	void cleanup(Node* node);


	/**
	 * Non-locked version of range scan. Designed to be called by public rangeScan method.
	 */
	Genode::size_t doRangeScan(
		const Node* node,
		const KeyType& lhs, 
		const KeyType& rhs, 
		void* data,
		bool (*collector) (void* data, const KeyType&, const DataType&)
	);


	/**
	 * Rotate left.
	 * 
	 * @param node 
	 * @exception If `node` dosen't have right subtree, undefined behaviour occurs.
	 */
	void rotateLeft(Node* node);

	/**
	 * Rotate right
	 * 
	 * @param node 
	 * @exception If `node` dosen't have left subtree, undefined behaviour occurs
	 */
	void rotateRight(Node* node);

	/**
	 * 
	 * 
	 * @param node Child node of the two connected red nodes.
	 * @exception If node is nullptr, UB would occur.
	 */
	void rebalanceRedNode(Node* node);

	/**
	 * 
	 * 
	 * @param node Lighter node.
	 * @exception UB if node is not the lighter one.
	 */
	void rebalanceChildren(Node* node);

protected:
	/**
	 * Root node.
	 */
	Node* root = nullptr;

	Genode::Allocator* allocator = nullptr;
	Genode::Mutex mutex;

};


}  // namespace MtsysKv
