#pragma once
#include <string>
#include <vector>
#include "BTree.h"
#include "DataStore.h"

#define TIMESTAMP time_t

using namespace std;

#pragma pack(push, 1)
/**
 * @brief Contains time-series metadata
*/
class SeriesInfo
{
public:
	/**
	 * @brief Constructs a SeriesInfo object
	*/
	SeriesInfo();
	
	int Options; /**< Series options (stepping, etc.)*/
	TIMESTAMP FirstSample;  /**< The first TIMESTAMP for this series */
};
#pragma pack(pop)