/*******************************************************************************
 * RetroShare full text indexing and search implementation based on Xapian     *
 *                                                                             *
 * Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
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

#include <system_error>
#include <vector>
#include <xapian.h>

#include "util/rstime.h"
#include "retroshare/rsgxsforums.h"
#include "retroshare/rsevents.h"
#include "deep_search/commonutils.hpp"

struct DeepForumsSearchResult
{
	std::string mUrl;
	double mWeight;
	std::string mSnippet;
};

struct DeepForumsIndex
{
	explicit DeepForumsIndex(const std::string& dbPath) :
	    mDbPath(dbPath), mWriteQueue(dbPath) {}

	/**
	 * @brief Search indexed GXS groups and messages
	 * @param[in] maxResults maximum number of acceptable search results, 0 for
	 * no limits
	 * @return search results count
	 */
	std::error_condition search( const std::string& queryStr,
	                             std::vector<DeepForumsSearchResult>& results,
	                             uint32_t maxResults = 100 );

	std::error_condition indexForumGroup(const RsGxsForumGroup& chan);

	std::error_condition removeForumFromIndex(const RsGxsGroupId& grpId);

	std::error_condition indexForumPost(const RsGxsForumMsg& post);

	std::error_condition removeForumPostFromIndex(
	        RsGxsGroupId grpId, RsGxsMessageId msgId );

	static std::string dbDefaultPath();

private:
	static std::string forumIndexId(const RsGxsGroupId& grpId);
	static std::string postIndexId(
	        const RsGxsGroupId& grpId, const RsGxsMessageId& msgId );

	enum : Xapian::valueno
	{
		/// Used to store retroshare url of indexed documents
		URL_VALUENO,

		/// @see Xapian::BAD_VALUENO
		BAD_VALUENO = Xapian::BAD_VALUENO
	};

	const std::string mDbPath;

	DeepSearch::StubbornWriteOpQueue mWriteQueue;
};
