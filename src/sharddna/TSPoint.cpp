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

//#include "pch.h"
#include "TSPoint.h"
#include <math.h>
#include <memory.h>
#include "BTreeKey.h"

TSPoint::TSPoint()
{
	Time = 0;
	Value = FP_NAN;
}

TSPoint::TSPoint(BTreeKey* Key)
{	
	memcpy(&Value, Key->Payload(), sizeof(double));
	memcpy(&Time, Key->Key(), sizeof(TIMESTAMP));
}

TSPoint TSPoint::NullValue()
{
	TSPoint P;
	P.Time = 0;
	P.Value = FP_NAN;
	return P;
}