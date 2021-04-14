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
#include "LogListener.h"
#include <iostream>

using namespace std;

LogListener::LogListener()
{
}


LogListener::~LogListener()
{
}

bool LogListener::LogEvent(const char *loglevel, const char *context, const char *content)
{
	return false;
}

Logger::Logger()
{
	ActiveContext = "init";
}

Logger::~Logger()
{

}

std::string Logger::TranslateLogLevel(int loglevel)
{
	if (loglevel == 8)
		return "INFO";
	return "UNKNWN";
}

Logger Logger::Global;

void Logger::SetContext(const char* buf)
{
	ActiveContext = buf;
}

void Logger::Log(int loglevel, const char *context, const char *content)
{
	if (Queue.size() == 0)
	{
		cout << TranslateLogLevel(loglevel).c_str() << "\t" << ActiveContext<<"."<<context << "\t" << content << endl;
		return;
	}
	for (int x = Queue.size() - 1; x >= 0; x--)
	{
		if (loglevel & Queue[x].Level)
		{			
			Queue[x].Listener->LogEvent(TranslateLogLevel(loglevel).c_str(), (ActiveContext + "." + context).c_str(), content);
		}
	}
}

void Logger::AddLogger(LogListener *listener, int level)
{
	Logger::LogQueueItem Itm;
	Itm.Listener = listener;
	Itm.Level = level;
	Queue.push_back(Itm);
}

void Logger::RemoveLogger(LogListener *listener)
{
	for (int x = Queue.size() - 1; x >= 0; x--)
	{
		if (Queue[x].Listener == listener)
		{
			Queue.erase(Queue.begin() + x);
		}
	}
}