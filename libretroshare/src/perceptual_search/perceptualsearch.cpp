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

#include <pHash.h>

#include "perceptual_search/perceptualsearch.hpp"
#include "util/rsdebug.h"
#include "retroshare/rsinit.h"

using namespace RetroShare;

PerceptualFileIndex::PerceptualFileIndex(const string& dbPath):
    mDbPath(dbPath) {}
PerceptualFileIndex::~PerceptualFileIndex() = default;

std::error_condition PerceptualFileIndex::search(
        PHash center, uint32_t radius,
        std::vector<PerceptualSearchResult> results, uint32_t maxResults )
{
	radius = radius < MAX_SEARCH_RADIUS ? radius : MAX_SEARCH_RADIUS;

	for (const auto& it: mStorage )
	{
		const auto& el = it.second;
		auto distance = static_cast<uint32_t>(
		            ph_hamming_distance(center, el.pHash) );
		if(distance <= radius)
			results.push_back(
			            PerceptualSearchResult{el.hash, el.pHash, distance} );
	}

	if(results.size() > maxResults)
	{
		using E_t = PerceptualSearchResult;
		std::sort( std::begin(results), std::end(results),
		           []( const E_t& elA, const E_t& elB)
		{ return elA.distance > elB.distance; } );
		results.resize(maxResults);
	}

	return std::error_condition();
}

std::error_condition PerceptualFileIndex::indexFile(
            const std::string& path, const RsFileHash& hash, PHash& pHash )
{
	auto ec = perceptualHash(path, pHash);

	RS_DBG("Indexing: ", path, " pHash: ", pHash, " ec: ", ec);

	if(ec) return ec;

	mStorage.insert(std::make_pair(hash, Item_t{path, hash, pHash}));
	return std::error_condition();
}

std::error_condition PerceptualFileIndex::removeFileFromIndex(
        const RsFileHash& hash )
{
	mStorage.erase(hash);
	return std::error_condition();
}

std::error_condition PerceptualFileIndex::perceptualHash(
        const std::string& filePath, PHash& hash )
{
	if(ph_dct_imagehash(filePath.c_str(), hash) == -1)
		return std::errc::invalid_argument;

	return std::error_condition();
}

/*static*/ std::string PerceptualFileIndex::dbDefaultPath()
{ return RsAccounts::AccountDirectory() + "/perceptual_file_index_db"; }
