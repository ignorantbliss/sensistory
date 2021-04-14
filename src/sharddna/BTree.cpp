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

//#include "pch.h"
#include "BTree.h"
#include "DataStore.h"
#include "Historian.h"
#include "Cursor.h"

BTree::BTree()
{
	SplitMode = BTREE_SPLITMODE_CENTER;
	Root = NULL;
	MaxKeysPerNode = 0;	
	KeyLength = 32;
	DS = NULL;
	Location = 0;
	MaxKeysPerTrunkNode = 0;
}


BTree::~BTree()
{
	if (Root != NULL)
		delete Root;
}

std::streamoff BTree::Create(DStore Ds)
{
	std::streamoff of = _Create(Ds);
	if (of > 0)
	{
		//Historian::Instance->Log(LOGLEVEL_INFO, "BTree Base", "Creating BTree Root");
		Location = of;
		Root = new BTreeNode(this->shared_from_this());
		Root->Create(Ds.get(), true);
		Root->Next = 0;
		Root->Prev = 0;
		if (Root->Write())
		{
			return of;
		}
	}
	return 0;
}

std::streamoff BTree::_Create(DStore Ds)
{	
	DS = Ds;
	std::streamoff offset = DS->AllocateBlock();

	int Overhead = sizeof(std::streamoff) * 2;

	KeyLength = BranchTemplate.DiskKeySize();

	int MaxKeys = (int)floor((DS->BlockSize() - Overhead) / KeyLength);
	MaxKeysPerTrunkNode = MaxKeys;
	int TotalBlockSize = (KeyLength * MaxKeysPerNode) + Overhead;

	MaxKeys = (int)floor((DS->BlockSize() - Overhead) / LeafTemplate.DiskKeySize());
	MaxKeysPerNode = MaxKeys;

	char* buf = DS->InitBlock();
	memcpy(buf, &KeyLength, sizeof(KeyLength));
	memcpy(buf + sizeof(KeyLength), &MaxKeysPerNode, sizeof(MaxKeysPerNode));
	memcpy(buf + sizeof(KeyLength) + sizeof(MaxKeysPerNode), &MaxKeysPerTrunkNode, sizeof(MaxKeysPerTrunkNode));
	DS->Mark(buf, "BTREE", 5);
	DS->WriteBlock(offset,buf);
	delete []buf;
	
	//Historian::Instance->Log(LOGLEVEL_INFO, "BTree Load", "BTree Loaded");

	return offset;
}

bool BTree::Load(DStore ds, std::streamoff offset)
{
	DS = ds;
	if (!DS->PosAvailable(offset))
	{
		std::streamoff of = _Create(ds);
		if (of > 0)
		{
			//Historian::Instance->Log(LOGLEVEL_INFO, "BTree Base", "Creating BTree Root");
			Location = of;
			Root = new BTreeNode(this->shared_from_this());
			Root->Create(DS.get(),true);
			Root->Next = 0;
			Root->Prev = 0;
			return Root->Write();			
		}
		return false;
	}
		
	//Historian::Instance->Log(LOGLEVEL_INFO, "BTree Base", "Reading BTree Root");
	char* buf = DS->ReadBlock(offset);
	if (!LoadHeader(buf, DS->BlockSize()))
	{
		return false;
	}

	Location = offset;

	if (Root == NULL)
	{
		std::shared_ptr<BTree> PBT = this->shared_from_this();
		Root = new BTreeNode(PBT);
	}

	if (!Root->Load(DS.get(), Location + DS->BlockSize()))
		return false;

	//Historian::Instance->Log(LOGLEVEL_INFO, "BTree Base", "BTree Root Loaded");

	return true;
}

bool BTree::LoadHeader(const char* buf, int ln)
{
	int offset = 0;
	memcpy(&KeyLength, buf, sizeof(KeyLength));
	offset += sizeof(KeyLength);
	memcpy(&MaxKeysPerNode, buf + offset, sizeof(MaxKeysPerNode));
	offset += sizeof(MaxKeysPerTrunkNode);
	memcpy(&MaxKeysPerTrunkNode, buf + offset, sizeof(MaxKeysPerTrunkNode));
	return true;
}

int BTree::KeysPerNode(bool leaf)
{
	if (leaf == true)
		return MaxKeysPerNode;
	else
		return MaxKeysPerTrunkNode;
}

BTreeKey* BTree::PointSearch(const void* key, int ln, int mode)
{	
	char* filled = NULL;

	if (ln < LeafTemplate.KeySize)
	{
		filled = new char[LeafTemplate.KeySize];
		memset(filled, 0, LeafTemplate.KeySize);
		memcpy(filled, key, ln);
	}

	if (filled != NULL)
	{
		BTreeKey *N = _PointSearch(filled,mode);
		delete []filled;
		return N;
	}
	else
	{
		return _PointSearch(key,mode);
	}	
}

PCursor BTree::Search(const void* key, int ln, int mode)
{
	char* filled = NULL;

	if (ln < LeafTemplate.KeySize)
	{
		filled = new char[LeafTemplate.KeySize];
		memset(filled, 0, LeafTemplate.KeySize);
		memcpy(filled, key, ln);
	}

	if (filled != NULL)
	{
		PCursor N = _Search(filled, mode);
		delete[]filled;
		return N;
	}
	else
	{
		return _Search(key, mode);
	}
}

bool BTree::Add(const void* key, int kln, const void* value, int vln)
{
	char* kfilled = NULL;
	char* vfilled = NULL;

	const void* kin = key;
	const void* vin = value;

	if (kln < LeafTemplate.KeySize)
	{
		kfilled = new char[LeafTemplate.KeySize];
		memset(kfilled, 0, LeafTemplate.KeySize);
		memcpy(kfilled, key, kln);
		kin = kfilled;
	}

	if (vln < LeafTemplate.PayloadSize)
	{
		vfilled = new char[LeafTemplate.PayloadSize];
		memset(vfilled, 0, LeafTemplate.PayloadSize);
		memcpy(vfilled, value, vln);
		vin = vfilled;
	}

	bool k = _Add(kin, vin);

	if (kfilled != NULL)
	{
		delete[] kfilled;
	}
	if (vfilled != NULL)
	{
		delete[] vfilled;
	}
	return k;
}

bool BTree::_Add(const void* key, const void* val)
{
	BTreeSearchChain Chain = Root->Search(key, BTREE_SEARCH_PREVIOUS);	
	if (Chain.Key != NULL)
	{
		delete Chain.Key;
		Chain.Key = NULL;
	}

	if (Chain.found == false)
	{
		//Unable to find exact match - add at closest node.
		BTreeNode* Nd = new BTreeNode(this->shared_from_this());
		if (Chain.sequence.size() == 0)
		{
			ChainElement CE;
			CE.location = Root->position;
			CE.index = -1;
			CE.changed = false;
			Chain.sequence.push_back(CE);
		}
		if (Nd->Load(this->DS.get(), Chain.sequence[Chain.sequence.size() - 1].location))
		{
			Nd->_Add(key, val, &Chain,Chain.sequence.size()-1,SplitMode);			
			delete Nd;
			if (Chain.sequence[0].changed == true)
			{
				Root->Load(DS.get(),Root->position);
			}
			return true;
		}		
	}	
	return NULL;
}

BTreeKey* BTree::_PointSearch(const void* key, int mode)
{
	//Begin Search of the Tree...
	BTreeSearchChain Chain = Root->Search(key,mode);
	if (Chain.Key != NULL)
		return Chain.Key;
	return NULL;
}

PCursor BTree::_Search(const void* key, int mode)
{
	//Begin Search of the Tree...
	PCursor Csr(new Cursor());

	BTreeSearchChain Chain = Root->Search(key, mode);
	if (Chain.Key != NULL)
	{
		Csr->SetPosition(Chain.sequence[Chain.sequence.size() - 1].location, Chain.sequence[Chain.sequence.size() - 1].index);
		return Csr;
	}

	Csr->End();
	return Csr;
}

DStore BTree::GetDataStore()
{
	return DS;
}

PCursor BTree::FirstNode()
{
	BTreeNode* Rt = Root->FirstLeaf();
	PCursor P(new Cursor());
	P->SetPosition(Rt->position, 0);	
	P->SetTree(Rt->Base->shared_from_this());
	return P;
}