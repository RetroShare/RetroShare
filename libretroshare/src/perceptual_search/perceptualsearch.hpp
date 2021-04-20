/*******************************************************************************
 * RetroShare perceptual indexing and search implementation based on pHash     *
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
#include <string>
#include <vector>

#include "util/rsmemory.h"
#include "retroshare/rstypes.h"

namespace RetroShare
{
typedef uint64_t PHash;

struct PerceptualSearchResult
{
	RsFileHash hash;
	PHash pHash;
	uint32_t distance;
};

class PerceptualFileIndex
{
public:
	static constexpr uint32_t MAX_SEARCH_RADIUS = 20;

	explicit PerceptualFileIndex(const std::string& dbPath);
	~PerceptualFileIndex();

	static PerceptualFileIndex& instance()
	{
		static PerceptualFileIndex singleton(dbDefaultPath());
		return singleton;
	}

	/**
	 * @brief Search indexed files
	 * @param[in] center perceptual hash to search for similar items in the
	 *	index
	 * @param[in] radius tolerance in term of hamming distance to match items in
	 *	the index search
	 * @param[out] results storage for results
	 * @param[in] maxResults optional maximum number of acceptable search
	 *	results, 0 for no limits
	 * @return error information if some error occurred, 0/SUCCESS otherwise
	 */
	std::error_condition search(
	        PHash center, uint32_t radius,
	        std::vector<PerceptualSearchResult> results,
	        uint32_t maxResults = 100 );

	/**
	 * @brief Perceptually hash file and add it to the index
	 * @param[in] path path of the file to index
	 * @param[in] hash retroshare file hash
	 * @param[out] pHash optional storage for calculated file perceptual hash
	 * @return error information if some error occurred, 0/SUCCESS otherwise
	 */
	std::error_condition indexFile(
	        const std::string& path, const RsFileHash& hash,
	        PHash& pHash = RS_DEFAULT_STORAGE_PARAM(PHash, 0) );

	/**
	 * @brief Remove file entry from database
	 * @return error information if some error occurred, 0/SUCCESS otherwise
	 */
	std::error_condition removeFileFromIndex(const RsFileHash& hash);

	/**
	 * @brief Calculate perceptual hash of given file
	 * @return error information if some error occurred, 0/SUCCESS otherwise
	 */
	std::error_condition perceptualHash(
	        const std::string& filePath, PHash& hash );

	static std::string dbDefaultPath();

private:
	std::string mDbPath;

	struct Item_t
	{
		std::string path;
		RsFileHash hash;
		PHash pHash;
	};

	std::map<RsFileHash, Item_t> mStorage;
};

}
