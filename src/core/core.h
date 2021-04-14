#pragma once

#include "sqlite3.h"
#include "../queryservice/httpserver.hpp"
#include "../queryservice/ingestserver.h"
#include "../sharddna/History.h"
#include "../sharddna/ConfigFile.h"

#include <string>
#include <vector>
#include <time.h>
#include <map>

#define TIMESTAMP time_t

class DataResponse
{
public:
	DataResponse(std::string Err, int ErrNo);
	std::string Response;
	int ErrorCode;
};

class WriteDataPoint
{
public:
	WriteDataPoint(std::string name, double val, TIMESTAMP moment);

	std::string Point;
	double Value;
	TIMESTAMP Stamp;
};

struct WriteData
{
	std::vector<WriteDataPoint> Points;
};

#define CHANNEL_STEPPED 1

struct ChannelInfo
{
	std::string Name;
	int Options;
	std::string Units;	
	float MinValue;
	float MaxValue;
	char code[33];
	TIMESTAMP FirstRecord;
	double LastValue;
	TIMESTAMP LastCompressed;
};

class ShardingCore
{
public:
	ShardingCore();
	~ShardingCore();

	bool Init(int argcount, const char** arguments);
	
	bool Close();

	DataResponse ReadDataHTTP(std::string req);
	DataResponse WriteDataHTTP(std::string req);
	DataResponse RegisterChannel(ChannelInfo *CI);

	bool Write(WriteData WD);

	SeriesInfo GetChannelInfo(const char* name);
	void SetChannelInfo(const char* name, SeriesInfo SI);
	
protected:
	bool Initialised;
	sqlite3 *dbhandle;
	bool SeriesForName(std::string Name, char **Result);
	std::unordered_map<std::string, int> ChannelNames;
	std::unordered_map<std::string, int> ChannelCodes;
	std::vector<ChannelInfo> Channels;
	History Hist;
	ConfigFile cfg;

private:
	void InitLogging();
	bool InitDB();
	bool InitHistorian();
	bool InitREST();
	void InitConfig();

	bool AutoRegister;
	bool Compression;

	std::vector<std::thread> threads;
};