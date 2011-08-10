/*
 * libretroshare/src/distrib: p3distribcacheopt.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2011 by Christopher Evi-Parker
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

#ifndef P3DISTRIBCACHEOPT_H_
#define P3DISTRIBCACHEOPT_H_

#include <string>

#include "util/pugixml.h"



class p3DistribCacheOpt {

public:

	p3DistribCacheOpt();
	~p3DistribCacheOpt();

	/*!
	 * This updates the cache document with pending msg and grp cache data
	 */
	void updateCacheDocument();


	/*!
	 * to find if grps messages have been loaded (assumes grps have been loaded first)
	 * @param cached true if grp has been loaded, false if not
	 * @return true is grp entry does not exist in table, false if not
	 */
	bool grpCacheOpted(const std::string& grpId);

	/*!
	 * encrypts and saves cache file
	 */
	bool saveHistoryCacheFile(const std::string&);

	/*!
	 * decrypte and save cache file
	 * @return
	 */
	bool loadHistoryCacheFile(const std::string&);

private:

	pugi::xml_document mCacheDoc;
};

#endif /* P3DISTRIBCACHEOPT_H_ */
