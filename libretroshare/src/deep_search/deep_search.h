/*******************************************************************************
 * libretroshare/src/crypto: crypto.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include "util/rstime.h"
#include <vector>
#include <xapian.h>

#include "retroshare/rsgxschannels.h"
#include "retroshare/rsinit.h"
#include "util/rsurl.h"

#ifndef XAPIAN_AT_LEAST
#define XAPIAN_AT_LEAST(A,B,C) (XAPIAN_MAJOR_VERSION > (A) || \
	(XAPIAN_MAJOR_VERSION == (A) && \
	(XAPIAN_MINOR_VERSION > (B) || \
	(XAPIAN_MINOR_VERSION == (B) && XAPIAN_REVISION >= (C)))))
#endif // ndef XAPIAN_AT_LEAST

struct DeepSearch
{
	struct SearchResult
	{
		std::string mUrl;
		std::string mSnippet;
	};

	/**
	 * @param[in] maxResults maximum number of acceptable search results, 0 for
	 * no limits
	 * @return search results count
	 */
	static uint32_t search( const std::string& queryStr,
	                        std::vector<SearchResult>& results,
	                        uint32_t maxResults = 100 )
	{
		results.clear();

		Xapian::Database db;

		// Open the database we're going to search.
		try { db = Xapian::Database(dbPath()); }
		catch(Xapian::DatabaseOpeningError e)
		{
			std::cerr << __PRETTY_FUNCTION__ << " " << e.get_msg()
			          << ", probably nothing has been indexed yet."<< std::endl;
			return 0;
		}
		catch(Xapian::DatabaseError e)
		{
			std::cerr << __PRETTY_FUNCTION__ << " " << e.get_msg()
			          << " this is fishy, maybe " << dbPath()
			          << " has been corrupted (deleting it may help in that "
			          << "case without loosing data)" << std::endl;
			return 0;
		}

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
			SearchResult s;
			s.mUrl = doc.get_value(URL_VALUENO);
#if XAPIAN_AT_LEAST(1,3,5)
			s.mSnippet = mset.snippet(doc.get_data());
#endif // XAPIAN_AT_LEAST(1,3,5)
			results.push_back(s);
		}

		return results.size();
	}


	static void indexChannelGroup(const RsGxsChannelGroup& chan)
	{
		Xapian::WritableDatabase db(dbPath(), Xapian::DB_CREATE_OR_OPEN);

		// Set up a TermGenerator that we'll use in indexing.
		Xapian::TermGenerator termgenerator;
		//termgenerator.set_stemmer(Xapian::Stem("en"));

		// We make a document and tell the term generator to use this.
		Xapian::Document doc;
		termgenerator.set_document(doc);

		// Index each field with a suitable prefix.
		termgenerator.index_text(chan.mMeta.mGroupName, 1, "G");
		termgenerator.index_text(timetToXapianDate(chan.mMeta.mPublishTs), 1, "D");
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

	static void removeChannelFromIndex(RsGxsGroupId grpId)
	{
		// "Q" prefix is a Xapian convention for unique id term.
		RsUrl chanUrl; chanUrl
		        .setScheme("retroshare").setPath("/channel")
		        .setQueryKV("id", grpId.toStdString());
		std::string idTerm("Q" + chanUrl.toString());

		Xapian::WritableDatabase db(dbPath(), Xapian::DB_CREATE_OR_OPEN);
		db.delete_document(idTerm);
	}

	static void indexChannelPost(const RsGxsChannelPost& post)
	{
		Xapian::WritableDatabase db(dbPath(), Xapian::DB_CREATE_OR_OPEN);

		// Set up a TermGenerator that we'll use in indexing.
		Xapian::TermGenerator termgenerator;
		//termgenerator.set_stemmer(Xapian::Stem("en"));

		// We make a document and tell the term generator to use this.
		Xapian::Document doc;
		termgenerator.set_document(doc);

		// Index each field with a suitable prefix.
		termgenerator.index_text(post.mMeta.mMsgName, 1, "S");
		termgenerator.index_text(timetToXapianDate(post.mMeta.mPublishTs), 1, "D");

		// Avoid indexing HTML
		bool isPlainMsg = post.mMsg[0] != '<' || post.mMsg[post.mMsg.size() - 1] != '>';

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

	static void removeChannelPostFromIndex(
	        RsGxsGroupId grpId, RsGxsMessageId msgId )
	{
		RsUrl postUrl; postUrl
		        .setScheme("retroshare").setPath("/channel")
		        .setQueryKV("id", grpId.toStdString())
		        .setQueryKV("msgid", msgId.toStdString());
		// "Q" prefix is a Xapian convention for unique id term.
		std::string idTerm("Q" + postUrl.toString());

		Xapian::WritableDatabase db(dbPath(), Xapian::DB_CREATE_OR_OPEN);
		db.delete_document(idTerm);
	}

private:

	enum : Xapian::valueno
	{
		/// Used to store retroshare url of indexed documents
		URL_VALUENO,

		/// @see Xapian::BAD_VALUENO
		BAD_VALUENO = Xapian::BAD_VALUENO
	};

	static const std::string& dbPath()
	{
		static const std::string dbDir =
		        RsAccounts::AccountDirectory() + "/deep_search_xapian_db";
		return dbDir;
	}

	static std::string timetToXapianDate(const rstime_t& time)
	{
		char date[] = "YYYYMMDD\0";
		time_t tTime = static_cast<time_t>(time);
		std::strftime(date, 9, "%Y%m%d", std::gmtime(&tTime));
		return date;
	}
};

