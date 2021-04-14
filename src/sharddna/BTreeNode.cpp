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
#include "BTreeNode.h"

#include "DataStore.h"


BTreeNode::BTreeNode(std::shared_ptr<BTree> b)
{
	_KeyCount = 0;
	datastore = NULL;
	position = 0;
	Keys = NULL;
	Base = b;
	leaf = false;
	Next = 0;
	Prev = 0;
}


BTreeNode::~BTreeNode()
{
	if (Keys != NULL)
	{
		for (int x = 0; x < _KeyCount; x++)
		{
			if (Keys[x] != NULL)
				delete Keys[x];
		}
		delete[]Keys;
		Keys = NULL;
	}
}

int BTreeNode::KeyCount()
{
	return _KeyCount;
}

bool BTreeNode::Load(DataStore *ds, std::streamoff pos)
{
	datastore = ds;
	position = pos;

	int offset = 0;

	char *buf = datastore->ReadBlock(position);
	if (buf == NULL) return false;

	memcpy(&_KeyCount, buf, sizeof(short));		
	if (_KeyCount < 0)
	{
		_KeyCount = -(_KeyCount + 1);
		leaf = true;
	}
	else
	{
		leaf = false;
	}
	offset += sizeof(short);

	memcpy( &Prev, buf + offset,  sizeof(std::streamoff));
	offset += sizeof(std::streamoff);

	memcpy(&Next, buf + offset, sizeof(std::streamoff));
	offset += sizeof(std::streamoff);

	Keys = new BTreeKey * [Base->KeysPerNode(leaf)];
	if (_KeyCount > Base->KeysPerNode(leaf))
	{
		cout << "CRITICAL FILE ERROR: Node contains too many keys!" << endl;
		delete buf;
		return false;
	}

	BTreeKeyTemplate* Tmpl = &Base->BranchTemplate;
	if (leaf == true) Tmpl = &Base->LeafTemplate;

	for (int x = 0; x < Base->KeysPerNode(leaf); x++)
	{
		Keys[x] = NULL;
	}
	

	for (int x = 0; x < _KeyCount; x++)
	{		
		if (x < _KeyCount)
		{
			Keys[x] = new BTreeKey(Tmpl);
			offset += Keys[x]->Read(buf + offset);
		}		
	}

	delete buf;
	return true;
}

std::streamoff BTreeNode::Create(DataStore* ds, bool leaf)
{
	datastore = ds;
	position = ds->AllocateBlock();

	this->leaf = leaf;

	BTreeKeyTemplate* Tmpl = &Base->BranchTemplate;
	if (leaf == true) Tmpl = &Base->LeafTemplate;

	int KeyCount = Base->KeysPerNode(leaf);
	Keys = new BTreeKey * [Base->KeysPerNode(leaf)];
	for (int x = 0; x < KeyCount; x++)
	{
		Keys[x] = new BTreeKey(Tmpl);
	}

	return position;// Write();
}

bool BTreeNode::Write()
{
	int offset = 0;
	char *buf = datastore->InitBlock();
	short s = _KeyCount;
	if (leaf == true)
		s = -_KeyCount-1;
	memcpy(buf, &s, 2);
	offset+=2;
	memcpy(buf + offset, &Prev, sizeof(std::streamoff));
	offset += sizeof(std::streamoff);
	memcpy(buf + offset, &Next, sizeof(std::streamoff));
	offset += sizeof(std::streamoff);

	if (_KeyCount > 0)
	{
		int tkeylen = Keys[0]->Length();
		//int offset = 1;
		for (int x = 0; x < _KeyCount; x++)
		{
			Keys[x]->Write(buf + offset);
			offset += tkeylen;
		}
	}

	datastore->Mark(buf, "BTREN", 5);
	datastore->WriteBlock(position, buf);
	delete []buf;
	return true;
}

BTreeSearchChain BTreeNode::Search(const void* key, int mode)
{
	BTreeSearchChain Chain;
	ChainElement CE;
	CE.location = this->position;
	CE.index = -1;
	CE.changed = false;
	Chain.sequence.push_back(CE);
	_Search(key, mode, &Chain);
	return Chain;
}

bool BTreeNode::_Search(const void* key, int mode, BTreeSearchChain* Chain)
{
	BTreeKey* Closest = NULL;
	int cmp = 0;
	
	int ClosestIndex = 0;
	int HighestIndex = -1;
	
	for (int x = 0; x < _KeyCount; x++)
	{
		if (Keys[x] != NULL)
		{
			if ((Keys[x]->Code() & KEYCODE_DELETED) > 0)
				continue;

			cmp = Keys[x]->CompareTo(key);			

			if (cmp == 0)
			{
				if ((mode != BTREE_SEARCH_NEXTEXC) || ((leaf == false) && (mode == BTREE_SEARCH_NEXTEXC)))
				{
					if (leaf == true)
					{
						Closest = Keys[x];
						Chain->Key = new BTreeKey(*Closest);
						return true;
					}
					else
					{
						Closest = Keys[x];
						ClosestIndex = x;
						break;
					}
				}
			}
			if (cmp < 0)
			{
				Closest = Keys[x];
				ClosestIndex = x;
			}
			if (cmp > 0)
			{
				if (mode == BTREE_SEARCH_PREVIOUS)
				{
					if (Closest != NULL)
					{
						Chain->Key = new BTreeKey(*Closest);	//Closest
					}
					else
					{
						Closest = Keys[0];
						Chain->Key = new BTreeKey(*Keys[0]);    //First						
					}
					ClosestIndex = x-1;
					break;
				}
				if ((mode == BTREE_SEARCH_NEXT) || (mode == BTREE_SEARCH_NEXTEXC))
				{
					Closest = Keys[x];					
					ClosestIndex = x;
					HighestIndex = x;
					Chain->Key = new BTreeKey(*Keys[x]);	//Closest										
				}
			}
		}
	}
	if ((leaf == true) && (mode == BTREE_SEARCH_NEXT))
	{
		if (HighestIndex == -1)
		{
			Closest = NULL;
			ClosestIndex = -1;
		}
	}

	if (Closest != NULL)
	{
		if (leaf == false)
		{
			if (Base->Root->position == this->position)
			{
				Chain->sequence[Chain->sequence.size()-1].index = ClosestIndex;
			}
			std::streamoff offset = *(std::streamoff *)Closest->Payload();
			BTreeNode *N = new BTreeNode(Base);
			N->Load(datastore, offset);

			ChainElement CE;
			CE.location = N->position;
			CE.index = -1;
			Chain->sequence.push_back(CE);
			N->_Search(key, mode, Chain);
			if (Chain->Key != NULL)
			{
				delete N;
				return true;
			}
			Chain->sequence.pop_back();
			delete N;
		}
		else
		{
			if ((mode == BTREE_SEARCH_PREVIOUS) || (mode == BTREE_SEARCH_NEXT) || (mode == BTREE_SEARCH_NEXTEXC))
			{
				Chain->Key = new BTreeKey(*Closest);				
				Chain->sequence[Chain->sequence.size() - 1].index = ClosestIndex + 1;
				return Closest;
			}
		}
	}

	//Chain->sequence.pop_back();

	return false;
}

std::streamoff BTreeNode::_Split(int SplitPoint)
{
	//Figure out how many we are splitting off...
	int RemoveTo = (int)(_KeyCount / 2);
	if (SplitPoint == BTREE_SPLITMODE_END)
	{
		RemoveTo = _KeyCount;
	}

	//Create the new Node...
	BTreeNode* A = new BTreeNode(Base);	
	A->Create(datastore,leaf);	

	//Transfer the excess keys...
	int DestIndex = 0;
	for (int x = RemoveTo + 1; x < _KeyCount; x++)
	{
		A->Keys[DestIndex]->Set(Keys[x]->Key(), Keys[x]->Payload());
		DestIndex++;
	}
	A->_KeyCount = _KeyCount - RemoveTo;

	_KeyCount = RemoveTo;
	Write();
	Next = A->position;
	A->Write();
	std::streamoff Pos = A->position;
	delete A;
	return Pos;
}

bool BTreeNode::_Add(const void* key, const void* value, BTreeSearchChain* Chain, int Depth, int SplitMode)
{
	//Is there room in here?
	
	//Re-calculate the split point...
	BTreeKey* Closest = NULL;
	int cmp = 0;

	int ClosestIndex = 0;
	int HighestIndex = -1;

	for (int x = 0; x <= _KeyCount; x++)
	{
		if (x == _KeyCount)
		{
			ClosestIndex = x;
			break;
		}
		if (Keys[x] != NULL)
		{
			cmp = Keys[x]->CompareTo(key);

			if (cmp == 0)
			{
				//Existing Key!
				return true;
			}
			if (cmp < 0)
			{
				Closest = Keys[x];
				ClosestIndex = x;
			}
			if (cmp > 0)
			{				
				ClosestIndex = x - 1;
				break;				
			}
		}
	}

	Chain->sequence[Depth].index = ClosestIndex;

	//Now, begin adding...
	Chain->sequence[Depth].changed = true;	
	if (_KeyCount >= this->Base->KeysPerNode(leaf))
	{
		//This won't fit here - begin splitting a key
		if (Depth > 0)
		{
			//Create a new, split mode.
			std::streamoff newnode = _Split(SplitMode);
			BTreeNode* NewNode = new BTreeNode(Base);
			NewNode->Load(datastore, newnode);
			Next = newnode;

			//Figure out which split the new key lives in...
			if (NewNode->_KeyCount == 0)
			{				
				Chain->sequence[Depth].index = -1;
				NewNode->_Add(key, value, Chain, Depth, SplitMode);
			}
			else
			{
				if (NewNode->Keys[0]->CompareTo(key) >= 0)
				{
					Chain->sequence[Depth].index = -1;
					NewNode->_Add(key, value, Chain, Depth, SplitMode);
				}
				else
				{
					Chain->sequence[Depth].index = -1;
					_Add(key, value, Chain, Depth, SplitMode);
				}
			}

			//Update the parent node
			std::streamoff ParentPos = Chain->sequence[Depth - 1].location;
			BTreeNode* Parent = new BTreeNode(Base);
			Parent->Load(datastore,ParentPos);
			
			//Find insertion point...						
			Parent->_Add(NewNode->Keys[0]->Key(), &NewNode->position, Chain, Depth - 1, SplitMode);
			NewNode->Write();
			Write();
		}
		else
		{
			//Splitting the root node is more complicated.

			//Create a new, split node
			std::streamoff newnode = _Split(SplitMode);
			BTreeNode* NewNode = new BTreeNode(Base);
			NewNode->Load(datastore, newnode);

			//Create a new node for THIS node.
			BTreeNode* Branch = new BTreeNode(Base);
			Branch->Create(datastore, false);	

			//Change the file storage positions around
			std::streamoff currentpos = position;
			this->position = Branch->position;
			Branch->position = currentpos;
			this->Next = NewNode->position;
			Write();

			//Create new parent records...
			currentpos = position;
			void* splitkey = new char[Base->BranchTemplate.KeySize];
			memcpy(splitkey, Keys[0]->Key(), Base->BranchTemplate.KeySize);
			Chain->sequence[Depth].index = -1;
			Branch->_Add(splitkey, &currentpos,Chain,0,0);
			delete splitkey;

			if (NewNode->KeyCount() > 0)
			{
				currentpos = NewNode->position;
				Branch->_Add(NewNode->Keys[0]->Key(), &currentpos, Chain, 0, 0);
			}
			else
			{
				//Chain->index = -1;
				Chain->sequence[Depth].index = -1;
				currentpos = NewNode->position;
				Branch->_Add(key, &currentpos, Chain, Depth, 0);				

				if ((NewNode->KeyCount() == 0) || ((NewNode->Keys[0] != NULL) && (NewNode->Keys[0]->CompareTo(key) >= 0)))
				{
					Chain->sequence[Depth].index = -1;
					NewNode->_Add(key, value, Chain, Depth, SplitMode);
					//NewNode->_Add(key, value, Chain, Depth, SplitMode);
				}
				else
				{
					_Add(key, value, Chain, Depth, SplitMode);
				}
			}
			
			Branch->Write();

			Base->Root->Load(datastore,Base->Root->position);
			int q = 0;
			
		}
	}
	else
	{
		//This should fit
		int index = Chain->sequence[Depth].index;		
		//if (index != -1) index++;
		if ((index == -1) || (index >= _KeyCount))
		{
			if (index == -1) index = _KeyCount;
			if (Keys[index] == NULL)
			{
				if (leaf == false)
					Keys[index] = new BTreeKey(&Base->BranchTemplate);
				else
					Keys[index] = new BTreeKey(&Base->LeafTemplate);
			}
			Keys[index]->Set(key, value);
			_KeyCount++;
		}
		else
		{
			//Nudge Existing Keys Forward...		
			int last = Base->KeysPerNode(leaf) - 1;
			index++;
			if (Keys[last] != NULL)
			{
				delete Keys[last];
			}
			for (int x = last-1; x >= index;x--)
			{				
				Keys[x + 1] = Keys[x];
			}
			
				if (leaf == false)
					Keys[index] = new BTreeKey(&Base->BranchTemplate);
				else
					Keys[index] = new BTreeKey(&Base->LeafTemplate);
			
			Keys[index]->Set(key, value);
			_KeyCount++;
		}
		return Write();
	}
	return true;
}

void BTreeNode::Copy(BTreeNode* N)
{
	leaf = N->leaf;

	if (Keys != NULL)
	{
		for (int x = 0; x < _KeyCount; x++)
		{
			if (Keys[x] != NULL)
				delete Keys[x];
		}
		delete[]Keys;
		Keys = NULL;
	}

	Keys = new BTreeKey * [Base->KeysPerNode(leaf)];	

	BTreeKeyTemplate* Tmpl = &Base->BranchTemplate;
	if (leaf == true) Tmpl = &Base->LeafTemplate;

	for (int x = 0; x < Base->KeysPerNode(leaf); x++)
	{
		Keys[x] = NULL;
	}

	_KeyCount = N->_KeyCount;

	for (int x = 0; x < _KeyCount; x++)
	{
		Keys[x] = NULL;
		if (x < _KeyCount)
		{
			Keys[x] = new BTreeKey(Tmpl);
			Keys[x]->Set(N->Keys[x]->Key(), N->Keys[x]->Payload());
		}
	}	
}

BTreeNode* BTreeNode::FirstLeaf()
{
	if (this->leaf == true)
		return this;

	BTreeNode* Active = this;
	while (Active != NULL)
	{
		if (Active->leaf == true) return Active;

		BTreeNode* NextNode = new BTreeNode(Base);
		NextNode->Load(Base->GetDataStore().get(), *((std::streamoff*)Active->Keys[0]->Payload()));
		if (Active != this)
		{
			delete Active;
		}
		Active = NextNode;
	}
	return NULL;
}

BTreeSearchChain::BTreeSearchChain()
{
	found = false;	
	Key = NULL;
	//index = -1;

}

BTreeSearchChain::~BTreeSearchChain()
{
	/*if (Key != NULL)
	{
		delete Key;
	}*/
}