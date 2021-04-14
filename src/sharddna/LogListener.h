#pragma once

#include <vector>
#include <string>
using namespace std;


#define LOGLEVEL_INFO		8
#define LOGLEVEL_CRITICAL	1


class LogListener
{
public:
	LogListener();
	~LogListener();

	virtual bool LogEvent(const char *loglevel, const char *context, const char *content);
};


class Logger
{
public:
	Logger();
	~Logger();

	static Logger Global;

	struct LogQueueItem
	{
		LogListener *Listener;
		int Level;
	};

	void AddLogger(LogListener *listener, int level);
	void RemoveLogger(LogListener *listener);
	void Log(int loglevel, const char *context, const char *lg);
	std::string TranslateLogLevel(int loglevel);
	void SetContext(const char* buf);

protected:
	vector<LogQueueItem> Queue;
	std::string ActiveContext;
};
