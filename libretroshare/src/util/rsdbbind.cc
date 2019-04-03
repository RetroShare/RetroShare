/*******************************************************************************
 * libretroshare/src/util: rsbdbind.cc                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 Christopher Evi-Parker <retroshare@lunamutt.com>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
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

