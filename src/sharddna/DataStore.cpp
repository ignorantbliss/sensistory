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

#ifdef _WIN32
#include <Windows.h>
#endif

#include "DataStore.h"
#include "Historian.h"
#include <filesystem>
#include <fstream>



using namespace std;

DataStore::DataStore()
{
	BytesPerBlock = 512;
	TotalBlockSize = 512;
	Sequence[0] = 'B';
	Sequence[1] = 'T';
	Sequence[2] = 'R';
	Sequence[3] = 'E';
	EndPos = 0;
	Empty = false;
}

DataStore& DataStore::operator=(const DataStore& dsr)
{	
	BytesPerBlock = dsr.BytesPerBlock;
	EndPos = dsr.EndPos;
	filename = dsr.filename;
	TotalBlockSize = dsr.TotalBlockSize;
	file.open(dsr.filename, fstream::in | fstream::out | fstream::binary | fstream::trunc);
	return *this;
}

DataStore::~DataStore()
{
}

std::string ConvertPaths(const char *fn)
{	
	int ln = strlen(fn);
	char* buf = new char[ln+1];
	buf[ln] = NULL;

	for (int x = 0; x < ln; x++)
	{
#ifdef WIN32
		if (fn[x] == '/')
			buf[x] = '\\';
		else
			buf[x] = fn[x];
#else
		if (fn[x] == '\\')
			buf[x] = '/';
		else
			buf[x] = fn[x];
#endif
	}

	string st = buf;
	delete []buf;
	return st;
}

bool DataStore::OpenOrCreate(const char *fn)
{
	Empty = false;
	filename = ConvertPaths(fn);
	std::string st = "Opening Data Store: ";
	st += fn;
	//Historian::Instance->Log(LOGLEVEL_INFO, "DataStore",st.c_str());
	file.open(filename.c_str(),fstream::in | fstream::out | fstream::binary);
	if (file.is_open() == false)
	{
		file.open(fn, fstream::in | fstream::out | fstream::binary | fstream::trunc);
		Empty = true;
	}
	if (file.is_open() == false)
	{
		return false;
	}	
	return true;
}

//Returns TRUE if the datastore is completely empty.
bool DataStore::IsEmpty()
{		
	return Empty;
}

void DataStore::Close()
{
	file.flush();
	file.close();
}

//Initialises a data store.
bool DataStore::Init()
{
	file.flush();
	if (ReadFileHeader())
	{
		return Load();
	}
	file.clear();

	//Get total number of possible bytes per block...
	BytesPerBlock = GetBytesPerBlock();
	string Out = "Bytes Per Block: ";
	Out += BytesPerBlock;

	//Historian::Instance->Log(LOGLEVEL_INFO, "Datastore Init", Out.c_str());
	

	//Calculate total number of keys given the stated key size...
	
	
	std::string msg = "File ";
	msg += filename;
	//msg += " Maximum Key Size = ";
	//msg += MaxKeys;
	//Historian::Instance->Log(LOGLEVEL_INFO,"Datastore Init",msg.c_str());

	//cout << "File " << this->File->filename.c_str() << " Maximum Key Size = " << MaxKeys << " over " << Blocks << " Disk Blocks" << endl;	

	WriteFileHeader();
	file.flush();

	return true;
}

bool DataStore::Load()
{
	//Historian::Instance->Log(LOGLEVEL_INFO, "Datastore Init", "Loading Store Details");
	bool r = ReadFileHeader();	
	std::string msg = "File ";
	msg += filename;
	msg += " Block Size = ";
	msg += (int)BytesPerBlock;
	//Historian::Instance->Log(LOGLEVEL_INFO, "Datastore Load", msg.c_str());

	file.seekg(0, ios::end);
	std::streamoff pos = file.tellg();
	EndPos = pos;

	return r;
}

char *DataStore::ReadBlock(std::streamoff pos)
{
	if (BytesPerBlock == 0) BytesPerBlock = 512;
	char *buf = new char[BytesPerBlock];
	readlock.lock();
	file.seekg(pos, ios::beg);
	file.read(buf, BytesPerBlock);	
	readlock.unlock();
	if (file.gcount() != BytesPerBlock)
	{
		file.clear();
		delete buf;
		return NULL;
	}
	return buf;
}

bool DataStore::WriteBlock(std::streamoff pos, const char *buf)
{
	writelock.lock();
	file.seekp(pos, ios::beg);
	file.write(buf, BytesPerBlock);
	file.flush();
	writelock.unlock();
	if (file.bad() || file.fail()) return false;
	return true;
}

void DataStore::Mark(char* buf, const char *code, int ln)
{	
	/*for (int x = 0; x < ln; x++)
	{
		buf[(int)BytesPerBlock - ln + x] = code[x];
	}*/
}

char *DataStore::InitBlock()
{
	char *buf = new char[BytesPerBlock];
	memset(buf, 0, BytesPerBlock);
	return buf;
}


bool DataStore::ReadFileHeader()
{
	char *buf = ReadBlock(0);
	if (buf == NULL) return false;
	
	int offset = 0;
	memcpy(&BytesPerBlock,buf, sizeof(BytesPerBlock));
	offset += sizeof(BytesPerBlock);	
	if (memcmp(Sequence, buf + offset, 4) != 0)
	{
		delete buf;
		return false;
	}

	delete buf;
	return true;
}

bool DataStore::WriteFileHeader()
{
	char *buf = InitBlock();
	int offset = 0;

	memcpy(buf, &BytesPerBlock, sizeof(BytesPerBlock));
	offset += sizeof(BytesPerBlock);
	memcpy(buf + offset, Sequence, 4);

	Mark(buf, "DSHDR", 5);
	bool ret = WriteBlock(0, buf);
	delete buf;
	return ret;
}

std::streamoff DataStore::AllocateBlock()
{
	char *buf = new char[BytesPerBlock];
	memset(buf, 0, BytesPerBlock);
	writelock.lock();
	file.seekp(0, fstream::end);
	std::streamoff pos = file.tellp();
	file.write(buf, BytesPerBlock);
	writelock.unlock();
	EndPos = pos + BytesPerBlock;
	return pos;
}

std::streamoff DataStore::BlockIndexToOffset(int indx)
{
	return (std::streamoff)indx * (std::streamoff)BytesPerBlock;
}

int DataStore::GetBytesPerBlock()
{
#ifdef _WIN32
	char target[4];
	target[0] = filename[0];
	target[1] = filename[1];
	target[2] = filename[2];
	target[3] = NULL;

	DWORD Sectors, Bytes, FreeClusters, ClusterCount;

	GetDiskFreeSpace(target, &Sectors, &Bytes, &FreeClusters, &ClusterCount);

	return Bytes;
#endif
#ifdef __linux__
	int blocksize = 0;
	ioctl(fd, BLKPBSZGET, &block_size);
	return blocksize;
#endif
}

int DataStore::BlockSize()
{
	return BytesPerBlock;
}

bool DataStore::BlockAvailable(int i)
{
	std::streamoff ps = (std::streamoff)i * (std::streamoff)BytesPerBlock;
	return PosAvailable(ps);
}

bool DataStore::PosAvailable(std::streamoff ps)
{
	if (ps >= EndPos) return false;
	return true;
}

std::string DataStore::FileName()
{
	return filename;
}