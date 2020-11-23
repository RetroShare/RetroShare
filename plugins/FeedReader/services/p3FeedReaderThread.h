/*******************************************************************************
 * plugins/FeedReader/services/p3FeedReaderThread.h                            *
 *                                                                             *
 * Copyright (C) 2012 by Thunder <retroshare.project@gmail.com>                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef P3_FEEDREADERTHREAD
#define P3_FEEDREADERTHREAD

#include "interface/rsFeedReader.h"

#include "util/rsthreads.h"
#include <list>

class p3FeedReader;
class RsFeedReaderFeed;
class RsFeedReaderMsg;
class HTMLWrapper;
class RsFeedReaderXPath;

class p3FeedReaderThread : public RsTickingThread
{
public:
	enum Type
	{
		DOWNLOAD,
		PROCESS
	};

public:
	p3FeedReaderThread(p3FeedReader *feedReader, Type type, uint32_t feedId);
	virtual ~p3FeedReaderThread();

	uint32_t getFeedId() { return mFeedId; }

	static RsFeedReaderErrorState processXPath(const std::list<std::string> &xpathsToUse, const std::list<std::string> &xpathsToRemove, std::string &description, std::string &errorString);
	static RsFeedReaderErrorState processXPath(const std::list<std::string> &xpathsToUse, const std::list<std::string> &xpathsToRemove, HTMLWrapper &html, std::string &errorString);

	static RsFeedReaderErrorState processXslt(const std::string &xslt, std::string &description, std::string &errorString);
	static RsFeedReaderErrorState processXslt(const std::string &xslt, HTMLWrapper &html, std::string &errorString);

	static RsFeedReaderErrorState processTransformation(const RsFeedReaderFeed &feed, RsFeedReaderMsg *msg, std::string &errorString);
private:
	virtual void threadTick() override; /// @see RsTickingThread

	RsFeedReaderErrorState download(const RsFeedReaderFeed &feed, std::string &content, std::string &icon, std::string &errorString);
	RsFeedReaderErrorState process(const RsFeedReaderFeed &feed, std::list<RsFeedReaderMsg*> &entries, std::string &errorString);

	std::string getProxyForFeed(const RsFeedReaderFeed &feed);
	RsFeedReaderErrorState processMsg(const RsFeedReaderFeed &feed, RsFeedReaderMsg *msg, std::string &errorString);

	p3FeedReader *mFeedReader;
	Type mType;
	uint32_t mFeedId;
};

#endif 
