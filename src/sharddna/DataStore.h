#pragma once

#include <fstream>
#include <string>
#include <mutex>

#include "BTreeKey.h"

using namespace std;

class DataStore
{
public:
	DataStore();
	~DataStore();

	bool OpenOrCreate(const char *fn);
	bool IsEmpty();
	bool Init();
	bool Load();

	std::streamoff BlockIndexToOffset(int indx);
	char *ReadBlock(std::streamoff pos);
	bool WriteBlock(std::streamoff pos, const char *buf);
	std::streamoff AllocateBlock();
	char* InitBlock();
	void Mark(char* buf, const char* code, int ln);
	void Close();

	int BlockSize();
	bool BlockAvailable(int i);
	bool PosAvailable(std::streamoff pos);
	char Sequence[4];	
	std::string FileName();

	DataStore(const DataStore&) = default;
	DataStore& operator=(const DataStore&);

protected:
	string filename;
	fstream file{};
	mutex readlock;
	mutex writelock;	
	bool Empty;

	std::streamoff EndPos;
	int GetBytesPerBlock();		

	bool WriteFileHeader();
	bool ReadFileHeader();

	int BytesPerBlock;	
	int TotalBlockSize;
};

#define DStore std::shared_ptr<DataStore>