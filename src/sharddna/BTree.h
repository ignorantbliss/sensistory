/*
 * This file is part of PRODNAME.
 *
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "BTreeNode.h"
#include "BTreeKey.h"
#include "DataStore.h"
#include "Cursor.h"
#include <fstream>

//Search and PointSearch should only return if there is an EXACT key match
#define BTREE_SEARCH_EXACT		0
//Search should result in the closest, previous match unless there is an exact match.
#define BTREE_SEARCH_PREVIOUS	1
//Search should result in the closest, next match unless there is an exact match.
#define BTREE_SEARCH_NEXT	2
//As BTREE_SEARCH_NEXTEXC, but will NOT exact match.
#define BTREE_SEARCH_NEXTEXC	3

#define PBTREE std::shared_ptr<BTree>

using namespace std;

class DataStore;
class BTreeNode;

#define PCursor std::shared_ptr<Cursor>
class Cursor;

/**
 * Represents a single BTree structure on disk
 * 
 * This class is used to utilise a single BTree index on a Datastore. Note that a single Datastore may contain a large number of different
 * btrees.
 *
 */
class BTree : public std::enable_shared_from_this<BTree>
{
public:
	/**
	* Constructs a BTree instance.
	* Used to create a new, empty BTree instance.
	*/
	BTree();

	/**
	* Destructor.
	* Destroys the BTree
	*/
	~BTree();

	BTreeNode *Root;					//Stores the root node of the BTree
	BTreeKeyTemplate LeafTemplate;		//Describes the key length and content of LEAF nodes
	BTreeKeyTemplate BranchTemplate;	//Describes the key length and content of NON-LEAF nodes

	/**
	 * @brief Creates a new BTree in a Datastore
	 * @param DS The datastore to add to
	 * @return The disk location of the BTree
	*/
	std::streamoff Create(DStore DS);	
	bool Load(DStore ds, std::streamoff offset);	//Loads a BTree root from the given location in a Datastore
	int KeysPerNode(bool leaf);			//Returns the maximum number of keys per BTree node
	int KeyLength;						
	int SplitMode;						//The splitting mode - how the BTree nodes should split when adding new data.
	
	BTreeKey* PointSearch(const void* key, int ln, int mode);
	PCursor Search(const void* key, int ln, int mode);
	bool Add(const void* key, int keylen, const void* payload, int plen);
	PCursor FirstNode();

	/**
	 * @brief Returns the data store used by this BTree
	 * @return A smart pointer to a Datastore
	*/
	DStore GetDataStore();

	friend class Cursor;

private:
	
	bool LoadHeader(const char *buf, int cnt);
	BTreeKey* _PointSearch(const void* key, int mode);
	PCursor _Search(const void* key, int mode);
	bool _Add(const void* key, const void* value);
	std::streamoff _Create(DStore DS);

	//int KeyLength;
	int MaxKeysPerNode;
	int MaxKeysPerTrunkNode;	
	DStore DS;
	streamoff Location;
};

