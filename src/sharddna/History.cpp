//#include "pch.h"
#include "History.h"
#include "BTree.h"
#include "Cursor.h"
#include "QueryCursor.h"
#include "Series.h"

#include <string>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>
using namespace std;

//#define _CRT_SECURE_NO_WARNINGS

History::History()
{
	TimePerShard = 60 * 60 * 24;	
	SIGet = NULL;
	SISet = NULL;
	SIContext = NULL;
}

History::~History()
{
}

bool History::Init(HistoryConfig HC)
{
	//Get defaults initialise log...
	

	//Get Shard Index File
	StoragePath = HC.StorageFolder;
	if (StoragePath == "")
	{
		StoragePath = BaseDir + "\\Data";
	}
	TimePerShard = HC.TimePerShard;
	ShardStorage.reset(new DataStore());
	if (ShardStorage->OpenOrCreate((StoragePath + "/shard.index").c_str()) == false)
	{
		//Logger::Global.Log(LOGLEVEL_CRITICAL, "Historian", "Shard Storage File Could Not Be Opened");
		return false;
	}

	//Logger::Global.Log(LOGLEVEL_INFO, "Historian", "Loaded Shard Index File");
	Shards = std::shared_ptr<BTree>(new BTree());
	Shards->LeafTemplate.KeySize = sizeof(TIMESTAMP);
	Shards->BranchTemplate.KeySize = sizeof(long long);
	Shards->LeafTemplate.PayloadSize = 12;
	Shards->BranchTemplate.PayloadSize = sizeof(std::streamoff);
	Shards->LeafTemplate.CompareInverse = true;
	Shards->BranchTemplate.CompareInverse = true;
	
	ShardStorage->Init();
	Shards->Load(ShardStorage, ShardStorage->BlockIndexToOffset(1));

	if (SIGet == NULL)
	{
		SeriesStorage.reset(new DataStore());
		if (SeriesStorage->OpenOrCreate((StoragePath + "/series.index").c_str()) == false)
		{
			//Logger::Global.Log(LOGLEVEL_CRITICAL, "Historian", "Shard Storage File Could Not Be Opened");
			return false;
		}

		Series = std::shared_ptr<BTree>(new BTree());
		Series->LeafTemplate.KeySize = 32;
		Series->BranchTemplate.KeySize = 32;
		Series->LeafTemplate.PayloadSize = sizeof(SeriesInfo);
		Series->BranchTemplate.PayloadSize = sizeof(std::streamoff);

		SeriesStorage->Init();
		Series->Load(SeriesStorage, SeriesStorage->BlockIndexToOffset(1));
	}

	return true;
}

bool History::SeriesExists(const char* series)
{
	SeriesInfo SI = GetSeries(series, NULL);
	if (SI.FirstSample > 0)
		return true;
	return false;
}

SeriesInfo History::GetSeries(const char* series,SeriesInfo *SI)
{	
	if (SIGet != NULL)
	{
		SeriesInfo SIX = (*SIGet)(series,SIContext);
		if (SIX.FirstSample == 0)
		{
			if (SI != NULL)
			{
				(*SISet)(series, *SI,SIContext);
				return (*SIGet)(series, SIContext);
			}
		}
		return SIX;
	}
	SeriesInfo S;	
	BTreeKey *Key = Series->PointSearch(series, strlen(series), BTREE_SEARCH_EXACT);
	if (Key != NULL)
	{
		memcpy(&S, Key->Payload(), sizeof(SeriesInfo));
		delete Key;
	}
	else
	{
		if (SI != NULL)
		{
			S.FirstSample = SI->FirstSample;
			S.Options = SI->Options;
			Series->Add(series, strlen(series), SI, sizeof(SeriesInfo));			
		}
	}
	return S;
}

TSPoint History::GetLatest(const char* series)
{
	time_t CurrentTime = time(NULL);
	SeriesInfo SI = GetSeries(series,NULL);
	if (SI.FirstSample == 0)
	{
		return TSPoint::NullValue();
	}

	//DStore Shard = GetShard(CurrentTime);
	DStore ActiveShard = GetShard(CurrentTime,false);
	BTree PointList;
	PointList.LeafTemplate.KeySize = 32;
	PointList.BranchTemplate.KeySize = 32;
	PointList.LeafTemplate.PayloadSize = sizeof(std::streamoff);
	PointList.BranchTemplate.PayloadSize = sizeof(std::streamoff);
	//SetContext("Request");
	ActiveShard->Init();
	if (!PointList.Load(ActiveShard, ActiveShard->BlockIndexToOffset(1)))
	{
		return TSPoint::NullValue();
	}

	BTree Timeseries;
	Timeseries.LeafTemplate.KeySize = sizeof(TIMESTAMP);
	Timeseries.BranchTemplate.KeySize = sizeof(TIMESTAMP);
	Timeseries.LeafTemplate.PayloadSize = sizeof(double);
	Timeseries.BranchTemplate.PayloadSize = sizeof(std::streamoff);
	Timeseries.LeafTemplate.CompareInverse = true;
	Timeseries.BranchTemplate.CompareInverse = true;
	std::streamoff offset = 0;

	BTreeKey* Key = PointList.PointSearch(series, strlen(series), BTREE_SEARCH_EXACT);
	if (Key == NULL)
	{
		return TSPoint::NullValue();;
	}
	else
	{
		memcpy(&offset, Key->Payload(), sizeof(offset));
		delete Key;
		if (!Timeseries.Load(ActiveShard, offset))
		{
			return TSPoint::NullValue();
		}
	}

	Key = Timeseries.PointSearch(&CurrentTime, sizeof(CurrentTime), BTREE_SEARCH_PREVIOUS);
	if (Key == NULL)
	{
		return TSPoint::NullValue();
	}

	TSPoint P(Key);
	delete Key;
	return P;
}

DStore History::GetStore(const char* storename)
{
	std::string store = storename;
	for (unsigned int x = 0; x < OpenShards.size(); x++)
	{
		if (OpenShards[x]->FileName() == store)
		{
			return OpenShards[x];
		}
	}

	DataStore* ActiveShard = new DataStore();
	ActiveShard->OpenOrCreate(storename);
	ActiveShard->Init();
	std::shared_ptr<DataStore> Store(ActiveShard);
	OpenShards.push_back(Store);
	return Store;
}

ShardCursor* History::GetHistory(const char* series, TIMESTAMP from, TIMESTAMP to)
{	
	SeriesInfo SI = GetSeries(series,NULL);
	if (SI.FirstSample == 0)
	{
		return NULL;
	}
	if (from < SI.FirstSample)
	{
		from = SI.FirstSample;
	}

	TIMESTAMP CurrentTime = from;
	DStore ActiveShard = GetShard(CurrentTime,false);
	PBTREE PointList = std::shared_ptr<BTree>(new BTree());
	PointList->LeafTemplate.KeySize = 32;
	PointList->BranchTemplate.KeySize = 32;
	PointList->LeafTemplate.PayloadSize = sizeof(std::streamoff);
	PointList->BranchTemplate.PayloadSize = sizeof(std::streamoff);	
	ActiveShard->Init();
	if (!PointList->Load(ActiveShard, ActiveShard->BlockIndexToOffset(1)))
	{
		return NULL;
	}

	std::shared_ptr<BTree> Timeseries(new BTree());
	Timeseries->LeafTemplate.KeySize = sizeof(TIMESTAMP);
	Timeseries->BranchTemplate.KeySize = sizeof(TIMESTAMP);
	Timeseries->LeafTemplate.PayloadSize = sizeof(double);
	Timeseries->BranchTemplate.PayloadSize = sizeof(std::streamoff);
	Timeseries->LeafTemplate.CompareInverse = true;
	Timeseries->BranchTemplate.CompareInverse = true;
	std::streamoff offset = 0;

	BTreeKey* Key = PointList->PointSearch(series, strlen(series), BTREE_SEARCH_EXACT);
	if (Key == NULL)
	{
		return NULL;
	}
	else
	{
		memcpy(&offset, Key->Payload(), sizeof(offset));
		delete Key;
		if (!Timeseries->Load(ActiveShard, offset))
		{
			return NULL;
		}
	}

	PCursor C;
	C = Timeseries->Search(&CurrentTime, sizeof(CurrentTime), BTREE_SEARCH_PREVIOUS);
	C->SetTree(Timeseries);		
	ShardCursor* SC = new ShardCursor(this, C);	
	SC->SetSeries(series);
	return SC;
}

QueryCursor* History::GetHistory(Query Q)
{
	QueryCursor* Cursor = new QueryCursor();
	for (unsigned int x = 0; x < Q.Channels.size(); x++)
	{
		std::shared_ptr<ShardCursor> Csr = std::shared_ptr<ShardCursor>(GetHistory(Q.Channels[x].c_str(), Q.from, Q.to));		
		Cursor->AddChannel(Csr);
	}
	return Cursor;
}

QueryCursor* History::GetHistory(AggregatedQuery Q)
{
	return NULL;
}

bool History::RecordValue(const char* series, double Value, TIMESTAMP CurrentTime, int Options)
{
	//Step 1 - Identify Shard...

	DStore ActiveShard = GetShard(CurrentTime,true);

	SeriesInfo Original;
	Original.FirstSample = CurrentTime;
	Original.Options = 0;

	SeriesInfo SI = GetSeries(series, &Original);
	if (SI.FirstSample == 0)
	{
		return NULL;
	}	

	PBTREE PointList = std::shared_ptr<BTree>(new BTree);
	PointList->LeafTemplate.KeySize = 32;
	PointList->BranchTemplate.KeySize = 32;
	PointList->LeafTemplate.PayloadSize = sizeof(std::streamoff);
	PointList->BranchTemplate.PayloadSize = sizeof(std::streamoff);
	//SetContext("Request");
	ActiveShard->Init();
	if (!PointList->Load(ActiveShard, ActiveShard->BlockIndexToOffset(1)))
	{
		return false;
	}	
	
	PBTREE Timeseries = std::shared_ptr<BTree>(new BTree);
	Timeseries->LeafTemplate.KeySize = sizeof(TIMESTAMP);
	Timeseries->BranchTemplate.KeySize = sizeof(TIMESTAMP);
	Timeseries->LeafTemplate.PayloadSize = sizeof(double);
	Timeseries->BranchTemplate.PayloadSize = sizeof(std::streamoff);
	Timeseries->SplitMode = BTREE_SPLITMODE_END;
	Timeseries->LeafTemplate.CompareInverse = true;
	Timeseries->BranchTemplate.CompareInverse = true;
	std::streamoff offset = 0;

	BTreeKey* Key = PointList->PointSearch(series, strlen(series), BTREE_SEARCH_EXACT);
	if (Key == NULL)
	{
		//cout << "USING NEW CHANNEL" << endl;
		offset = Timeseries->Create(ActiveShard);
		PointList->Add(series, strlen(series), &offset, sizeof(offset));
	}
	else
	{
		//cout << "USING EXISTING CHANNEL" << endl;
		memcpy(&offset, Key->Payload(), sizeof(offset));
		if (!Timeseries->Load(ActiveShard, offset))
		{
			return false;
		}
	}

	//cout << "ADDING TO TIME SERIES" << endl;
	Timeseries->Add((const char*)&CurrentTime, sizeof(TIMESTAMP), (const char*)&Value, sizeof(Value));


	return false;
}

bool History::RecordValue(const char* series, double Value)
{
	//Step 1 - Identify Shard...
	TIMESTAMP CurrentTime = time(NULL);

	return RecordValue(series, Value, CurrentTime);
}

std::string ShardNameFromDate(TIMESTAMP tmx)
{
	char buf[13];
	tm* t = localtime(&tmx);
	std::strftime(buf, 13, "%Y%m%d%H%M",t);
	return buf;
}

std::shared_ptr<DataStore> History::GetShard(TIMESTAMP date,bool create)
{
	//char shardname[32];
	char shardname[13];
	memset(shardname, 0, 13);

	//cout << "Requesting Shard for " << date << endl;

	TIMESTAMP CurrentTime = date;
	BTreeKey* Key = Shards->PointSearch((const char*)&CurrentTime, sizeof(TIMESTAMP), BTREE_SEARCH_PREVIOUS);
	if (Key == NULL)
	{
		std::string SN = ShardNameFromDate(CurrentTime);
		Shards->Add((const char*)&CurrentTime, sizeof(TIMESTAMP), SN.c_str(), SN.size());
		memcpy(shardname, SN.c_str(), SN.size());
	}
	else
	{
		TIMESTAMP basis = *(TIMESTAMP*)Key->Key();
		memcpy(shardname, Key->Payload(), 12);
		//cout << "Closest Shard is " << shardname << " for "<< basis << endl;
		if (create == true)
		{
			if (basis > CurrentTime)
			{				
				std::string SN = ShardNameFromDate(CurrentTime);
				Shards->Add((const char*)&CurrentTime, sizeof(TIMESTAMP), SN.c_str(), SN.size());
				memcpy(shardname, SN.c_str(), SN.size());				
			}
			else
			{
				if ((CurrentTime - basis) > TimePerShard)
				{					
					int md = ((CurrentTime - basis) % TimePerShard);
					CurrentTime -= md;
					//cout << "New Shard Required - Corrected @ " << CurrentTime << endl;
					std::string SN = ShardNameFromDate(CurrentTime);
					if (Shards->Add((const char*)&CurrentTime, sizeof(TIMESTAMP), SN.c_str(), SN.size()))
					{
						memcpy(shardname, SN.c_str(), SN.size());
					}
					else
					{
						cout << "FAILURE: Could not add new shard record" << endl;
						return std::shared_ptr<DataStore>(NULL);
					}
				}				
			}
		}
	}

	DataStore* ActiveShard = new DataStore();
	string Shd = StoragePath + "\\";
	Shd += shardname;
	Shd += ".shard";

	
	//ActiveShard->OpenOrCreate(Shd.c_str());	
	std::shared_ptr<DataStore> Store = GetStore(Shd.c_str());
	return Store;
}

std::shared_ptr<DataStore> History::GetNextShard(TIMESTAMP date)
{
	char shardname[32];

	TIMESTAMP CurrentTime = time(NULL);
	BTreeKey* Key = Shards->PointSearch((const char*)&CurrentTime, sizeof(TIMESTAMP), BTREE_SEARCH_NEXT);
	if (Key == NULL)
	{
		return NULL;
	}
	else
	{
		//Get Shard Name		
		//cout << "USING EXISTING SHARD" << endl;
		memcpy(shardname, Key->Payload(), 32);
	}

	DataStore* ActiveShard = new DataStore();
	string Shd = StoragePath + "\\";
	Shd += shardname;
	Shd += ".shard";

	std::shared_ptr<DataStore> Store = GetStore(Shd.c_str());

	return Store;
}

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

PCursor History::GetNextShardWith(TIMESTAMP CurrentTime, const char *series)
{	
	char shardname[13];
	memset(shardname, 0, 13);
	
	while (true)
	{
		BTreeKey* Key = Shards->PointSearch((const char*)&CurrentTime, sizeof(TIMESTAMP), BTREE_SEARCH_NEXT);
		if (Key == NULL)
		{
			//cout << "No Remaining Shards" << endl;
			return NULL;
		}
		else
		{			
			memcpy(shardname, Key->Payload(), 12);			
			CurrentTime = *(time_t *)Key->Key();
			
			//cout << "Checking Shard " << shardname << " with Timestamp " <<CurrentTime<<endl;
			
			std::string ShardFile = StoragePath + "\\";
			ShardFile += shardname;
			ShardFile += ".shard";
			DStore DS = GetStore(ShardFile.c_str());
			
			PBTREE PointList = std::shared_ptr<BTree>(new BTree());
			PointList->LeafTemplate.KeySize = 32;
			PointList->BranchTemplate.KeySize = 32;
			PointList->LeafTemplate.PayloadSize = sizeof(std::streamoff);
			PointList->BranchTemplate.PayloadSize = sizeof(std::streamoff);
			//DS->OpenOrCreate(ShardFile.c_str());
			//DS->Init();
			if (!PointList->Load(DS, DS->BlockIndexToOffset(1)))
			{
				return NULL;
			}

			//Find this series in the point list
			BTreeKey* Key = PointList->PointSearch(series, strlen(series), BTREE_SEARCH_EXACT);
			if (Key != NULL)
			{
				//Found the series in the shard! Let's go!
				DataStore* ActiveShard = new DataStore();
				string Shd = StoragePath + "\\";
				Shd += shardname;
				Shd += ".shard";

				std::shared_ptr<DataStore> Store = GetStore(Shd.c_str());

				PBTREE Timeseries(new BTree());
				Timeseries->LeafTemplate.KeySize = sizeof(time_t);
				Timeseries->BranchTemplate.KeySize = sizeof(time_t);
				Timeseries->LeafTemplate.PayloadSize = sizeof(double);
				Timeseries->BranchTemplate.PayloadSize = sizeof(std::streamoff);
				Timeseries->SplitMode = BTREE_SPLITMODE_END;				
				Timeseries->LeafTemplate.CompareInverse = true;
				Timeseries->BranchTemplate.CompareInverse = true;
				std::streamoff offset = 0;
				memcpy(&offset, Key->Payload(), sizeof(offset));
				if (!Timeseries->Load(Store, offset))
				{
					return NULL;
				}
				PCursor Curse = Timeseries->FirstNode();										
				return Curse;
			}			
			else
			{
				//Series isn't in this shard - let's continue.
				cout << "Series Not Present In Shard" << endl;
				CurrentTime++;
			}
		}
	}

	return NULL;
}

void History::ShardDiagnostics()
{
	PCursor C = Shards->FirstNode();
	char buf[13];
	memset(buf, 0, 13);
	
	while (C->Next() >= 0)
	{
		memcpy(buf, C->Value(), 12);
		time_t T = (time_t)C->Key();

		cout << "Shard " << buf << " @ " <<T << endl;
	}
}

Query::Query()
{
	this->MaxSamples = 0;
	this->Options = 0;
	this->from = 0;
	this->to = 0;
}

AggregatedQuery::AggregatedQuery()
{
	Query::Query();
	this->Method = Aggregation::avg;
	this->SampleCount = 100;
}