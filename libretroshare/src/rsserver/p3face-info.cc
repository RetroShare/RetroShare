/*******************************************************************************
 * libretroshare/src/rsserver: p3face-info.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2015 by RetroShare Team <retroshare.project@gmail.com>            *
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

#include "rsserver/p3face.h"
#include <bzlib.h>
#include <openssl/ssl.h>
#include <zlib.h>

#ifdef NO_SQLCIPHER
#	include <sqlite3.h>
#else
#	include <sqlcipher/sqlite3.h>
#endif

#ifdef RS_USE_LIBUPNP
#	include "upnp/upnpconfig.h"
#elif defined(RS_USE_LIBMINIUPNPC)
#	include "miniupnpc/miniupnpc.h"
#endif // def RS_USE_LIBUPNP

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

#ifdef RS_USE_LIBUPNP
	libraries.push_back(RsLibraryInfo("UPnP (libupnp)", UPNP_VERSION_STRING));
#elif defined(RS_USE_LIBMINIUPNPC)
	libraries.push_back(RsLibraryInfo("UPnP (MiniUPnP)", MINIUPNPC_VERSION));
#endif // def RS_USE_LIBUPNP

	libraries.push_back(RsLibraryInfo("Zlib", ZLIB_VERSION));
}
