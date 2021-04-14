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

#pragma once
#include "ConfigFile.h"
#include "LogListener.h"
#include "DataStore.h"
#include "BTree.h"
#include "TSPoint.h"
#include "ShardCursor.h"
#include "Series.h"
#include <string>
#include <vector>
#include <time.h>
using namespace std;

class History;

#define PSC std::shared_ptr<ShardCursor>
#define TIMESTAMP time_t

/**
 * @brief Historian configuration options
 * 
 * This structure is used to pass configuration details about your shard storage, such as the path to store the files and the time between generated shards.
*/
struct HistoryConfig
{	
	std::string StorageFolder; /**< The folder to store all of the database files */	
	int TimePerShard; /**< The amount of time (in seconds, rounded to the nearest minute) between shards */
};

/**
 * @brief Multi-channel query details
 *
 * This class allows you to define a multi-channel query for use with the History::GetHistory function.
*/
class Query
{
public:
	/**
	 * @brief Default Constructor
	 * Constructs a Query object.
	*/
	Query();

	std::vector<std::string> Channels; /**< A vector of each of the channel names to query */
	time_t from;  /**< The start time for the query */
	time_t to;  /**< The end time for the query */
	int MaxSamples;  /**< The total number of samples to return (per-channel) */
	int Options;  /**< Options to the query (currently unused) */
};

/**
 * @brief Multi-channel query details with aggregation
 *
 * This class allows you to perform aggregated queries - breaking the time space into discrete chunks and fetching 
 * aggregated data about those chunks.
*/
class AggregatedQuery : public Query
{
public:
	AggregatedQuery();
	
	int SampleCount;
	enum Aggregation {avg, min, max};
	Aggregation Method;
};

class QueryCursor;

typedef SeriesInfo (*SeriesInfoGetCallback)(const char *, void *);
typedef void (*SeriesInfoSetCallback)(const char *,SeriesInfo, void *);

/**
 * @brief Represents your time-series history
 *
 * This is the core ShardDNA class - it allows you to create historical storage and read and write time-series data.
*/
class History
{
public:
	History();
	~History();
	
	/**
	 * @brief Initialises a History object
	 * @param Cfg Configuration details for this historian
	 * @return Returns true if successful, otherwise false
	*/
	bool Init(HistoryConfig Cfg);	

	/**
	 * @brief Gets a shard that covers the specified time frame
	 * @param tm The time-frame for the shard
	 * @param create If true, the shard is created if it doesn't already exist.
	 * @return A smart-pointer to data store, or 'null' if the call fails.
	*/
	DStore GetShard(TIMESTAMP tm, bool create);

	/**
	 * @brief Gets the NEXT shard based on a time frame
	 * @param tm The time-frame for the shard	 
	 * @return A smart-pointer to data store, or 'null' if the call fails.
	*/
	DStore GetNextShard(TIMESTAMP tm);

	/**
	 * @brief Gets the NEXT shard based on a time frame and a series name
	 * @param tm The time-frame for the shard
	 * @param series The name/code of the time-series 
	 * @return A smart-pointer to data store, or 'null' if the call fails.
	*/
	PCursor GetNextShardWith(TIMESTAMP tm,const char *series);

	/**
	 * @brief Checks if a particular time series exists
	 * @param name The name/code for the time-series
	 * @return Returns true if the series exists, otherwise false
	*/
	bool SeriesExists(const char* name);

	/**
	 * @brief Gets or sets the SeriesInfo for a named series
	 * @param name The name/code of the time-series
	 * @param SI The SeriesInfo to write if the series doesn't exist, or NULL if it should not create the series.
	 * @return 
	*/
	SeriesInfo GetSeries(const char* name, SeriesInfo* SI);

	/**
	 * @brief Record a value at the current time, with no options
	 * @param name The name/code of the time-series
	 * @param Value The value to write
	 * @return Returns true if successful, otherwise false.
	*/
	bool RecordValue(const char* name, double Value);

	/**
	 * @brief Records a value to the Time-Series
	 * @param name The name/code for the time-series
	 * @param Value The value to record
	 * @param Stamp The time-stamp to write to
	 * @param Options Any options regarding this write (currently unused)
	 * @return Returns true if successful, otherwise false
	*/
	bool RecordValue(const char* name, double Value, TIMESTAMP Stamp, int Options = 0);

	/**
	 * @brief Returns the latest value for a given series
	 * Note that this function heavily accesses the hard-drive and is NOT intended for polling or other routine
	 * checking for live data. 
	 * @param name The name/code for the time-series	 
	 * @return Returns true if successful, otherwise false
	*/
	TSPoint GetLatest(const char* name);

	/**
	 * @brief Get history for a single channel
	 * @param name The name/code for the time-series
	 * @param from The date to begin the query
	 * @param to The date to end the query (otherwise 0)
	 * @return returns a ShardCursor to navigate the results
	*/
	ShardCursor* GetHistory(const char* name, TIMESTAMP from, TIMESTAMP to);

	/**
	 * @brief Get the raw history for one or more channels
	 * @param Q The Query object containing query details	 
	 * @return returns a QueryCursor to navigate the results
	*/
	QueryCursor* GetHistory(Query Q);

	/**
	 * @brief Get the aggregated/interpolated history for one or more channels
	 * @param Q The AggregatedQuery object containing query details
	 * @return returns a QueryCursor to navigate the results
	 * @warning Not yet implemented
	*/
	QueryCursor* GetHistory(AggregatedQuery Q);

	/**
	 * @brief Internal function to display all shards on standard output	 
	*/
	void ShardDiagnostics();

	SeriesInfoGetCallback SIGet;
	SeriesInfoSetCallback SISet;
	void* SIContext;

protected:
	std::string StoragePath;
	int TimePerShard;
	vector<DStore> OpenShards;
	DStore GetStore(const char* filename);

private:
	PBTREE Shards;
	PBTREE Series;
	DStore ShardStorage;
	DStore SeriesStorage;
	std::string BaseDir;
	std::string ConfigDir;

};

