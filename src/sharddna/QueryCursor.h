#pragma once

#include <vector>
#include "Cursor.h"
#include "ShardCursor.h"
#include "History.h"

/**
 * @brief Contains the results of a single row
*/
struct QueryRow
{
	int ChannelID; /** <The number of the channel (ie. the 1st requested channel is 0)*/
	TIMESTAMP Timestamp; /** <The record time stamp*/
	double Value; /** <The record value*/
};

/**
 * @brief A cursor that handles multiple channels in a single query
*/
class QueryCursor
{	
public:
	/**
	 * @brief Constructs a QueryCursor
	 * @warning QueryCursors should be created from History::Query and not directly created in end-user code.
	*/
	QueryCursor();
	~QueryCursor();

	/**
	 * @brief Gets the values of the current row
	 * @return A QueryRow for the current row
	*/
	QueryRow GetRow();

	/**
	 * @brief Moves to the next record
	 * @return Successful if the result is > 0. Returns CURSOR_OK, CURSOR_PRE or CURSOR_POST on success or CURSOR_END on failure/end of file.
	 */
	int Next();

	/**
	 * @brief Checks if the cursor is at the end
	 * @return Returns true if at the end of the results, otherwise false.
	*/
	bool EoF();

protected:
	int Channels;
	std::vector<PSC> Cursors;	
	void AddChannel(PSC Curse);

	friend History;

private:
	int ActiveChannel;
	
	struct PendingNext
	{
		int Chan;
		std::streamoff offset;
	};

	vector<PendingNext> PendingOpps;
	bool Empty;

};

