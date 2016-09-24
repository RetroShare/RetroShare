#ifndef RSDBBIND_H_
#define RSDBBIND_H_

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

#include <string>
#include <inttypes.h>

#ifdef NO_SQLCIPHER
#include <sqlite3.h>
#else
#include <sqlcipher/sqlite3.h>
#endif

class RetroBind
{
public:

	enum BindType { BLOB=1, STRING, INT32, INT64, DOUBLE, BOOL } ;
	RetroBind(const BindType& type, int index) : mIndex(index), mType(type) {}

	virtual bool bind(sqlite3_stmt* const stm) const = 0;
	BindType getType() const { return mType; }
	inline int getIndex() const { return mIndex;}
	virtual ~RetroBind() {}

private:
	int mIndex;
	BindType mType;
};

class RsDoubleBind : public RetroBind
{
public:
	RsDoubleBind(double value, int index);
	bool bind(sqlite3_stmt* const stm) const;
	double mValue;
};

class RsStringBind : public RetroBind
{
public:
	RsStringBind(const std::string& value, int index);
	bool bind(sqlite3_stmt* const stm) const ;
	std::string mValue;
};

class RsInt32Bind : public RetroBind
{
public:
	RsInt32Bind(int32_t value, int index);
	bool bind(sqlite3_stmt* const stm) const ;
	int32_t mValue;
};

class RsInt64bind : public RetroBind
{
public:
	RsInt64bind(int64_t value, int index);
	bool bind(sqlite3_stmt* const stm) const ;
	int64_t mValue;
};

class RsBoolBind : public RetroBind
{
public:
	RsBoolBind(bool value, int index);
	bool bind(sqlite3_stmt* const stm) const ;
	bool mValue;
};

class RsBlobBind : public RetroBind
{
public:
	RsBlobBind(char* data, uint32_t dataLen, int index);
	bool bind(sqlite3_stmt* const stm) const;
	char* mData;
	uint32_t mDataLen;
};


#endif /* RSDBBIND_H_ */
