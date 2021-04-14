#pragma once

#include <map>
#include <string>

using namespace std;

class ConfigFile
{
public:
	ConfigFile();
	~ConfigFile();

	std::string GetValueString(const char *key, const char *def);
	int GetValueInt(const char *key, int def);
	double GetValueFloat(const char *key, double def);

	bool Load(const char *filename);

protected:
	std::map<std::string, std::string> Values;
};

