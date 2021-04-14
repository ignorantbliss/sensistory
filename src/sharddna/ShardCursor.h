#pragma once

#include <string>
#include "History.h"
#include "Cursor.h"
#include "ShardDNA.h"

using namespace std;

/**
 * @brief Used to navigate the results of a single channel query
*/
class ShardCursor
{
public:
	/**
	 * @brief Constructs a ShardCursor
	 * @param H A history object that manages this cursor
	 * @param C A non-sharded cursor
	*/
	ShardCursor(History *H,std::shared_ptr<::Cursor> C);
	~ShardCursor();
	
	void SetParent(History* H);
	void SetSeries(std::string S);

	/**
	 * @brief Used to estimate the cost of calling 'Next'
	 * @return Returns a number representing a rough cost estimate
	*/
	int NextStepCost();

	/**
	 * @brief Moves to the next record
	 * @return Successful if the result is > 0. Returns CURSOR_OK, CURSOR_PRE or CURSOR_POST on success or CURSOR_END on failure/end of file.
	 */
	int Next();

	/**
	 * @brief Gets the timestamp of the current record.
	 * @return A TIMESTAMP value.
	*/
	TIMESTAMP Timestamp();

	/**
	 * @brief Gets the value of the current record.
	 * @return A double/floating point value.
	*/
	double Value();
protected:
	std::string Series;
	PCursor ActiveCursor;
	History* Hist;
	bool EoF;
	TIMESTAMP LastTimestamp;
};

