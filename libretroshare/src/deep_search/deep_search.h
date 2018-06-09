#pragma once
/*
 * RetroShare Content Search and Indexing.
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <xapian.h>

#include "retroshare/rsgxschannels.h"

struct DeepSearch
{
	//DeepSearch(const std::string& dbPath) : mDbPath(dbPath) {}

	static void search(/*query*/) { /*return all matching results*/ }


	static void indexChannelGroup(const RsGxsChannelGroup& chan)
	{
		Xapian::WritableDatabase db(mDbPath, Xapian::DB_CREATE_OR_OPEN);

		// Set up a TermGenerator that we'll use in indexing.
		Xapian::TermGenerator termgenerator;
		//termgenerator.set_stemmer(Xapian::Stem("en"));

		// We make a document and tell the term generator to use this.
		Xapian::Document doc;
		termgenerator.set_document(doc);

		// Index each field with a suitable prefix.
		termgenerator.index_text(chan.mMeta.mGroupName, 1, "G");
		termgenerator.index_text(chan.mDescription, 1, "XD");

		// Index fields without prefixes for general search.
		termgenerator.index_text(chan.mMeta.mGroupName);
		termgenerator.increase_termpos();
		termgenerator.index_text(chan.mDescription);

		// We use the identifier to ensure each object ends up in the
		// database only once no matter how many times we run the
		// indexer.
		std::string idTerm("Qretroshare://channel?id=");
		idTerm += chan.mMeta.mGroupId.toStdString();

		doc.add_boolean_term(idTerm);
		db.replace_document(idTerm, doc);
	}

	static void removeChannelFromIndex(RsGxsGroupId grpId)
	{
		std::string idTerm("Qretroshare://channel?id=");
		idTerm += grpId.toStdString();

		Xapian::WritableDatabase db(mDbPath, Xapian::DB_CREATE_OR_OPEN);
		db.delete_document(idTerm);
	}

	static void indexChannelPost(const RsGxsChannelPost& post)
	{
		Xapian::WritableDatabase db(mDbPath, Xapian::DB_CREATE_OR_OPEN);

		// Set up a TermGenerator that we'll use in indexing.
		Xapian::TermGenerator termgenerator;
		//termgenerator.set_stemmer(Xapian::Stem("en"));

		// We make a document and tell the term generator to use this.
		Xapian::Document doc;
		termgenerator.set_document(doc);

		// Index each field with a suitable prefix.
		termgenerator.index_text(post.mMeta.mMsgName, 1, "S");
		termgenerator.index_text(post.mMsg, 1, "XD");

		// Index fields without prefixes for general search.
		termgenerator.index_text(post.mMeta.mMsgName);
		termgenerator.increase_termpos();
		termgenerator.index_text(post.mMsg);

		// We use the identifier to ensure each object ends up in the
		// database only once no matter how many times we run the
		// indexer.
		std::string idTerm("Qretroshare://channel?id=");
		idTerm += post.mMeta.mGroupId.toStdString();
		idTerm += "&msgid=";
		idTerm += post.mMeta.mMsgId.toStdString();
		doc.add_boolean_term(idTerm);
		db.replace_document(idTerm, doc);
	}

	static std::string mDbPath;
};

std::string DeepSearch::mDbPath = "/tmp/deep_search_xapian_db";
