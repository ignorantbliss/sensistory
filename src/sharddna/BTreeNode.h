#pragma once

#include <iostream>
#include <vector>
#include "BTree.h"

using namespace std;

#define BTREE_SPLITMODE_CENTER	1
#define BTREE_SPLITMODE_END 0

class DataStore;
class BTreeKey;
class BTree;

struct ChainElement
{
	std::streamoff location;
	int index;
	bool changed;
};

class BTreeSearchChain
{
public:
	BTreeSearchChain();
	~BTreeSearchChain();
	
	BTreeKey* Key;	
	std::vector<ChainElement> sequence;
	bool found;
};

#define SEARCHMODE_EXACT	0

class BTreeNode
{
public:
	BTreeNode(std::shared_ptr<BTree> b);
	~BTreeNode();	

	void Copy(BTreeNode* N);

	std::streamoff Create(DataStore* ds, bool leaf);
	bool Load(DataStore *ds,std::streamoff offset);
	int KeyCount();	
	bool Write();

	BTreeSearchChain Search(const void* key, int mode);
	BTreeKey** Keys;
	BTreeNode* FirstLeaf();
	bool leaf;

	std::streamoff Next;
	std::streamoff Prev;

	friend class BTree;
	
protected:
	DataStore *datastore;	
	std::shared_ptr<BTree> Base;
	std::streamoff position;	
	bool _Search(const void* key, int mode, BTreeSearchChain* Chain);
	bool _Add(const void* key, const void* value, BTreeSearchChain* Chain, int Depth, int splitmode);
	std::streamoff _Split(int splitpoint);

	signed short _KeyCount;
};

