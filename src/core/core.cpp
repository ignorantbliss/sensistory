#include <boost/program_options.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <string>
#include "core.h"
#include "../sharddna/QueryCursor.h"

#ifdef _WIN32
#define PATHSEP '\\';
#define PATHSEP_STR "\\";
#else
#define PATHSEP '/';
#define PATHSEP_STR "/";
#endif

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace po = boost::program_options;
using namespace std;

ShardingCore::ShardingCore()
{
	dbhandle = 0;
	Initialised = false;
	AutoRegister = false;
	Compression = false;
}

ShardingCore::~ShardingCore()
{
	if (Initialised == true)
	{
		Close();
	}
	if (dbhandle != 0)
	{
		sqlite3_close(dbhandle);
	}
}

bool ShardingCore::Init(int argcount, const char** arguments)
{
	BOOST_LOG_TRIVIAL(info) << "Starting Sharding Core";
	Initialised = true;

	po::options_description desc("Start Sharding Service");
	desc.add_options()
		("help", "show help")
		("log,l", po::value<string>(), "log file location")
		("store,s", po::value<string>(), "storage directory")
		("conf,c", po::value<string>()->default_value("sharding.conf"), "configuration file");

	po::variables_map vm;
	po::store(po::parse_command_line(argcount, arguments, desc), vm);
	po::notify(vm);
	
	std::string ConfigFile = vm["conf"].as<string>();
	BOOST_LOG_TRIVIAL(info) << "Opening Config File @ " << ConfigFile;
	cfg.Load(vm["conf"].as<string>().c_str());

	InitLogging();
	InitConfig();
	InitDB();
	InitHistorian();
	InitREST();

	
	BOOST_LOG_TRIVIAL(info) << "Closing REST Endpoint";

	// Block until all the threads exit
	for (auto& t : threads)
		t.join();

	BOOST_LOG_TRIVIAL(info) << "Closing SQLite Database";
	sqlite3_close(dbhandle);
	dbhandle = 0;

	return true;
}

void ShardingCore::InitConfig()
{
	int arenabled = cfg.GetValueInt("AUTOCREATE:enabled", 0);
	if (arenabled == 1)
	{
		AutoRegister = true;
	}
	int arenabled = cfg.GetValueInt("STORAGE:compression", 1);
	if (arenabled == 1)
	{
		Compression = true;
	}
}

SeriesInfo GetChannelInfo(const char* seriesname,void *context)
{
	return ((ShardingCore*)context)->GetChannelInfo(seriesname);
}

void SetChannelInfo(const char* seriesname, SeriesInfo SI, void *context)
{
	return ((ShardingCore*)context)->SetChannelInfo(seriesname, SI);
}

void ShardingCore::SetChannelInfo(const char* seriesname, SeriesInfo SI)
{
	if (ChannelCodes.find(seriesname) != this->ChannelCodes.end())
	{
		Channels[ChannelCodes[seriesname]].FirstRecord = SI.FirstSample;		

		sqlite3_stmt* statement;
		std::string sql = "UPDATE series SET firstrecord=";
		char buf[32];
		//sprintf(buf, "%lf", SI.FirstSample);
		sql += to_string(SI.FirstSample);
		sql += " WHERE code='";
		sql += seriesname;
		sql += "'";
		char* errbuf = NULL;
		if (sqlite3_exec(dbhandle, sql.c_str(), NULL, NULL, &errbuf) != SQLITE_OK)
		{
			BOOST_LOG_TRIVIAL(error) << "Failed to create series table - " << errbuf;
			sqlite3_free(errbuf);
			return;
		}
	}	
}

SeriesInfo ShardingCore::GetChannelInfo(const char* seriesname)
{
	SeriesInfo Blank;
	Blank.FirstSample = 0;
	Blank.Options = 0;

	if (this->ChannelCodes.find(seriesname) != this->ChannelCodes.end())
	{
		Blank.FirstSample = Channels[ChannelCodes[seriesname]].FirstRecord;
		Blank.Options = Channels[ChannelCodes[seriesname]].Options;
	}

	return Blank;
}

bool ShardingCore::InitHistorian()
{
	HistoryConfig HC;
	HC.StorageFolder = cfg.GetValueString("STORAGE:path","data");
	HC.TimePerShard = cfg.GetValueInt("STORAGE:shardtime", 60 * 60 * 24);

	BOOST_LOG_TRIVIAL(info) << "Opening Historian @ " <<HC.StorageFolder.c_str() << " with " << HC.TimePerShard << " second shards";

	Hist.SIGet = &::GetChannelInfo;
	Hist.SISet = &::SetChannelInfo;
	Hist.SIContext = (void*)this;
	Hist.Init(HC);
	return true;
}

bool ShardingCore::InitDB()
{	
	std::string dbfile = cfg.GetValueString("STORAGE:path", "");
	if (dbfile != "")
		dbfile += PATHSEP_STR;
	dbfile += "lookup.db";

	BOOST_LOG_TRIVIAL(info) << "Connecting to SQLite DB @ " <<dbfile;
	sqlite3_open_v2(dbfile.c_str(), &dbhandle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	sqlite3_stmt* statement;
	std::string sql = "CREATE TABLE IF NOT EXISTS series (name VARCHAR(120), code VARCHAR(32), min FLOAT, max FLOAT, options INT, units VARCHAR(10),firstrecord INT8, lastrecord INT8)";
	char* errbuf = NULL;
	if (sqlite3_exec(dbhandle, sql.c_str(), NULL, NULL, &errbuf) != SQLITE_OK)
	{
		if (errbuf == NULL)
			BOOST_LOG_TRIVIAL(error) << "Failed to create series table (Undefined Error)";
		else
		{
			BOOST_LOG_TRIVIAL(error) << "Failed to create series table - " << errbuf;
			sqlite3_free(errbuf);
		}
		return false;
	}
	
	//Load series details...	
	//sqlite3_stmt* statement;
	std::string qry = "SELECT name,code,min,max,units,options,firstrecord,lastrecord FROM series";
	if (sqlite3_prepare_v2(dbhandle, qry.c_str(), qry.size(), &statement, NULL) == SQLITE_OK)
	{
		int rw = sqlite3_step(statement);
		while ((rw == SQLITE_ROW) || (rw == SQLITE_OK))
		{
			ChannelInfo CI;
			CI.Name = (const char*)sqlite3_column_text(statement, 0);
			const char* buf = (const char*)sqlite3_column_text(statement, 1);
			memcpy(CI.code, buf, 32);
			CI.code[32] = 0;
			CI.Units = (const char*)sqlite3_column_text(statement, 4);
			CI.MinValue = sqlite3_column_double(statement, 2);
			CI.MaxValue = sqlite3_column_double(statement, 3);
			CI.Options = sqlite3_column_int(statement, 5);
			CI.FirstRecord = sqlite3_column_int(statement, 6);
			Channels.push_back(CI);
			int ps = Channels.size() - 1;
			ChannelNames[CI.Name] = ps;
			ChannelCodes[CI.code] = ps;

			rw = sqlite3_step(statement);
		}
	}
	else
	{
		cout << "DB Error: " << sqlite3_errmsg(dbhandle) << endl;
	}
	sqlite3_finalize(statement);	
}

bool ShardingCore::Close()
{	
	BOOST_LOG_TRIVIAL(info) << "Shutting Down Sharding Core";
	Initialised = false;

	//Ingestion.Stop();

	return true;
}

void ShardingCore::InitLogging()
{
	logging::register_simple_formatter_factory<logging::trivial::severity_level, char>("Severity");

	std::string dir = cfg.GetValueString("LOG:target", "sharding.log");

	logging::add_file_log
	(
		keywords::file_name = dir,
		keywords::format = "[%TimeStamp%] [%Severity%] %Message%"
	);

	logging::add_file_log(dir);

	logging::core::get()->set_filter
	(
		logging::trivial::severity >= cfg.GetValueInt("LOG:level", logging::trivial::info)//
	);	

	BOOST_LOG_TRIVIAL(info) << "Log File Opened";

	logging::add_common_attributes();
}

std::vector<std::string> splitstring(const std::string& str, char delim)
{
	auto result = std::vector<std::string>{};
	auto ss = std::stringstream{ str };

	for (std::string line; std::getline(ss, line, delim);)
	{
		if (line[line.length() - 1] == '\r')
		{
			line = line.substr(0, line.length() - 1);
		}
		result.push_back(line);
	}


	return result;
}

DataResponse ShardingCore::ReadDataHTTP(std::string Content)
{
	std::vector<std::string> channels;
	std::vector<std::string> lines = splitstring(Content, '\n');

	Query Qry;
	Qry.from = 0;
	Qry.Options = 0;
	Qry.MaxSamples = 0;
	Qry.to = 99999999999;

	std::vector<std::string> FinalChannels;
	std::unordered_map<int, int> Mapping;
	std::string alpha;
	std::string bravo;

	int indx = 0;
	for (size_t x = 0; x < lines.size(); x++)
	{
		if (x == 0)
		{
			std::vector<std::string> pieces = splitstring(lines[x], ';');
			for (size_t y = 0; y < pieces.size(); y++)
			{
				int ps = pieces[y].find('=');
				if (ps != string::npos)
				{
					alpha = pieces[y].substr(0, ps);
					bravo = pieces[y].substr(ps + 1);

					if (alpha == "from")
					{
						//Qry.from = bravo
					}
				}
			}
		}
		else
		{
			if (lines[x].find("*") != string::npos)
			{
				//This is a like/regex expression
				sqlite3_stmt* statement;
				std::string qry = "SELECT name,code FROM series WHERE name LIKE '";
				std::string nw = lines[x];
				for (int q = 0; q < nw.length(); q++)
				{
					if (nw[q] == '*')
					{
						nw[q] = '%';
					}
				}
				qry += nw;
				qry += "'";
				if (sqlite3_prepare_v2(dbhandle, qry.c_str(), qry.size(), &statement, NULL) == SQLITE_OK)
				{
					int rw = sqlite3_step(statement);
					while ((rw == SQLITE_ROW) || (rw == SQLITE_OK))
					{
						FinalChannels.push_back((const char *)sqlite3_column_text(statement, 0));
						Qry.Channels.push_back((const char*)sqlite3_column_text(statement,1));
						//Mapping[indx] = x-1;
						indx++;

						rw = sqlite3_step(statement);
					}
				}
				else
				{
					cout << "DB Error: " << sqlite3_errmsg(dbhandle) << endl;
				}

			}
			else
			{				
				if (ChannelNames.find(lines[x]) != ChannelNames.end())
				{
					FinalChannels.push_back(lines[x]);
					Qry.Channels.push_back(Channels[ChannelNames[lines[x]]].code);
					//Mapping[indx] = x-1;
					indx++;
				}
			}
		}
	}

	std::string Response = "";

	if (Qry.Channels.size() > 0)
	{

		for (size_t x = 0; x < FinalChannels.size(); x++)
		{
			if (x > 0)
				Response += ",";
			Response += FinalChannels[x];
		}
		Response += "\r\n";

		QueryCursor* Csr = Hist.GetHistory(Qry);
		while (Csr->Next() >= 0)
		{
			QueryRow Rw = Csr->GetRow();
			Response += std::to_string(Rw.ChannelID);
			Response += ",";
			Response += std::to_string(Rw.Value);
			Response += ",";
			Response += std::to_string(Rw.Timestamp);
			Response += "\r\n";
		}
		delete Csr;
	}	

	DataResponse DS(Response,0);	
	return DS;
}

DataResponse ShardingCore::WriteDataHTTP(std::string Content)
{
	std::vector<std::string> lines = splitstring(Content,'\n');
	WriteData WD;
	double val;
	TIMESTAMP tms;
	std::string leading;
	std::string tailing;
	std::string timing;
	for (int x = 0; x < lines.size(); x++)
	{
		if (lines[x][0] == '+')
		{
			//Adding a new Series
			leading = lines[x].substr(1);
			std::vector<std::string> options = splitstring(leading, ';');
			ChannelInfo CI;

			CI.MinValue = 0;
			CI.MaxValue = 100;
			CI.FirstRecord = 0;
			CI.Options = 0;
			CI.Units = "";
			CI.LastValue = nan("");
			CI.LastCompressed = -1;

			for (int x = 0; x < options.size(); x++)
			{
				int ps = options[x].find('=');
				if (ps >= 0)
				{
					leading = options[x].substr(0, ps);
					tailing = options[x].substr(ps + 1);
				}
				else
				{
					leading = options[x];
				}

				if (leading == "name")
				{
					CI.Name = tailing;
				}
				/*if (leading == "min")
				{
					scanf(tailing.c_str(), "%lf", &CI.MinValue);					
				}
				if (leading == "max")
				{
					scanf(tailing.c_str(), "%lf", &CI.MaxValue);
				}*/
				if (leading == "units")
				{
					CI.Units = tailing;
					if (tailing[tailing.size() - 1] == '\r')
					{
						CI.Units = CI.Units.substr(0, tailing.size() - 1);
					}
				}
				if (leading == "step")
				{
					if ((tailing == "") || (tailing == "1"))
					{
						CI.Options |= CHANNEL_STEPPED;
					}
				}				
			}

			RegisterChannel(&CI);
		}
		else
		{
			//Adding points to existing series
			int ps = lines[x].find('=');
			if (ps >= 0)
			{
				leading = lines[x].substr(0, ps);
				tailing = lines[x].substr(ps + 1);
				cout << "Line Detected: " << leading << endl;
				ps = tailing.find('@');
				tms = 0;
				if (ps >= 0)
				{
					timing = tailing.substr(ps + 1);
					tailing = tailing.substr(0, ps);
					sscanf(timing.c_str(), "%llu", &tms);
				}
				sscanf(tailing.c_str(), "%lf", &val);

				WD.Points.push_back(WriteDataPoint(leading, val, tms));
			}
		}
	}
	if (WD.Points.size() > 0)
	{
		Write(WD);
	}
	return DataResponse("Data Written!",0);
}

DataResponse ShardingCore::RegisterChannel(ChannelInfo *CI)
{
	//Check to see if this channel exists..
	std::unordered_map<std::string, int>::const_iterator found = ChannelNames.find(CI->Name);
	if (found != ChannelNames.end())
	{
		return DataResponse("Already Exists", 0);
	}

	//Generate a starter code
	char NewCode[33];	
	for (int x = 0; x < 32; x++)
	{
		NewCode[x] = 'a';
	}
	NewCode[31] = 'a' - 1;
	NewCode[32] = 0;	

	//Get highest code already in the database
	sqlite3_stmt* statement;
	std::string qry = "SELECT code FROM series ORDER BY code DESC LIMIT 1";
	sqlite3_prepare_v2(dbhandle, qry.c_str(), qry.size(), &statement, NULL);
	int rw = sqlite3_step(statement);
	if ((rw == SQLITE_ROW) || (rw == SQLITE_OK))
	{
		const unsigned char *txt = sqlite3_column_text(statement, 0);
		memcpy(NewCode, txt, 32);
		rw = sqlite3_step(statement);
	}
	sqlite3_finalize(statement);

	//Add one to the existing code
	NewCode[31]++;
	for (int x = 31; x >= 0; x--)
	{
		NewCode[x]++;
		if (NewCode[x] > 'z')
		{
			NewCode[x] = 'a';
		}
		else
			break;
	}

	//Store new code
	memcpy(CI->code, NewCode, 32);
	CI->code[32] = 0;

	//Prepare an INSERT string to add the series to the index.
	char numbuf[24];
	qry = "INSERT INTO series (name,min,max,units,firstrecord,lastrecord,code) VALUES ('";
	qry = qry + CI->Name + "',";
	sprintf(numbuf, "%f", CI->MinValue);
	qry = qry + numbuf;
	qry = qry + ",";
	sprintf(numbuf, "%f", CI->MaxValue);
	qry = qry + numbuf;
	qry = qry + ",'";
	qry = qry + CI->Units;
	qry = qry + "',0,0,'";
	qry = qry + NewCode;
	qry = qry + "')";
	if (sqlite3_prepare_v2(dbhandle, qry.c_str(), qry.size(), &statement, NULL) == SQLITE_OK)
	{
		rw = sqlite3_step(statement);
		if ((rw == SQLITE_ROW) || (rw == SQLITE_OK))
		{
			rw = sqlite3_step(statement);
		}
		sqlite3_finalize(statement);
	}
	else
	{
		const char *msg = sqlite3_errmsg(dbhandle);
		cout << "SQL Error: " << msg << endl;		
	}

	//Add to in-memory cache
	Channels.push_back(*CI);
	ChannelNames[CI->Name] = Channels.size()-1;
	ChannelCodes[NewCode] = Channels.size()-1;

	return DataResponse("Channel Created", 0);
}

bool ShardingCore::Write(WriteData WD)
{
	char buf[33];
	for (size_t x = 0; x < WD.Points.size(); x++)
	{
		WriteDataPoint WDP = WD.Points[x];
		if (ChannelNames.find(WDP.Point) != ChannelNames.end())
		{			
			int indx = ChannelNames[WDP.Point];
			ChannelInfo CI = Channels[indx];

			if (Compression == true)
			{
				if (CI.LastValue == WDP.Value)
				{
					CI.LastCompressed = WDP.Stamp;
					continue;
				}
			}

			if (WDP.Stamp == 0)
				Hist.RecordValue(CI.code, WDP.Value);
			else
				Hist.RecordValue(CI.code, WDP.Value,WDP.Stamp,0);

			if (Compression == true)
			{
				Channels[indx].LastCompressed = -1;
				Channels[indx].LastValue = WDP.Value;
			}
		}
		else
		{
			if (AutoRegister == true)
			{
				//Auto-register a data point
				ChannelInfo CI;
				CI.MinValue = cfg.GetValueFloat("AUTOCREATE:minimum", 0);
				CI.MaxValue = cfg.GetValueFloat("AUTOCREATE:maximum", 100);
				CI.Options = cfg.GetValueInt("AUTOCREATE:options", 0);
				CI.Units = cfg.GetValueString("AUTOCREATE:units", "");
				CI.Name = WDP.Point;

				RegisterChannel(&CI);

				int indx = ChannelNames[WDP.Point];
				if (WDP.Stamp == 0)
					Hist.RecordValue(Channels[indx].code, WDP.Value);
				else
					Hist.RecordValue(Channels[indx].code, WDP.Value, WDP.Stamp, 0);
			}			
		}
	}
	return true;
}

bool ShardingCore::SeriesForName(std::string nm, char **buf)
{
	return false;
}

bool ShardingCore::InitREST()
{
	BOOST_LOG_TRIVIAL(info) << "Opening REST Endpoint";
	int RESTThreads = cfg.GetValueInt("REST:threads", 2);
	unsigned short RESTPort = cfg.GetValueInt("REST:port",80);
	boost::asio::ip::address address = net::ip::make_address(cfg.GetValueString("REST:ip","0.0.0.0").c_str());
	auto const doc_root = std::make_shared<std::string>("");

	// The io_context is required for all I/O
	net::io_context ioc{ RESTThreads };

	// Create and launch a listening port
	std::make_shared<RESTInterface>(ioc, tcp::endpoint{ address, RESTPort }, doc_root, this)->run();

	// Capture SIGINT and SIGTERM to perform a clean shutdown
	net::signal_set signals(ioc, SIGINT, SIGTERM);
	signals.async_wait([&](beast::error_code const&, int) {
		// Stop the `io_context`. This will cause `run()`
		// to return immediately, eventually destroying the
		// `io_context` and all of the sockets in it.
		ioc.stop();
		});

	// Run the I/O service on the requested number of threads
	
	threads.reserve(RESTThreads - 1);
	for (auto i = RESTThreads - 1; i > 0; --i)
		threads.emplace_back(
			[&ioc]
			{
				ioc.run();
			});
	ioc.run();
	return true;
}

WriteDataPoint::WriteDataPoint(std::string name, double val, TIMESTAMP tms)
{
	Point = name;
	Value = val;
	Stamp = tms;
}

DataResponse::DataResponse(std::string Txt, int Code)
{
	Response = Txt;
	ErrorCode = Code;
}

