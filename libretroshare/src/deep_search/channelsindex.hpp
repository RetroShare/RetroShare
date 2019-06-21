/*******************************************************************************
 * RetroShare full text indexing and search implementation based on Xapian     *
 *                                                                             *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019  Asociación Civil Altermundi <info@altermundi.net>       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License version 3 as    *
 * published by the Free Software Foundation.                                  *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <vector>
#include <xapian.h>

#include "util/rstime.h"
#include "retroshare/rsgxschannels.h"
#include "retroshare/rsinit.h"
#include "util/rsurl.h"

struct DeepChannelsSearchResult
{
	std::string mUrl;
	double mWeight;
	std::string mSnippet;
};

struct DeepChannelsIndex
{
	/**
	 * @brief Search indexed GXS groups and messages
	 * @param[in] maxResults maximum number of acceptable search results, 0 for
	 * no limits
	 * @return search results count
	 */
	static uint32_t search( const std::string& queryStr,
	                        std::vector<DeepChannelsSearchResult>& results,
	                        uint32_t maxResults = 100 );

	static void indexChannelGroup(const RsGxsChannelGroup& chan);

	static void removeChannelFromIndex(RsGxsGroupId grpId);

	static void indexChannelPost(const RsGxsChannelPost& post);

	static void removeChannelPostFromIndex(
	        RsGxsGroupId grpId, RsGxsMessageId msgId );

	static uint32_t indexFile(const std::string& path);

private:

	enum : Xapian::valueno
	{
		/// Used to store retroshare url of indexed documents
		URL_VALUENO,

		/// @see Xapian::BAD_VALUENO
		BAD_VALUENO = Xapian::BAD_VALUENO
	};

	static const std::string& dbPath()
	{
		static const std::string dbDir =
		        RsAccounts::AccountDirectory() + "/deep_channels_xapian_db";
		return dbDir;
	}
};
