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

#include "deep_search/forumsindex.hpp"
#include "deep_search/commonutils.hpp"
#include "retroshare/rsinit.h"
#include "retroshare/rsgxsforums.h"
#include "util/rsdebuglevel4.h"

std::error_condition DeepForumsIndex::search(
        const std::string& queryStr,
        std::vector<DeepForumsSearchResult>& results, uint32_t maxResults )
{
	results.clear();

	std::unique_ptr<Xapian::Database> dbPtr(
	            DeepSearch::openReadOnlyDatabase(mDbPath) );
	if(!dbPtr) return std::errc::bad_file_descriptor;

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

	for( Xapian::MSetIterator m = mset.begin(); m != mset.end(); ++m )
	{
		const Xapian::Document& doc = m.get_document();
		DeepForumsSearchResult s;
		s.mUrl = doc.get_value(URL_VALUENO);
#if XAPIAN_AT_LEAST(1,3,5)
		s.mSnippet = mset.snippet(doc.get_data());
#endif // XAPIAN_AT_LEAST(1,3,5)
		results.push_back(s);
	}

	return std::error_condition();
}

/*static*/ std::string DeepForumsIndex::forumIndexId(const RsGxsGroupId& grpId)
{
	RsUrl forumIndexId(RsGxsForums::DEFAULT_FORUM_BASE_URL);
	forumIndexId.setQueryKV(
	            RsGxsForums::FORUM_URL_ID_FIELD, grpId.toStdString() );
	return forumIndexId.toString();
}

/*static*/ std::string DeepForumsIndex::postIndexId(
        const RsGxsGroupId& grpId, const RsGxsMessageId& msgId )
{
	RsUrl postIndexId(RsGxsForums::DEFAULT_FORUM_BASE_URL);
	postIndexId.setQueryKV(RsGxsForums::FORUM_URL_ID_FIELD, grpId.toStdString());
	postIndexId.setQueryKV(RsGxsForums::FORUM_URL_MSG_ID_FIELD, msgId.toStdString());
	return postIndexId.toString();
}

std::error_condition DeepForumsIndex::indexForumGroup(
        const RsGxsForumGroup& forum )
{
	// Set up a TermGenerator that we'll use in indexing.
	Xapian::TermGenerator termgenerator;
	//termgenerator.set_stemmer(Xapian::Stem("en"));

	// We make a document and tell the term generator to use this.
	Xapian::Document doc;
	termgenerator.set_document(doc);

	// Index each field with a suitable prefix.
	termgenerator.index_text(forum.mMeta.mGroupName, 1, "G");
	termgenerator.index_text(
	            DeepSearch::timetToXapianDate(forum.mMeta.mPublishTs), 1, "D" );
	termgenerator.index_text(forum.mDescription, 1, "XD");

	// Index fields without prefixes for general search.
	termgenerator.index_text(forum.mMeta.mGroupName);
	termgenerator.increase_termpos();
	termgenerator.index_text(forum.mDescription);

	// store the RS link so we are able to retrive it on matching search
	const std::string rsLink(forumIndexId(forum.mMeta.mGroupId));
	doc.add_value(URL_VALUENO, rsLink);

	/* Store some fields for display purposes. Retrieved later to provide the
	 * matching snippet on search */
	doc.set_data(forum.mMeta.mGroupName + "\n" + forum.mDescription);

	/* We use the identifier to ensure each object ends up in the database only
	 * once no matter how many times we run the indexer.
	 * "Q" prefix is a Xapian convention for unique id term. */
	const std::string idTerm("Q" + rsLink);
	doc.add_boolean_term(idTerm);

	mWriteQueue.push([idTerm, doc](Xapian::WritableDatabase& db)
	{ db.replace_document(idTerm, doc); } );

	return std::error_condition();
}

std::error_condition DeepForumsIndex::removeForumFromIndex(
        const RsGxsGroupId& grpId )
{
	mWriteQueue.push([grpId](Xapian::WritableDatabase& db)
	{ db.delete_document("Q" + forumIndexId(grpId)); });

	return std::error_condition();
}

std::error_condition DeepForumsIndex::indexForumPost(const RsGxsForumMsg& post)
{
	RS_DBG4(post);

	const auto& groupId = post.mMeta.mGroupId;
	const auto& msgId = post.mMeta.mMsgId;

	if(groupId.isNull() || msgId.isNull())
	{
		RS_ERR("Got post with invalid id ", post);
		print_stacktrace();
		return std::errc::invalid_argument;
	}

	// Set up a TermGenerator that we'll use in indexing.
	Xapian::TermGenerator termgenerator;
	//termgenerator.set_stemmer(Xapian::Stem("en"));

	// We make a document and tell the term generator to use this.
	Xapian::Document doc;
	termgenerator.set_document(doc);

	// Index each field with a suitable prefix.
	termgenerator.index_text(post.mMeta.mMsgName, 1, "S");
	termgenerator.index_text(
	            DeepSearch::timetToXapianDate(post.mMeta.mPublishTs), 1, "D" );

	// Avoid indexing RetroShare-gui HTML tags
	const std::string cleanMsg = DeepSearch::simpleTextHtmlExtract(post.mMsg);
	termgenerator.index_text(cleanMsg, 1, "XD" );

	// Index fields without prefixes for general search.
	termgenerator.index_text(post.mMeta.mMsgName);

	termgenerator.increase_termpos();
	termgenerator.index_text(cleanMsg);
	// store the RS link so we are able to retrive it on matching search
	const std::string rsLink(postIndexId(groupId, msgId));
	doc.add_value(URL_VALUENO, rsLink);

	// Store some fields for display purposes.
	doc.set_data(post.mMeta.mMsgName + "\n" + cleanMsg);

	// We use the identifier to ensure each object ends up in the
	// database only once no matter how many times we run the
	// indexer.
	const std::string idTerm("Q" + rsLink);
	doc.add_boolean_term(idTerm);

	mWriteQueue.push( [idTerm, doc](Xapian::WritableDatabase& db)
	{ db.replace_document(idTerm, doc); } );


	return std::error_condition();
}

std::error_condition DeepForumsIndex::removeForumPostFromIndex(
        RsGxsGroupId grpId, RsGxsMessageId msgId )
{
	// "Q" prefix is a Xapian convention for unique id term.
	std::string idTerm("Q" + postIndexId(grpId, msgId));
	mWriteQueue.push( [idTerm](Xapian::WritableDatabase& db)
	{ db.delete_document(idTerm); } );

	return std::error_condition();
}

/*static*/ std::string DeepForumsIndex::dbDefaultPath()
{ return RsAccounts::AccountDirectory() + "/deep_forum_index_xapian_db"; }
