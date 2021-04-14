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

#include "BTreeKey.h"
#include "BTreeNode.h"
#include "ShardDNA.h"
#include <memory>
#include <fstream>

class BTreeCursor
{
public:
	virtual bool Next() = 0;
	virtual const void* Key() = 0;
	virtual const void* Value() = 0;

	virtual void End() = 0;
};

using namespace std;

#define PCursor std::shared_ptr<Cursor>

#define CURSOR_PRE	1
#define CURSOR_OK	0
#define CURSOR_POST	2
#define CURSOR_END	-1
#define CURSOR_CHANGING	3

class History;

class Cursor
{
public:
	Cursor();
	~Cursor();

	void SetPosition(std::streamoff Position, int Index);	
	void SetTree(std::shared_ptr<BTree> tr);
	void SetEndpoint(TIMESTAMP t);	
	//void SetParent(History* H);
	//void SetSeries(std::string S);

	int Next();
	const void *Key();
	const void *Value();	
	void Delete(bool WriteNow = true);
	//int NextStepCost();

	void End();

	friend class ShardCursor;

protected:	
	std::streamoff Offset;		
	BTreeNode* Active;		
	int Index;
	TIMESTAMP EndPoint;

	bool EoF;
	std::shared_ptr<BTree> Tree;
	History* Hist;
	std::string Series;
};

