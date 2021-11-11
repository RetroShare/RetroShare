/*******************************************************************************
 * RetroShare full text indexing and search implementation based on Xapian     *
 *                                                                             *
 * Copyright (C) 2018-2021 Gioacchino Mazzurco <gio@eigenlab.org>              *
 * Copyright (C) 2019-2021  Asociación Civil Altermundi <info@altermundi.net>  *
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

#include <string>
#include <cstdint>
#include <vector>
#include <xapian.h>
#include <map>
#include <functional>

#include "retroshare/rstypes.h"
#include "deep_search/commonutils.hpp"

struct DeepFilesSearchResult
{
	DeepFilesSearchResult() : mWeight(0) {}

	RsFileHash mFileHash;
	double mWeight;
	std::string mSnippet;
};

class DeepFilesIndex
{
public:
	explicit DeepFilesIndex(const std::string& dbPath):
	    mDbPath(dbPath), mWriteQueue(dbPath) {}

	/**
	 * @brief Search indexed files
	 * @param[in] maxResults maximum number of acceptable search results, 0 for
	 * no limits
	 * @return search results count
	 */
	std::error_condition search( const std::string& queryStr,
	                 std::vector<DeepFilesSearchResult>& results,
	                 uint32_t maxResults = 100 );

	/**
	 * @return false if file could not be indexed because of error or
	 *	unsupported type, true otherwise.
	 */
	std::error_condition indexFile(
	        const std::string& path, const std::string& name,
	        const RsFileHash& hash );

	/**
	 * @brief Remove file entry from database
	 * @return false on error, true otherwise.
	 */
	std::error_condition removeFileFromIndex(const RsFileHash& hash);

	static std::string dbDefaultPath();

	using IndexerFunType = std::function<
	uint32_t( const std::string& path, const std::string& name,
	Xapian::TermGenerator& xTG, Xapian::Document& xDoc ) >;

	static bool registerIndexer(
	        int order, const IndexerFunType& indexerFun );

private:
	enum : Xapian::valueno
	{
		/// Used to store RsFileHash of indexed documents
		FILE_HASH_VALUENO,

		/** Used to check if some file need reindex because was indexed with an
		 * older version of the indexer */
		INDEXER_VERSION_VALUENO,

		/** Used to check if some file need reindex because was indexed with an
		 * older version of the indexer */
		INDEXERS_COUNT_VALUENO,

		/// @see Xapian::BAD_VALUENO
		BAD_VALUENO = Xapian::BAD_VALUENO
	};

	const std::string mDbPath;

	DeepSearch::StubbornWriteOpQueue mWriteQueue;

	/** Storage for indexers function by order */
	static std::multimap<int, IndexerFunType> indexersRegister;
};
