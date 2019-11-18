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
#include "deep_search/commonutils.hpp"
#include "util/rsdebug.h"
#include "retroshare/rsinit.h"
#include "retroshare/rsversion.h"

#include <utility>

/*static*/ std::multimap<int, DeepFilesIndex::IndexerFunType>
DeepFilesIndex::indexersRegister = {};

bool DeepFilesIndex::indexFile(
        const std::string& path, const std::string& name,
        const RsFileHash& hash )
{
	auto dbPtr = DeepSearch::openWritableDatabase(
	            mDbPath, Xapian::DB_CREATE_OR_OPEN );
	if(!dbPtr) return false;
	Xapian::WritableDatabase& db(*dbPtr);

	const std::string hashString = hash.toStdString();
	const std::string idTerm("Q" + hashString);

	Xapian::Document oldDoc;
	Xapian::PostingIterator pIt = db.postlist_begin(idTerm);
	if( pIt != db.postlist_end(idTerm) )
	{
		oldDoc = db.get_document(*pIt);
		if( oldDoc.get_value(INDEXER_VERSION_VALUENO) ==
		        RS_HUMAN_READABLE_VERSION &&
		    std::stoull(oldDoc.get_value(INDEXERS_COUNT_VALUENO)) ==
		        indexersRegister.size() )
		{
			/* Looks like this file has already been indexed by this RetroShare
			 * exact version, so we can skip it. If the version was different it
			 * made sense to reindex it as better indexers might be available
			 * since last time it was indexed */
			Dbg3() << __PRETTY_FUNCTION__ << " skipping laready indexed file: "
			       << hash << " " << name << std::endl;
			return true;
		}
	}

	Xapian::Document doc;

	// Set up a TermGenerator that we'll use in indexing.
	Xapian::TermGenerator termgenerator;
	//termgenerator.set_stemmer(Xapian::Stem("en"));
	termgenerator.set_document(doc);

	for(auto& indexerPair : indexersRegister)
		if(indexerPair.second(path, name, termgenerator, doc) > 50)
			break;

	doc.add_boolean_term(idTerm);
	termgenerator.index_text(name, 1, "N");
	termgenerator.index_text(name);
	doc.add_value(FILE_HASH_VALUENO, hashString);
	doc.add_value(INDEXER_VERSION_VALUENO, RS_HUMAN_READABLE_VERSION);
	doc.add_value(
	            INDEXERS_COUNT_VALUENO,
	            std::to_string(indexersRegister.size()) );
	db.replace_document(idTerm, doc);

	return true;
}

bool DeepFilesIndex::removeFileFromIndex(const RsFileHash& hash)
{
	Dbg3() << __PRETTY_FUNCTION__ << " removing file from index: "
	       << hash << std::endl;

	std::unique_ptr<Xapian::WritableDatabase> db =
	        DeepSearch::openWritableDatabase(mDbPath, Xapian::DB_CREATE_OR_OPEN);
	if(!db) return false;

	db->delete_document("Q" + hash.toStdString());
	return true;
}

/*static*/ std::string DeepFilesIndex::dbDefaultPath()
{ return RsAccounts::AccountDirectory() + "/deep_files_index_xapian_db"; }

/*static*/ bool DeepFilesIndex::registerIndexer(
        int order, const DeepFilesIndex::IndexerFunType& indexerFun )
{
	Dbg1() << __PRETTY_FUNCTION__ << " " << order << std::endl;

	indexersRegister.insert(std::make_pair(order, indexerFun));
	return true;
}

uint32_t DeepFilesIndex::search(
        const std::string& queryStr,
        std::vector<DeepFilesSearchResult>& results, uint32_t maxResults )
{
	results.clear();

	auto dbPtr = DeepSearch::openReadOnlyDatabase(mDbPath);
	if(!dbPtr) return 0;
	Xapian::Database& db(*dbPtr);

	// Set up a QueryParser with a stemmer and suitable prefixes.
	Xapian::QueryParser queryparser;
	//queryparser.set_stemmer(Xapian::Stem("en"));
	queryparser.set_stemming_strategy(queryparser.STEM_SOME);
	// Start of prefix configuration.
	//queryparser.add_prefix("title", "S");
	//queryparser.add_prefix("description", "XD");
	// End of prefix configuration.

	// And parse the query.
	Xapian::Query query = queryparser.parse_query(queryStr);

	// Use an Enquire object on the database to run the query.
	Xapian::Enquire enquire(db);
	enquire.set_query(query);

	Xapian::MSet mset = enquire.get_mset(
	            0, maxResults ? maxResults : db.get_doccount() );

	for ( Xapian::MSetIterator m = mset.begin(); m != mset.end(); ++m )
	{
		const Xapian::Document& doc = m.get_document();
		DeepFilesSearchResult s;
		s.mFileHash = RsFileHash(doc.get_value(FILE_HASH_VALUENO));
		s.mWeight = m.get_weight();
#if XAPIAN_AT_LEAST(1,3,5)
		s.mSnippet = mset.snippet(doc.get_data());
#endif // XAPIAN_AT_LEAST(1,3,5)
		results.push_back(s);
	}

	return static_cast<uint32_t>(results.size());
}


#ifdef RS_DEEP_FILES_INDEX_OGG
#	include "deep_search/filesoggindexer.hpp"
static RsDeepOggFileIndexer oggFileIndexer;
#endif // def RS_DEEP_FILES_INDEX_OGG

#ifdef RS_DEEP_FILES_INDEX_FLAC
#	include "deep_search/filesflacindexer.hpp"
static RsDeepFlacFileIndexer flacFileIndexer;
#endif // def RS_DEEP_FILES_INDEX_FLAC

#ifdef RS_DEEP_FILES_INDEX_TAGLIB
#	include "deep_search/filestaglibindexer.hpp"
static RsDeepTaglibFileIndexer taglibFileIndexer;
#endif // def RS_DEEP_FILES_INDEX_TAGLIB
