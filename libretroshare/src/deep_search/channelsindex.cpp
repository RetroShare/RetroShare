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

#include "deep_search/channelsindex.hpp"
#include "deep_search/commonutils.hpp"

uint32_t DeepChannelsIndex::search(
        const std::string& queryStr,
        std::vector<DeepChannelsSearchResult>& results, uint32_t maxResults )
{
	results.clear();

	std::unique_ptr<Xapian::Database> dbPtr(
	            DeepSearch::openReadOnlyDatabase(dbPath()) );
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
		DeepChannelsSearchResult s;
		s.mUrl = doc.get_value(URL_VALUENO);
#if XAPIAN_AT_LEAST(1,3,5)
		s.mSnippet = mset.snippet(doc.get_data());
#endif // XAPIAN_AT_LEAST(1,3,5)
		results.push_back(s);
	}

	return static_cast<uint32_t>(results.size());
}

void DeepChannelsIndex::indexChannelGroup(const RsGxsChannelGroup& chan)
{
	std::unique_ptr<Xapian::WritableDatabase> dbPtr(
	            DeepSearch::openWritableDatabase(
	                dbPath(), Xapian::DB_CREATE_OR_OPEN ) );
	if(!dbPtr) return;

	Xapian::WritableDatabase& db(*dbPtr);

	// Set up a TermGenerator that we'll use in indexing.
	Xapian::TermGenerator termgenerator;
	//termgenerator.set_stemmer(Xapian::Stem("en"));

	// We make a document and tell the term generator to use this.
	Xapian::Document doc;
	termgenerator.set_document(doc);

	// Index each field with a suitable prefix.
	termgenerator.index_text(chan.mMeta.mGroupName, 1, "G");
	termgenerator.index_text(
	            DeepSearch::timetToXapianDate(chan.mMeta.mPublishTs), 1, "D" );
	termgenerator.index_text(chan.mDescription, 1, "XD");

	// Index fields without prefixes for general search.
	termgenerator.index_text(chan.mMeta.mGroupName);
	termgenerator.increase_termpos();
	termgenerator.index_text(chan.mDescription);

	RsUrl chanUrl; chanUrl
	        .setScheme("retroshare").setPath("/channel")
	        .setQueryKV("id", chan.mMeta.mGroupId.toStdString());
	const std::string idTerm("Q" + chanUrl.toString());

	chanUrl.setQueryKV("publishTs", std::to_string(chan.mMeta.mPublishTs));
	chanUrl.setQueryKV("name", chan.mMeta.mGroupName);
	if(!chan.mMeta.mAuthorId.isNull())
		chanUrl.setQueryKV("authorId", chan.mMeta.mAuthorId.toStdString());
	if(chan.mMeta.mSignFlags)
		chanUrl.setQueryKV( "signFlags",
		                    std::to_string(chan.mMeta.mSignFlags) );
	std::string rsLink(chanUrl.toString());

	// store the RS link so we are able to retrive it on matching search
	doc.add_value(URL_VALUENO, rsLink);

	// Store some fields for display purposes.
	doc.set_data(chan.mMeta.mGroupName + "\n" + chan.mDescription);

	// We use the identifier to ensure each object ends up in the
	// database only once no matter how many times we run the
	// indexer. "Q" prefix is a Xapian convention for unique id term.
	doc.add_boolean_term(idTerm);
	db.replace_document(idTerm, doc);
}

void DeepChannelsIndex::removeChannelFromIndex(RsGxsGroupId grpId)
{
	// "Q" prefix is a Xapian convention for unique id term.
	RsUrl chanUrl; chanUrl
	        .setScheme("retroshare").setPath("/channel")
	        .setQueryKV("id", grpId.toStdString());
	std::string idTerm("Q" + chanUrl.toString());

	std::unique_ptr<Xapian::WritableDatabase> dbPtr(
	            DeepSearch::openWritableDatabase(
	                dbPath(), Xapian::DB_CREATE_OR_OPEN ) );
	if(!dbPtr) return;

	Xapian::WritableDatabase& db(*dbPtr);
	db.delete_document(idTerm);
}

void DeepChannelsIndex::indexChannelPost(const RsGxsChannelPost& post)
{
	std::unique_ptr<Xapian::WritableDatabase> dbPtr(
	            DeepSearch::openWritableDatabase(
	                dbPath(), Xapian::DB_CREATE_OR_OPEN ) );
	if(!dbPtr) return;

	Xapian::WritableDatabase& db(*dbPtr);

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

	// TODO: we should strip out HTML tags instead of skipping indexing
	// Avoid indexing HTML
	bool isPlainMsg =
	        post.mMsg[0] != '<' || post.mMsg[post.mMsg.size() - 1] != '>';

	if(isPlainMsg)
		termgenerator.index_text(post.mMsg, 1, "XD");

	// Index fields without prefixes for general search.
	termgenerator.index_text(post.mMeta.mMsgName);
	if(isPlainMsg)
	{
		termgenerator.increase_termpos();
		termgenerator.index_text(post.mMsg);
	}

	for(const RsGxsFile& attachment : post.mFiles)
	{
		termgenerator.index_text(attachment.mName, 1, "F");

		termgenerator.increase_termpos();
		termgenerator.index_text(attachment.mName);
	}

	// We use the identifier to ensure each object ends up in the
	// database only once no matter how many times we run the
	// indexer.
	RsUrl postUrl; postUrl
	        .setScheme("retroshare").setPath("/channel")
	        .setQueryKV("id", post.mMeta.mGroupId.toStdString())
	        .setQueryKV("msgid", post.mMeta.mMsgId.toStdString());
	std::string idTerm("Q" + postUrl.toString());

	postUrl.setQueryKV("publishTs", std::to_string(post.mMeta.mPublishTs));
	postUrl.setQueryKV("name", post.mMeta.mMsgName);
	postUrl.setQueryKV("authorId", post.mMeta.mAuthorId.toStdString());
	std::string rsLink(postUrl.toString());

	// store the RS link so we are able to retrive it on matching search
	doc.add_value(URL_VALUENO, rsLink);

	// Store some fields for display purposes.
	if(isPlainMsg)
		doc.set_data(post.mMeta.mMsgName + "\n" + post.mMsg);
	else doc.set_data(post.mMeta.mMsgName);

	doc.add_boolean_term(idTerm);
	db.replace_document(idTerm, doc);
}

void DeepChannelsIndex::removeChannelPostFromIndex(
        RsGxsGroupId grpId, RsGxsMessageId msgId )
{
	RsUrl postUrl; postUrl
	        .setScheme("retroshare").setPath("/channel")
	        .setQueryKV("id", grpId.toStdString())
	        .setQueryKV("msgid", msgId.toStdString());
	// "Q" prefix is a Xapian convention for unique id term.
	std::string idTerm("Q" + postUrl.toString());

	std::unique_ptr<Xapian::WritableDatabase> dbPtr(
	            DeepSearch::openWritableDatabase(
	                dbPath(), Xapian::DB_CREATE_OR_OPEN ) );
	if(!dbPtr) return;

	Xapian::WritableDatabase& db(*dbPtr);
	db.delete_document(idTerm);
}
