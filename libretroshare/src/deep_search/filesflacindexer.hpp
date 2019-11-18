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
#include <FLAC++/metadata.h>
#include <cctype>
#include <memory>

struct RsDeepFlacFileIndexer
{
	RsDeepFlacFileIndexer()
	{
		DeepFilesIndex::registerIndexer(31, indexFlacFile);
	}

	static uint32_t indexFlacFile(
	        const std::string& path, const std::string& /*name*/,
	        Xapian::TermGenerator& xTG, Xapian::Document& xDoc )
	{
		Dbg3() << __PRETTY_FUNCTION__ << " " << path << std::endl;

		using FlacChain = FLAC::Metadata::Chain;
		std::unique_ptr<FlacChain> flacChain(new FlacChain);

		if(!flacChain->is_valid())
		{
			RsErr() << __PRETTY_FUNCTION__ << " Failed creating FLAC Chain 1"
			        << std::endl;
			return 1;
		}

		if(!flacChain->read(path.c_str(), false))
		{
			Dbg3() << __PRETTY_FUNCTION__ << " Failed to open the file as FLAC"
			       << std::endl;

			flacChain.reset(new FlacChain);
			if(!flacChain->is_valid())
			{
				RsErr() << __PRETTY_FUNCTION__
				        << " Failed creating FLAC Chain 2" << std::endl;
				return 1;
			}
			if(!flacChain->read(path.c_str(), true))
			{
				Dbg3() << __PRETTY_FUNCTION__
				       << " Failed to open the file as OggFLAC"
				       << std::endl;
				return 0;
			}
		}

		unsigned validCommentsCnt = 0;
		std::string docData = xDoc.get_data();

		FLAC::Metadata::Iterator mdit;
		mdit.init(*flacChain);
		if(!mdit.is_valid()) return 1;

		do
		{
			::FLAC__MetadataType mdt = mdit.get_block_type();
			if (mdt != FLAC__METADATA_TYPE_VORBIS_COMMENT) continue;

			Dbg2() << __PRETTY_FUNCTION__ << " Found Vorbis Comment Block"
			       << std::endl;

			std::unique_ptr<FLAC::Metadata::Prototype> proto(mdit.get_block());
			if(!proto) continue;

			const FLAC::Metadata::VorbisComment* vc =
			        dynamic_cast<FLAC::Metadata::VorbisComment*>(proto.get());
			if(!vc || !vc->is_valid()) continue;

			unsigned numComments = vc->get_num_comments();
			for(unsigned i = 0; i < numComments; ++i)
			{
				FLAC::Metadata::VorbisComment::Entry entry =
				        vc->get_comment(i);
				if(!entry.is_valid()) continue;

				std::string tagName( entry.get_field_name(),
				                     entry.get_field_name_length() );

				/* Vorbis tags should be uppercases but not all the softwares
				 * enforce it */
				for (auto& c: tagName) c = static_cast<char>(toupper(c));

				std::string tagValue( entry.get_field_value(),
				                      entry.get_field_value_length() );

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

				Dbg2() << __PRETTY_FUNCTION__ << " Indexed " << fullComment
				       << std::endl;

				++validCommentsCnt;
			}
		}
		while(mdit.next());

		if(validCommentsCnt > 0)
		{
			Dbg1() << __PRETTY_FUNCTION__ << " Successfully indexed: " << path
			       << std::endl;

			xDoc.set_data(docData);
			return 99;
		}

		/* Altought the file appears to be a valid FLAC, no vorbis comment has
		 * been found so return less then 50 maybe it has tagged only with ID3
		 * tags ? */
		return 30;
	}

	RS_SET_CONTEXT_DEBUG_LEVEL(3)
};
