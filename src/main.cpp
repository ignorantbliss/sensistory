// Sharding.cpp : Defines the entry point for the application.
//

#include "sharding.h"
#include "core/core.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <boost/log/common.hpp>

using namespace std;

int main(int argc,const char **argv)
{
	ShardingCore Core;
	if (!Core.Init(argc, argv))
	{
		return -1;
	}
	while (true)
	{
		Sleep(1000);
	}
	return 0;
}
