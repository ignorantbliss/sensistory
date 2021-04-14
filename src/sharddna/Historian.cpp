//#include "pch.h"
#include "Historian.h"
#include "BTree.h"
#include "Cursor.h"

#include <string>
#include <iostream>
#include <fstream>
#include <time.h>

using namespace std;

Historian::Historian()
{	
	Instance = this;
}

Historian *Historian::Instance = NULL;

Historian::~Historian()
{
}

bool Historian::ParseArguments(int args, const char** argv)
{
	std::string BaseDir;
	std::string ConfigDir;

	BaseDir = argv[0];
	ConfigDir = "";
	const char* ps = strrchr(argv[0], '\\');
	if (ps == NULL)
		ps = strrchr(argv[0], '/');

	if (ps != NULL)
	{
		int ln = strlen(argv[0]);
		char* buf = new char[ln];
		memcpy(buf, argv[0], ln);
		buf[(int)(ps - argv[0])] = NULL;
		BaseDir = buf;
		delete[]buf;
	}

	ConfigDir = BaseDir + "\\default.conf";

	for (int x = 1; x < args; x++)
	{
		if (strcmp(argv[x], "--conf"))
		{
			ConfigDir = argv[x + 1];
		}
	}

	Config.Load(ConfigDir.c_str());

	return true;
}

bool Historian::Init()
{
	LogPath = Config.GetValueString("LogFile", "Log/service.log");
	int CurrentLogLevel = Config.GetValueInt("LogLevel", 0xFFFFFF);

	AddLogger(this, CurrentLogLevel);
	Log(LOGLEVEL_INFO, "Historian", "Starting Up");

	HistoryConfig HC;
	HC.StorageFolder = Config.GetValueString("Storage","Data\\");
	return Data.Init(HC);
}

bool Historian::Start()
{	
	return true;
}

bool Historian::LogEvent(const char *ev, const char *ctx, const char *message)
{
	ofstream strm;
	strm.open(LogPath, ofstream::app);
	strm << ev << "\t" << ctx << "\t" << message << endl;
	strm.flush();
	strm.close();

	cout << ev << "\t" << ctx << "\t" << message << endl;

	return true;
}

