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
#include <vorbis/vorbisfile.h>
#include <cctype>

struct RsDeepOggFileIndexer
{
	RsDeepOggFileIndexer()
	{
		DeepFilesIndex::registerIndexer(30, indexOggFile);
	}

	static uint32_t indexOggFile(
	        const std::string& path, const std::string& /*name*/,
	        Xapian::TermGenerator& xTG, Xapian::Document& xDoc )
	{
		Dbg3() << __PRETTY_FUNCTION__ << " " << path << std::endl;

		OggVorbis_File vf;
		int ret = ov_fopen(path.c_str(), &vf);

		if(ret == 0 && vf.vc)
		{
			vorbis_comment& vc = *vf.vc;
			std::string docData = xDoc.get_data();
			for (int i = 0; i < vc.comments; ++i)
			{
				using szt = std::string::size_type;
				std::string userComment(
				            vc.user_comments[i],
				            static_cast<szt>(vc.comment_lengths[i]) );

				if(userComment.empty()) continue;

				szt equalPos = userComment.find('=');
				if(equalPos == std::string::npos) continue;

				std::string tagName = userComment.substr(0, equalPos);
				if(tagName.empty()) continue;

				std::string tagValue = userComment.substr(equalPos + 1);
				if(tagValue.empty()) continue;

				/* Ogg tags should be uppercases but not all the softwares
				 * enforce it */
				for (auto& c: tagName) c = static_cast<char>(toupper(c));

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
				xTG.index_text(userComment);
				docData += userComment + "\n";
			}
			xDoc.set_data(docData);

			ov_clear(&vf);
			return 99;
		}

		return 0;
	}

	RS_SET_CONTEXT_DEBUG_LEVEL(1)
};
