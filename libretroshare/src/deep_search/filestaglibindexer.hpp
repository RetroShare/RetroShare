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

#include "deep_search/filesindex.hpp"
#include "util/rsdebug.h"

#include <xapian.h>
#include <string>
#include <memory>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

struct RsDeepTaglibFileIndexer
{
	RsDeepTaglibFileIndexer()
	{
		DeepFilesIndex::registerIndexer(40, indexFile);
	}

	static uint32_t indexFile(
	        const std::string& path, const std::string& /*name*/,
	        Xapian::TermGenerator& xTG, Xapian::Document& xDoc )
	{
		Dbg4() << __PRETTY_FUNCTION__ << " " << path << std::endl;

		TagLib::FileRef tFile(path.c_str());
		if(tFile.isNull()) return 0;

		const TagLib::Tag* tag = tFile.tag();
		if(!tag) return 0;

		TagLib::PropertyMap tMap = tag->properties();

		unsigned validCommentsCnt = 0;
		std::string docData = xDoc.get_data();
		for( TagLib::PropertyMap::ConstIterator mIt = tMap.begin();
		     mIt != tMap.end(); ++mIt )
		{
			if(mIt->first.isNull() || mIt->first.isEmpty()) continue;
			std::string tagName(mIt->first.upper().to8Bit());

			if(mIt->second.isEmpty()) continue;
			std::string tagValue(mIt->second.toString(", ").to8Bit(true));
			if(tagValue.empty()) continue;

			if(tagName == "ARTIST")
				xTG.index_text(tagValue, 1, "A");
			else if (tagName == "DESCRIPTION")
				xTG.index_text(tagValue, 1, "XD");
			else if (tagName == "TITLE")
				xTG.index_text(tagValue, 1, "S");
			else if(tagName.find("COVERART") != tagName.npos)
				continue; // Avoid polluting the index with binary data
			else if (tagName.find("METADATA_BLOCK_PICTURE") != tagName.npos)
				continue; // Avoid polluting the index with binary data

			// Index fields without prefixes for general search.
			xTG.increase_termpos();
			std::string fullComment(tagName + "=" + tagValue);
			xTG.index_text(fullComment);
			docData += fullComment + "\n";

			Dbg2() << __PRETTY_FUNCTION__ << " Indexed " << tagName << "=\""
			       << tagValue << '"' << std::endl;

			++validCommentsCnt;
		}

		if(validCommentsCnt > 0)
		{
			Dbg1() << __PRETTY_FUNCTION__ << " Successfully indexed: " << path
			       << std::endl;

			xDoc.set_data(docData);
			return 99;
		}

		/* Altought the file appears to be supported by taglib, no comments has
		 * been found so return less then 50 maybe another indexer is capable of
		 * extracting information */
		return 30;
	}

	RS_SET_CONTEXT_DEBUG_LEVEL(3)
};
