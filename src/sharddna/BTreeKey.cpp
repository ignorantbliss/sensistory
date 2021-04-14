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
#include "BTreeKey.h"


BTreeKey::BTreeKey(BTreeKeyTemplate *Tmp)
{	
	_template = Tmp;
	_buffer = new char[Tmp->KeySize];
	_payload = new char[Tmp->PayloadSize];
	_code = 0;		
}

BTreeKey::BTreeKey(BTreeKey &Origin)
{
	_template = Origin._template;
	_buffer = new char[_template->KeySize];
	_payload = new char[_template->PayloadSize];
	memcpy(_buffer, Origin._buffer, _template->KeySize);
	memcpy(_payload, Origin._payload, _template->PayloadSize);
	_code = 0;
}

BTreeKey::~BTreeKey()
{
	if (_buffer != NULL)
	{
		delete[] _buffer;
		_buffer = NULL;
	}
	if (_payload != NULL)
	{
		delete[]_payload;
		_payload = NULL;
	}
}

int BTreeKey::Write(char* buf)
{
	buf[0] = _code;	
	memcpy(buf + 1, _buffer, _template->KeySize);
	memcpy(buf + _template->KeySize + 1, _payload, _template->PayloadSize);
	return _template->KeySize + 1 + _template->PayloadSize;
}

int BTreeKey::Read(const char* buf)
{
	_code = buf[0];
	memcpy(_buffer, buf + 1, _template->KeySize);
	memcpy(_payload, buf + _template->KeySize + 1, _template->PayloadSize);
	return _template->KeySize + 1 + _template->PayloadSize;
}

int BTreeKey::Length()
{
	return _template->PayloadSize + _template->KeySize + 1;
}

const void* BTreeKey::Payload()
{
	return _payload;
}

const void* BTreeKey::Key()
{
	return _buffer;
}

unsigned char BTreeKey::Code()
{
	return _code;
}

void BTreeKey::Delete()
{
	_code |= KEYCODE_DELETED;	
}

BTreeKeyTemplate::BTreeKeyTemplate()
{
	KeySize = 32;
	PayloadSize = sizeof(std::streamoff);
	CompareInverse = false;
}

int memrcmp(const char* a, const char* b, int ln)
{
	for (int x = ln - 1; x >= 0; x--)
	{
		if (a[x] > b[x])
			return 1;
		else
		{
			if (a[x] < b[x])
				return -1;
		}
	}
	return 0;
}

BTreeKeyTemplate::~BTreeKeyTemplate()
{

}

int BTreeKeyTemplate::DiskKeySize()
{
	return KeySize + PayloadSize + 1;
}

int BTreeKey::CompareTo(BTreeKey* To)
{
	if (_template->CompareInverse == false)
		return memcmp(_buffer, To->_buffer, _template->KeySize);
	else
		return memrcmp(_buffer, To->_buffer, _template->KeySize);
}

int BTreeKey::CompareTo(const void *ky)
{
	if (_template->CompareInverse == false)
		return memcmp(_buffer, ky, _template->KeySize);
	else
		return memrcmp(_buffer, (const char *)ky, _template->KeySize);
}

void BTreeKey::Set(const void* ky, const void* pl)
{
	memcpy(_buffer, ky, _template->KeySize);
	memcpy(_payload, pl, _template->PayloadSize);
}
