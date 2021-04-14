//#include "pch.h"
#include "QueryCursor.h"

//#define QUERY_MOMENTUM

QueryCursor::QueryCursor()
{
	ActiveChannel = 0;
	Channels = 0;	
	Empty = false;
}

QueryCursor::~QueryCursor()
{
	
}

int QueryCursor::Next()
{
	if (Empty == false)
	{
#ifndef QUERY_MOMENTUM
		while (Cursors[ActiveChannel].get() == NULL)
		{
			ActiveChannel++;
			if (ActiveChannel >= Channels)
			{
				Empty = true;
				return CURSOR_END;
			}
		}
		int R = Cursors[ActiveChannel]->Next();
		if (R < 0)
		{
			ActiveChannel++;
			if (ActiveChannel >= Channels)
			{
				Empty = true;
				return CURSOR_END;
			}
			while (Cursors[ActiveChannel].get() == NULL)
			{
				ActiveChannel++;
				if (ActiveChannel >= Channels)
				{
					Empty = true;
					return CURSOR_END;
				}
			}
			if (ActiveChannel >= Channels)
			{
				Empty = true;
				return CURSOR_END;
			}
			R = Next();
			
		}
		return R;
#else
		int R = Cursors[ActiveChannel]->Next();
		if (R == CURSOR_END)
		{
			//When the cursor ends, move to the next valid channel.
			for (int x = 0; x < Cursors.size(); x++)
			{
				if (Cursors[x]->)
				ActiveChannel++;
				if (ActiveChannel >= Channels)
				{
					Empty = true;
					return CURSOR_END;
				}
			}
			
		}
		if (R == CURSOR_CHANGING)
		{
			ActiveChannel++;
			if (ActiveChannel >= Channels)
			{
				Empty = true;
				return CURSOR_END;
			}
		}
		return R;
#endif
	}
	return CURSOR_END;
}

bool QueryCursor::EoF()
{
	return Empty;
}

QueryRow QueryCursor::GetRow()
{
	QueryRow QR;	
	QR.ChannelID = ActiveChannel;
	QR.Timestamp = Cursors[ActiveChannel]->Timestamp();
	QR.Value = Cursors[ActiveChannel]->Value();
	return QR;
}

void QueryCursor::AddChannel(PSC C)
{
	Cursors.push_back(C);
	Channels++;
}