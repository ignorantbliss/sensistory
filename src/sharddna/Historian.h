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
#include "ConfigFile.h"
#include "LogListener.h"
#include "DataStore.h"
#include "BTree.h"
//#include "Query.h"
#include "TSPoint.h"
#include "History.h"
#include <string>
#include <vector>
#include <time.h>
using namespace std;

class Historian : public Logger, public LogListener
{
public:
	Historian();
	~Historian();


	bool Init();
	bool Start();
	bool ParseArguments(int argv, const char** args);

	ConfigFile Config;
	
	bool LogEvent(const char *typ, const char *ctx, const char *message);
	static Historian* Instance;

	History Data;
private:
	std::string LogPath;
};

