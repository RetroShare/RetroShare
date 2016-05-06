
/*
 * "$Id: p3face-info.cc,v 1.5 2007-04-15 18:45:23 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2015 by RetroShare Team.
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

#include "rsserver/p3face.h"
#include <bzlib.h>
#include <openssl/ssl.h>
#include <zlib.h>

#ifdef NO_SQLCIPHER
#include <sqlite3.h>
#else
#include <sqlcipher/sqlite3.h>
#endif

std::string RsServer::getSQLCipherVersion()
{
	sqlite3* mDb;
	std::string versionstring("");
	const char* version;
	int rc = sqlite3_open_v2("", &mDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE , NULL); //create DB in a temp file

	if(rc){
		std::cerr << "Can't open database, Error code: " <<  sqlite3_errmsg(mDb)
				  << std::endl;
		sqlite3_close(mDb);
		mDb = NULL;
		return "";
	 }

	std::string sqlQuery = "PRAGMA cipher_version;";
	sqlite3_stmt* stmt = NULL;
	rc = sqlite3_prepare_v2(mDb, sqlQuery.c_str(), sqlQuery.length(), &stmt, NULL);
	if (rc == SQLITE_OK) {
		rc = sqlite3_step(stmt);
		switch (rc) {
		case SQLITE_ROW:
			version = (const char *)sqlite3_column_text(stmt, 0); //not needed to free
			versionstring.append(version);
			break;
		case SQLITE_DONE:
			break;
		default:
			std::cerr << "RetroDb::tableExists(): Error executing statement (code: " << rc << ")"
					  << std::endl;
			break;
		}
	}

	if (stmt) {
		sqlite3_finalize(stmt);
	}
	sqlite3_close(mDb);	// no-op if mDb is NULL (https://www.sqlite.org/c3ref/close.html)
	return versionstring;
}

void RsServer::getLibraries(std::list<RsLibraryInfo> &libraries)
{
	libraries.push_back(RsLibraryInfo("bzip2", BZ2_bzlibVersion()));
	libraries.push_back(RsLibraryInfo("OpenSSL", SSLeay_version(SSLEAY_VERSION)));
	libraries.push_back(RsLibraryInfo("SQLite", SQLITE_VERSION));
#ifndef NO_SQLCIPHER
	libraries.push_back(RsLibraryInfo("SQLCipher", getSQLCipherVersion()));
#endif
	libraries.push_back(RsLibraryInfo("Zlib", ZLIB_VERSION));
}
