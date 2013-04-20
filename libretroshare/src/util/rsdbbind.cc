/*
 * RetroShare : RetroDb functionality
 *
 * Copyright 2013 Christopher Evi-Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "rsdbbind.h"

RsDoubleBind::RsDoubleBind(double value, int index)
 : RetroBind(DOUBLE, index), mValue(value) {}

RsStringBind::RsStringBind(const std::string& value, int index)
 : RetroBind(STRING, index), mValue(value) {}

RsInt32Bind::RsInt32Bind(int32_t value, int index)
 : RetroBind(INT32, index), mValue(value) {}

RsInt64bind::RsInt64bind(int64_t value, int index)
 : RetroBind(INT64, index), mValue(value) {}

RsBoolBind::RsBoolBind(bool value, int index)
 : RetroBind(BOOL, index), mValue(value) {}

RsBlobBind::RsBlobBind(char* data, uint32_t dataLen, int index)
 : RetroBind(BLOB, index), mData(data), mDataLen(dataLen) {}

bool RsDoubleBind::bind(sqlite3_stmt* const stm) const
{
	return (SQLITE_OK == sqlite3_bind_double(stm, getIndex(), mValue));
}

bool RsStringBind::bind(sqlite3_stmt* const stm) const
{
	return (SQLITE_OK == sqlite3_bind_text(stm, getIndex(), mValue.c_str(), mValue.size(), SQLITE_TRANSIENT));
}

bool RsInt32Bind::bind(sqlite3_stmt* const stm) const
{
	return (SQLITE_OK == sqlite3_bind_int(stm, getIndex(), mValue));
}

bool RsInt64bind::bind(sqlite3_stmt* const stm) const
{
	return (SQLITE_OK == sqlite3_bind_int64(stm, getIndex(), mValue));
}

bool RsBoolBind::bind(sqlite3_stmt* const stm) const
{
	return (SQLITE_OK == sqlite3_bind_int(stm, getIndex(), mValue ? 1 : 0));
}

bool RsBlobBind::bind(sqlite3_stmt* const stm) const
{
	return (SQLITE_OK == sqlite3_bind_blob(stm, getIndex(), mData, mDataLen, SQLITE_TRANSIENT));
}

