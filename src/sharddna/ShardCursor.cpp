//#include "pch.h"
#include "Cursor.h"
#include "ShardCursor.h"

ShardCursor::ShardCursor(History* H, std::shared_ptr<::Cursor> C)
{
	Hist = H;
	ActiveCursor = C;
	EoF = false;
	LastTimestamp = 0;
}

ShardCursor::~ShardCursor()
{

}

int ShardCursor::Next()
{	
	int Ret = ActiveCursor->Next();
	if (Ret == CURSOR_END)
	{
		if (EoF == false)
		{		
			//Check to see if there is anything else in the next shard...			
			//DStore DS = Hist->GetNextShardWith(LastTimestamp, Series.c_str());
			std::shared_ptr<Cursor> Curse = Hist->GetNextShardWith(LastTimestamp, Series.c_str());
			if (Curse == NULL)
			{
				this->EoF = true;
			}
			else
			{
				//OK - we have a new shard.
				ActiveCursor = Curse;
				return ActiveCursor->Next();
			}
		}
	}
	else
	{
		LastTimestamp = Timestamp();
	}
	return Ret;
}

TIMESTAMP ShardCursor::Timestamp()
{
	return *(TIMESTAMP*)ActiveCursor->Key();
}


double ShardCursor::Value()
{
	return *(double*)ActiveCursor->Value();
}

void ShardCursor::SetParent(History* H)
{
	Hist = H;
}

void ShardCursor::SetSeries(std::string S)
{
	Series = S;
}

int ShardCursor::NextStepCost()
{
	if (this->EoF == true)
	{
		return 1;
	}
	if (ActiveCursor->Index < ActiveCursor->Active->KeyCount() - 1)
	{
		return 0;
	}
	else
	{
		if (ActiveCursor->Active->Next == 0)
		{
			return 9999999;
		}
		return (int)(ActiveCursor->Active->Next - ActiveCursor->Offset);
	}
}