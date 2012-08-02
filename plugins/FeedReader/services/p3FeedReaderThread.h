/****************************************************************
 *  RetroShare GUI is distributed under the following license:
 *
 *  Copyright (C) 2012 by Thunder
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

 #ifndef P3_FEEDREADERTHREAD
#define P3_FEEDREADERTHREAD

#include "util/rsthreads.h"
#include <list>

class p3FeedReader;
class RsFeedReaderFeed;
class RsFeedReaderMsg;

class p3FeedReaderThread : public RsThread
{
public:
	enum Type
	{
		DOWNLOAD,
		PROCESS
	};
	enum DownloadResult
	{
		DOWNLOAD_SUCCESS,
		DOWNLOAD_ERROR_INIT,
		DOWNLOAD_ERROR,
		DOWNLOAD_UNKNOWN_CONTENT_TYPE,
		DOWNLOAD_NOT_FOUND,
		DOWNLOAD_UNKOWN_RESPONSE_CODE,
		DOWNLOAD_INTERNAL_ERROR
	};
	enum ProcessResult
	{
		PROCESS_SUCCESS,
		PROCESS_ERROR_INIT,
		PROCESS_UNKNOWN_FORMAT
	};

public:
	p3FeedReaderThread(p3FeedReader *feedReader, Type type);

private:
	virtual void run();

	DownloadResult download(const RsFeedReaderFeed &feed, std::string &content, std::string &icon, std::string &error);
	ProcessResult process(const RsFeedReaderFeed &feed, std::list<RsFeedReaderMsg*> &entries, std::string &error);

	p3FeedReader *mFeedReader;
	Type mType;
	/*xmlCharEncodingHandlerPtr*/ void *mCharEncodingHandler;
};

#endif 
