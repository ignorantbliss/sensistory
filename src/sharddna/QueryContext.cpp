#include "pch.h"
#include "QueryContext.h"

#include <iostream>
using namespace std;

QueryContext::QueryContext()
{
}


QueryContext::~QueryContext()
{
}

void QueryContext::Log(const char *txt)
{
	cout << txt << endl;
}