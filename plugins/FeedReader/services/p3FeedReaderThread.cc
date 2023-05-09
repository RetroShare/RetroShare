/*******************************************************************************
 * plugins/FeedReader/services/p3FeedReaderThread.cc                           *
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

#include "p3FeedReaderThread.h"
#include "rsFeedReaderItems.h"
#include "util/rsstring.h"
#include "util/rstime.h"
#include "util/CURLWrapper.h"
#include "util/XMLWrapper.h"
#include "util/HTMLWrapper.h"
#include "util/XPathWrapper.h"

#include <openssl/evp.h>
#include <unistd.h> // for usleep

enum FeedFormat { FORMAT_RSS, FORMAT_RDF, FORMAT_ATOM };

/*********
 * #define FEEDREADER_DEBUG
 *********/

p3FeedReaderThread::p3FeedReaderThread(p3FeedReader *feedReader, Type type, uint32_t feedId) :
    RsTickingThread(), mFeedReader(feedReader), mType(type), mFeedId(feedId)
{
}

p3FeedReaderThread::~p3FeedReaderThread()
{
}

/***************************************************************************/
/****************************** Thread *************************************/
/***************************************************************************/

void p3FeedReaderThread::threadTick()
{
		rstime::rs_usleep(1000000);

		/* every second */

		switch (mType) {
		case DOWNLOAD:
			{
				RsFeedReaderFeed feed;
				if (mFeedReader->getFeedToDownload(feed, mFeedId)) {
					std::string content;
					std::string icon;
					std::string errorString;

					RsFeedReaderErrorState result = download(feed, content, icon, errorString);
					if (result == RS_FEED_ERRORSTATE_OK) {
						/* trim */
						XMLWrapper::trimString(content);

						mFeedReader->onDownloadSuccess(feed.feedId, content, icon);
					} else {
						mFeedReader->onDownloadError(feed.feedId, result, errorString);
					}
				}
			}
			break;
		case PROCESS:
			{
				RsFeedReaderFeed feed;
				if (mFeedReader->getFeedToProcess(feed, mFeedId)) {
					std::list<RsFeedReaderMsg*> msgs;
					std::string errorString;
					std::list<RsFeedReaderMsg*>::iterator it;

					RsFeedReaderErrorState result = process(feed, msgs, errorString);
					if (result == RS_FEED_ERRORSTATE_OK) {
						/* first, filter the messages */
						mFeedReader->onProcessSuccess_filterMsg(feed.feedId, msgs);
						if (isRunning()) {
							/* second, process the descriptions and attachment */
							for (it = msgs.begin(); it != msgs.end(); ) {
								if (!isRunning()) {
									break;
								}

								RsFeedReaderMsg *mi = *it;
								result = processMsg(feed, mi, errorString);
								if (result != RS_FEED_ERRORSTATE_OK) {
									break;
								}

								if (feed.preview) {
									/* add every message */
									it = msgs.erase(it);

									std::list<RsFeedReaderMsg*> msgSingle;
									msgSingle.push_back(mi);
									mFeedReader->onProcessSuccess_addMsgs(feed.feedId, msgSingle);

									/* delete not accepted message */
									std::list<RsFeedReaderMsg*>::iterator it1;
									for (it1 = msgSingle.begin(); it1 != msgSingle.end(); ++it1) {
										delete (*it1);
									}
								} else {
									result = processTransformation(feed, mi, errorString);
									if (result != RS_FEED_ERRORSTATE_OK) {
										break;
									}
									++it;
								}
							}

							if (!feed.preview) {
								if (isRunning()) {
									if (result == RS_FEED_ERRORSTATE_OK) {
										/* third, add messages */
										mFeedReader->onProcessSuccess_addMsgs(feed.feedId, msgs);
									} else {
										mFeedReader->onProcessError(feed.feedId, result, errorString);
									}
								}
							}
						}
					} else {
						mFeedReader->onProcessError(feed.feedId, result, errorString);
					}

					/* delete not accepted messages */
					for (it = msgs.begin(); it != msgs.end(); ++it) {
						delete (*it);
					}
					msgs.clear();
				}
			}
			break;
		}
}

/***************************************************************************/
/****************************** Download ***********************************/
/***************************************************************************/

bool p3FeedReaderThread::isContentType(const std::string &contentType, const char *type)
{
	return (strncasecmp(contentType.c_str(), type, strlen(type)) == 0);
}

bool p3FeedReaderThread::toBase64(const std::vector<unsigned char> &data, std::string &base64)
{
	bool result = false;

	/* Set up a base64 encoding BIO that writes to a memory BIO */
	BIO *b64 = BIO_new(BIO_f_base64());
	if (b64) {
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
		BIO *bmem = BIO_new(BIO_s_mem());
		if (bmem) {
			BIO_set_flags(bmem, BIO_CLOSE);  // probably redundant
			b64 = BIO_push(b64, bmem);
			/* Send the data */
			BIO_write(b64, data.data(), data.size());
			/* Collect the encoded data */
			BIO_flush(b64);
			char* temp;
			int count = BIO_get_mem_data(bmem, &temp);
			if (count && temp) {
				base64.assign(temp, count);
				result = true;
			}
		}
		BIO_free_all(b64);
	}

	return result;
}

bool p3FeedReaderThread::fromBase64(const std::string &base64, std::vector<unsigned char> &data)
{
	bool result = false;

	BIO *b64 = BIO_new(BIO_f_base64());
	if (b64) {
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
		BIO *source = BIO_new_mem_buf(base64.c_str(), -1); // read-only source
		if (source) {
			BIO_push(b64, source);
			const int maxlen = base64.length() / 4 * 3 + 1;
			data.resize(maxlen);
			const int len = BIO_read(b64, data.data(), maxlen);
			data.resize(len);
			result = true;
		}
		BIO_free_all(b64);
	}

	return result;
}

static std::string getBaseLink(std::string link)
{
	size_t found = link.rfind('/');
	if (found != std::string::npos) {
		link.erase(found + 1);
	}

	return link;
}

static std::string calculateLink(const std::string &baseLink, const std::string &link)
{
	if (link.substr(0, 7) == "http://" || link.substr(0, 8) == "https://") {
		/* absolute link */
		return link;
	}

	/* calculate link of base link */
	std::string resultLink = baseLink;

	int hostStart = 0;
	/* link should begin with "http://" or "https://" */
	if (resultLink.substr(0, 7) == "http://") {
		hostStart = 7;
	} else if (resultLink.substr(0, 8) == "https://") {
		hostStart = 8;
	} else {
		hostStart = 7;
		resultLink.insert(0, "http://");
	}

	if (link.empty()) {
		/* no link */
		return resultLink;
	}

	if (*link.begin() == '/') {
		/* link begins with "/" */
		size_t found = resultLink.find('/', hostStart);
		if (found != std::string::npos) {
			resultLink.erase(found);
		}
	} else {
		/* check for "/" at the end */
		std::string::reverse_iterator it = resultLink.rend ();
		it--;
		if (*it != '/') {
			resultLink += "/";
		}
	}

	resultLink += link;

	return resultLink;
}

static bool getFavicon(CURLWrapper &CURL, const std::string &url, std::string &icon)
{
	icon.clear();

	bool result = false;

	std::vector<unsigned char> vicon;
	CURLcode code = CURL.downloadBinary(calculateLink(url, "/favicon.ico"), vicon);
	if (code == CURLE_OK) {
		if (CURL.responseCode() == 200) {
			std::string contentType = CURL.contentType();
			if (p3FeedReaderThread::isContentType(contentType, "image/") ||
				p3FeedReaderThread::isContentType(contentType, "application/octet-stream") ||
				p3FeedReaderThread::isContentType(contentType, "text/plain")) {
				if (!vicon.empty()) {
#warning p3FeedReaderThread.cc TODO thunder2: check it
					result = p3FeedReaderThread::toBase64(vicon, icon);
				}
			}
		}
	}

	return result;
}

RsFeedReaderErrorState p3FeedReaderThread::download(const RsFeedReaderFeed &feed, std::string &content, std::string &icon, std::string &errorString)
{
#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReaderThread::download - feed " << feed.feedId << " (" << feed.name << ")" << std::endl;
#endif

	content.clear();
	errorString.clear();

	RsFeedReaderErrorState result;

	std::string proxy = getProxyForFeed(feed);
	CURLWrapper CURL(proxy);
	CURLcode code = CURL.downloadText(feed.url, content);

	if (code == CURLE_OK) {
		long responseCode = CURL.responseCode();

		switch (responseCode) {
		case 200:
			{
				std::string contentType = CURL.contentType();

				if (isContentType(contentType, "text/xml") ||
					isContentType(contentType, "text/html") ||
					isContentType(contentType, "application/rss+xml") ||
					isContentType(contentType, "application/xml") ||
					isContentType(contentType, "application/xhtml+xml") ||
					isContentType(contentType, "application/atom+xml")) {
					/* ok */
					result = RS_FEED_ERRORSTATE_OK;
				} else {
					result = RS_FEED_ERRORSTATE_DOWNLOAD_UNKNOWN_CONTENT_TYPE;
					errorString = contentType;
				}
			}
			break;
		case 403:
			result = RS_FEED_ERRORSTATE_DOWNLOAD_BLOCKED;
			break;
		case 404:
			result = RS_FEED_ERRORSTATE_DOWNLOAD_NOT_FOUND;
			break;
		default:
			result = RS_FEED_ERRORSTATE_DOWNLOAD_UNKOWN_RESPONSE_CODE;
			rs_sprintf(errorString, "%ld", responseCode);
		}

		getFavicon(CURL, feed.url, icon);
	} else {
		result = RS_FEED_ERRORSTATE_DOWNLOAD_ERROR;
		errorString = curl_easy_strerror(code);
	}

#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReaderThread::download - feed " << feed.feedId << " (" << feed.name << "), result " << result << ", error = " << errorString << std::endl;
#endif

	return result;
}

/***************************************************************************/
/****************************** Process ************************************/
/***************************************************************************/

static xmlNodePtr getNextItem(FeedFormat feedFormat, xmlNodePtr channel, xmlNodePtr item)
{
	if (!channel) {
		return NULL;
	}

	if (!item) {
		switch (feedFormat) {
		case FORMAT_RSS:
		case FORMAT_ATOM:
				item = channel->children;
				break;
		case FORMAT_RDF:
				item = channel->next;
				break;
		default:
				return NULL;
		}
	} else {
		item = item->next;
	}
	for (; item; item = item->next) {
		if (item->type == XML_ELEMENT_NODE && xmlStrcasecmp(item->name, (feedFormat == FORMAT_ATOM) ? BAD_CAST"entry" : BAD_CAST"item") == 0) {
			break;
		}
	}

	return item;
}

static void splitString(std::string s, std::vector<std::string> &v, const char d)
{
	v.clear();

	std::string::size_type p;
	while ((p = s.find_first_of(d)) != std::string::npos) {
		v.push_back(s.substr(0, p));
		s.erase(0, p + 1);
	}
	if (!s.empty()) {
		v.push_back(s);
	}
}

static unsigned int ymdhms_to_seconds(int year, int mon, int day, int hour, int minute, int second)
{
	if (sizeof(time_t) == 4)
	{
		if ((time_t)-1 < 0)
		{
			if (year >= 2038)
			{
				year = 2038;
				mon = 0;
				day = 1;
				hour = 0;
				minute = 0;
				second = 0;
			}
		}
		else
		{
			if (year >= 2115)
			{
				year = 2115;
				mon = 0;
				day = 1;
				hour = 0;
				minute = 0;
				second = 0;
			}
		}
	}

	unsigned int ret = (day - 32075)       /* days */
			+ 1461L * (year + 4800L + (mon - 14) / 12) / 4
			+ 367 * (mon - 2 - (mon - 14) / 12 * 12) / 12
			- 3 * ((year + 4900L + (mon - 14) / 12) / 100) / 4
			- 2440588;
	ret = 24*ret + hour;     /* hours   */
	ret = 60*ret + minute;   /* minutes */
	ret = 60*ret + second;   /* seconds */

	return ret;
}

static const char haystack[37]="janfebmaraprmayjunjulaugsepoctnovdec";

// we follow the recommendation of rfc2822 to consider all
// obsolete time zones not listed here equivalent to "-0000"
static const struct {
	const char tzName[4];
	int tzOffset;
} known_zones[] = {
	{ "UT", 0 },
	{ "GMT", 0 },
	{ "EST", -300 },
	{ "EDT", -240 },
	{ "CST", -360 },
	{ "CDT", -300 },
	{ "MST", -420 },
	{ "MDT", -360 },
	{ "PST", -480 },
	{ "PDT", -420 },
	{ { 0,0,0,0 }, 0 }
};

// copied from KRFCDate::parseDate
static time_t parseRFC822Date(const std::string &pubDate)
{
	if (pubDate.empty())
		return 0;

	// This parse a date in the form:
	//     Wednesday, 09-Nov-99 23:12:40 GMT
	// or
	//     Sat, 01-Jan-2000 08:00:00 GMT
	// or
	//     Sat, 01 Jan 2000 08:00:00 GMT
	// or
	//     01 Jan 99 22:00 +0100    (exceptions in rfc822/rfc2822)
	//
	// We ignore the weekday
	//
	time_t result = 0;
	int offset = 0;
	char *newPosStr;
	const char *dateString = pubDate.c_str();
	int day = 0;
	char monthStr[4];
	int month = -1;
	int year = 0;
	int hour = 0;
	int minute = 0;
	int second = 0;

	// Strip leading space
	while(*dateString && isspace(*dateString))
		dateString++;

	// Strip weekday
	while(*dateString && !isdigit(*dateString) && !isspace(*dateString))
		dateString++;

	// Strip trailing space
	while(*dateString && isspace(*dateString))
		dateString++;

	if (!*dateString)
		return result;  // Invalid date

	if (isalpha(*dateString))
	{
		// ' Nov 5 1994 18:15:30 GMT'
		// Strip leading space
		while(*dateString && isspace(*dateString))
			dateString++;

		for(int i=0; i < 3;i++)
		{
			if (!*dateString || (*dateString == '-') || isspace(*dateString))
				return result;  // Invalid date
			monthStr[i] = tolower(*dateString++);
		}
		monthStr[3] = '\0';

		newPosStr = (char*)strstr(haystack, monthStr);

		if (!newPosStr)
			return result;  // Invalid date

		month = (newPosStr-haystack)/3; // Jan=00, Feb=01, Mar=02, ..

		if ((month < 0) || (month > 11))
			return result;  // Invalid date

		while (*dateString && isalpha(*dateString))
			dateString++; // Skip rest of month-name
	}

	// ' 09-Nov-99 23:12:40 GMT'
	// ' 5 1994 18:15:30 GMT'
	day = strtol(dateString, &newPosStr, 10);
	dateString = newPosStr;

	if ((day < 1) || (day > 31))
		return result; // Invalid date;

	if (!*dateString)
		return result;  // Invalid date

	while(*dateString && (isspace(*dateString) || (*dateString == '-')))
		dateString++;

	if (month == -1)
	{
		for(int i=0; i < 3;i++)
		{
			if (!*dateString || (*dateString == '-') || isspace(*dateString))
				return result;  // Invalid date
			monthStr[i] = tolower(*dateString++);
		}
		monthStr[3] = '\0';

		newPosStr = (char*)strstr(haystack, monthStr);

		if (!newPosStr)
			return result;  // Invalid date

		month = (newPosStr-haystack)/3; // Jan=00, Feb=01, Mar=02, ..

		if ((month < 0) || (month > 11))
			return result;  // Invalid date

		while (*dateString && isalpha(*dateString))
			dateString++; // Skip rest of month-name

	}

	// '-99 23:12:40 GMT'
	while(*dateString && (isspace(*dateString) || (*dateString == '-')))
		dateString++;

	if (!*dateString || !isdigit(*dateString))
		return result;  // Invalid date

	// '99 23:12:40 GMT'
	year = strtol(dateString, &newPosStr, 10);
	dateString = newPosStr;

	// Y2K: Solve 2 digit years
	if ((year >= 0) && (year < 50))
		year += 2000;

	if ((year >= 50) && (year < 100))
		year += 1900;  // Y2K

	if ((year < 1900) || (year > 2500))
		return result; // Invalid date

	// Don't fail if the time is missing.
	if (*dateString)
	{
		// ' 23:12:40 GMT'
		if (!isspace(*dateString++))
			return result;  // Invalid date

		hour = strtol(dateString, &newPosStr, 10);
		dateString = newPosStr;

		if ((hour < 0) || (hour > 23))
			return result; // Invalid date

		if (!*dateString)
			return result;  // Invalid date

		// ':12:40 GMT'
		if (*dateString++ != ':')
			return result;  // Invalid date

		minute = strtol(dateString, &newPosStr, 10);
		dateString = newPosStr;

		if ((minute < 0) || (minute > 59))
			return result; // Invalid date

		if (!*dateString)
			return result;  // Invalid date

		// ':40 GMT'
		if (*dateString != ':' && !isspace(*dateString))
			return result;  // Invalid date

		// seconds are optional in rfc822 + rfc2822
		if (*dateString ==':') {
			dateString++;

			second = strtol(dateString, &newPosStr, 10);
			dateString = newPosStr;

			if ((second < 0) || (second > 59))
				return result; // Invalid date
		} else {
			dateString++;
		}

		while(*dateString && isspace(*dateString))
			dateString++;
	}

	// don't fail if the time zone is missing, some
	// broken mail-/news-clients omit the time zone
	if (*dateString) {
		if ((strncasecmp(dateString, "gmt", 3) == 0) ||
			(strncasecmp(dateString, "utc", 3) == 0))
		{
			dateString += 3;
			while(*dateString && isspace(*dateString))
				dateString++;
		}

		if ((*dateString == '+') || (*dateString == '-')) {
			offset = strtol(dateString, &newPosStr, 10);
			if (abs(offset) < 30)
			{
				dateString = newPosStr;

				offset = offset * 100;

				if (*dateString && *(dateString+1))
				{
					dateString++;
					int minutes = strtol(dateString, &newPosStr, 10);
					if (offset > 0)
						offset += minutes;
					else
						offset -= minutes;
				}
			}

			if ((offset < -9959) || (offset > 9959))
				return result; // Invalid date

			int sgn = (offset < 0)? -1:1;
			offset = abs(offset);
			offset = ((offset / 100)*60 + (offset % 100))*sgn;
		} else {
			for (int i=0; known_zones[i].tzName[0] != 0; i++) {
				if (0 == strncasecmp(dateString, known_zones[i].tzName, strlen(known_zones[i].tzName))) {
					offset = known_zones[i].tzOffset;
					break;
				}
			}
		}
	}

	result = ymdhms_to_seconds(year, month+1, day, hour, minute, second);

	// avoid negative time values
	if ((offset > 0) && (offset > result))
		offset = 0;

	result -= offset*60;

	// If epoch 0 return epoch +1 which is Thu, 01-Jan-70 00:00:01 GMT
	// This is so that parse error and valid epoch 0 return values won't
	// be the same for sensitive applications...
	if (result < 1) result = 1;

	return result;
}

// copied and converted to std::string from KRFCDate::parseDateISO8601
static time_t parseISO8601Date(const std::string &pubDate)
{
	if (pubDate.empty()) {
		return 0;
	}

	// These dates look like this:
	// YYYY-MM-DDTHH:MM:SS
	// But they may also have 0, 1 or 2 suffixes.
	// Suffix 1: .secfrac (fraction of second)
	// Suffix 2: Either 'Z' or +zone or -zone, where zone is HHMM

	unsigned int year     = 0;
	unsigned int month    = 0;
	unsigned int mday     = 0;
	unsigned int hour     = 0;
	unsigned int min      = 0;
	unsigned int sec      = 0;

	int offset = 0;

	std::string input = pubDate;

	// First find the 'T' separator, if any.
	int tPos = input.find('T');

	// If there is no time, no month or no day specified, fill those missing
	// fields so that 'input' matches YYYY-MM-DDTHH:MM:SS
	if (-1 == tPos) {
		int dashes = 0;
		std::string::iterator it;
		for (it = input.begin(); it != input.end(); ++it) {
			if (*it == '-') {
				++dashes;
			}
		}
		if (0 == dashes) {
			input += "-01-01";
		} else if (1 == dashes) {
			input += "-01";
		}
		tPos = input.length();
		input += "T12:00:00";
	}

	// Now parse the date part.

	std::string dateString = input.substr(0, tPos);//.stripWhiteSpace();

	std::string timeString = input.substr(tPos + 1);//.stripWhiteSpace();

	std::vector<std::string> l;
	splitString(dateString, l, '-');

	if (l.size() < 3)
		return 0;

	sscanf(l[0].c_str(), "%u", &year);
	sscanf(l[1].c_str(), "%u", &month);
	sscanf(l[2].c_str(), "%u", &mday);

	// Z suffix means UTC.
	if ('Z' == timeString[timeString.length() - 1]) {
		timeString.erase(timeString.length() - 1, 1);
	}

	// +zone or -zone suffix (offset from UTC).

	int plusPos = timeString.find_last_of('+');

	if (-1 != plusPos) {
		std::string offsetString = timeString.substr(plusPos + 1);

		unsigned int offsetHour;
		unsigned int offsetMinute;

		sscanf(offsetString.substr(0, 1).c_str(), "%u", &offsetHour);
		sscanf(offsetString.substr(offsetString.length() - 2).c_str(), "%u", &offsetMinute);

		offset = offsetHour * 60 + offsetMinute;

		timeString = timeString.substr(0, plusPos);
	} else {
		int minusPos = timeString.find_last_of('-');

		if (-1 != minusPos) {
			std::string offsetString = timeString.substr(minusPos + 1);

			unsigned int offsetHour;
			unsigned int offsetMinute;

			sscanf(offsetString.substr(0, 1).c_str(), "%u", &offsetHour);
			sscanf(offsetString.substr(offsetString.length() - 2).c_str(), "%u", &offsetMinute);

			timeString = timeString.substr(0, minusPos);
		}
	}

	// secfrac suffix.
	int dotPos = timeString.find_last_of('.');

	if (-1 != dotPos) {
		timeString = timeString.substr(0, dotPos);
	}

	// Now parse the time part.

	splitString(timeString, l, ':');

	if (l.size() < 3)
		return 0;

	sscanf(l[0].c_str(), "%u", &hour);
	sscanf(l[1].c_str(), "%u", &min);
	sscanf(l[2].c_str(), "%u", &sec);

	time_t result = ymdhms_to_seconds(year, month, mday, hour, min, sec);

	// avoid negative time values
	if ((offset > 0) && (offset > result))
		offset = 0;

	result -= offset*60;

	// If epoch 0 return epoch +1 which is Thu, 01-Jan-70 00:00:01 GMT
	// This is so that parse error and valid epoch 0 return values won't
	// be the same for sensitive applications...
	if (result < 1) result = 1;

	return result;
}

RsFeedReaderErrorState p3FeedReaderThread::process(const RsFeedReaderFeed &feed, std::list<RsFeedReaderMsg*> &entries, std::string &errorString)
{
#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReaderThread::process - feed " << feed.feedId << " (" << feed.name << ")" << std::endl;
#endif

	RsFeedReaderErrorState result = RS_FEED_ERRORSTATE_OK;

	XMLWrapper xml;
	if (xml.readXML(feed.content.c_str())) {
		xmlNodePtr root = xml.getRootElement();
		if (root) {
			FeedFormat feedFormat;
			if (xmlStrcasecmp(root->name, BAD_CAST"rss") == 0) {
				feedFormat = FORMAT_RSS;
			} else if (xmlStrcasecmp (root->name, BAD_CAST"rdf") == 0) {
				feedFormat = FORMAT_RDF;
			} else if (xmlStrcasecmp (root->name, BAD_CAST"feed") == 0) {
				feedFormat = FORMAT_ATOM;
			} else {
				result = RS_FEED_ERRORSTATE_PROCESS_UNKNOWN_FORMAT;
				errorString = "Only RSS, RDF or ATOM supported";
			}

			if (result == RS_FEED_ERRORSTATE_OK) {
				xmlNodePtr channel = NULL;
				switch (feedFormat) {
				case FORMAT_RSS:
				case FORMAT_RDF:
					channel = xml.findNode(root->children, "channel");
					break;
				case FORMAT_ATOM:
					channel = root;
					break;
				}

				if (channel) {
					/* import header info */
					if (feed.flag & RS_FEED_FLAG_INFO_FROM_FEED) {
						std::string title;
						if (xml.getChildText(channel, "title", title) && !title.empty()) {
							std::string::size_type p;
							while ((p = title.find_first_of("\r\n")) != std::string::npos) {
								title.erase(p, 1);
							}
							std::string description;
							xml.getChildText(channel, (feedFormat == FORMAT_ATOM) ? "subtitle" : "description", description);
							mFeedReader->setFeedInfo(feed.feedId, title, description);
						}
					}

					/* process items */
					xmlNodePtr node;
					for (node = NULL; (node = getNextItem(feedFormat, channel, node)) != NULL; ) {
						if (!isRunning()) {
							break;
						}

						std::string title;
						if (!xml.getChildText(node, "title", title) || title.empty()) {
							continue;
						}

						/* remove newlines */
						std::string::size_type p;
						while ((p = title.find_first_of("\r\n")) != std::string::npos) {
							title.erase(p, 1);
						}

						RsFeedReaderMsg *item = new RsFeedReaderMsg();
						item->msgId.clear(); // is calculated later
						item->feedId = feed.feedId;
						item->title = title;

						/* try feedburner:origLink */
						if (!xml.getChildText(node, "origLink", item->link) || item->link.empty()) {
							xml.getChildText(node, "link", item->link);
							if (item->link.empty()) {
								xmlNodePtr linkNode = xml.findNode(node, "link", true);
								item->link = xml.getAttr(linkNode, "href");
							}
						}

						// remove sid=
						std::string linkUpper;
						stringToUpperCase(item->link, linkUpper);
						std::string::size_type sidStart = linkUpper.find("SID=");
						if (sidStart != std::string::npos) {
							std::string::size_type sidEnd1 = linkUpper.find(";", sidStart);
							std::string::size_type sidEnd2 = linkUpper.find("#", sidStart);

							if (sidEnd1 == std::string::npos) {
								sidEnd1 = linkUpper.size();
							}
							if (sidEnd2 == std::string::npos) {
								sidEnd2 = linkUpper.size();
							}

							if (sidStart > 0 && linkUpper[sidStart - 1] == '&') {
								sidStart--;
							}

							std::string::size_type sidEnd = std::min(sidEnd1, sidEnd2);
							item->link.erase(sidStart, sidEnd - sidStart);
						}

						if (feedFormat == FORMAT_ATOM) {
							/* <author><name>...</name></author> */
							xmlNodePtr author = xml.findNode(node->children, "author", false);
							if (author) {
								xml.getChildText(node, "name", item->author);
							}
						} else {
							if (!xml.getChildText(node, "author", item->author)) {
								xml.getChildText(node, "creator", item->author);
							}
						}

						switch (feedFormat) {
						case FORMAT_RSS:
						case FORMAT_RDF:
							/* try content:encoded */
							if (!xml.getChildText(node, "encoded", item->description)) {
								/* use description */
								xml.getChildText(node, "description", item->description);
							}
							break;
						case FORMAT_ATOM:
							/* try content */
							if (!xml.getChildText(node, "content", item->description)) {
								/* use summary */
								xml.getChildText(node, "summary", item->description);
							}
							break;
						}

						std::string pubDate;
						if (xml.getChildText(node, "pubDate", pubDate)) {
							item->pubDate = parseRFC822Date(pubDate);
						}
						if (xml.getChildText(node, "date", pubDate)) {
							item->pubDate = parseISO8601Date (pubDate);
						}
						if (xml.getChildText(node, "updated", pubDate)) {
							// atom
							item->pubDate = parseISO8601Date (pubDate);
						}

						if (item->pubDate == 0) {
							/* use current time */
							item->pubDate = time(NULL);
						}

						if (feedFormat == FORMAT_RSS) {
							/* <enclosure url="" type=""></enclosure> */
							xmlNodePtr enclosure = xml.findNode(node->children, "enclosure", false);
							if (enclosure) {
								std::string enclosureMimeType = xml.getAttr(enclosure, "type");
								std::string enclosureUrl = xml.getAttr(enclosure, "url");
								if (!enclosureUrl.empty()) {
									item->attachmentLink = enclosureUrl;
									item->attachmentMimeType = enclosureMimeType;
								}
							}
						}

						entries.push_back(item);
					}
				} else {
					result = RS_FEED_ERRORSTATE_PROCESS_UNKNOWN_FORMAT;
					errorString = "Channel not found";
				}
			}
		} else {
			result = RS_FEED_ERRORSTATE_PROCESS_UNKNOWN_FORMAT;
			errorString = "Can't read document";
		}
	} else {
		result = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
		errorString = xml.lastError();
	}

#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReaderThread::process - feed " << feed.feedId << " (" << feed.name << "), result " << result << ", error = " << errorString << std::endl;
	if (result == RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR) {
		std::cerr << "  Error: " << errorString << std::endl;
	}
#endif

	return result;
}

std::string p3FeedReaderThread::getProxyForFeed(const RsFeedReaderFeed &feed)
{
	std::string proxy;
	if (feed.flag & RS_FEED_FLAG_STANDARD_PROXY) {
		std::string standardProxyAddress;
		uint16_t standardProxyPort;
		if (mFeedReader->getStandardProxy(standardProxyAddress, standardProxyPort)) {
			rs_sprintf(proxy, "%s:%u", standardProxyAddress.c_str(), standardProxyPort);
		}
	} else {
		if (!feed.proxyAddress.empty() && feed.proxyPort) {
			rs_sprintf(proxy, "%s:%u", feed.proxyAddress.c_str(), feed.proxyPort);
		}
	}
	return proxy;
}

RsFeedReaderErrorState p3FeedReaderThread::processMsg(const RsFeedReaderFeed &feed, RsFeedReaderMsg *msg, std::string &errorString)
{
	//long todo_fill_errorString;

	if (!msg) {
		return RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
	}

	RsFeedReaderErrorState result = RS_FEED_ERRORSTATE_OK;
	std::string proxy = getProxyForFeed(feed);

	/* attachment */
	if (!msg->attachmentLink.empty()) {
		if (isContentType(msg->attachmentMimeType, "image/")) {
			CURLWrapper CURL(proxy);
			CURLcode code = CURL.downloadBinary(msg->attachmentLink, msg->attachmentBinary);
			if (code == CURLE_OK && CURL.responseCode() == 200) {
				std::string contentType = CURL.contentType();
				if (isContentType(contentType, "image/")) {
					msg->attachmentBinaryMimeType = contentType;

					bool forum = (feed.flag & RS_FEED_FLAG_FORUM) && !feed.preview;
					bool posted = (feed.flag & RS_FEED_FLAG_POSTED) && !feed.preview;

					if (!forum && ! posted) {
						/* no need to optimize image */
						std::vector<unsigned char> optimizedBinary;
						std::string optimizedMimeType;
						if (mFeedReader->optimizeImage(FeedReaderOptimizeImageTask::SIZE, msg->attachmentBinary, msg->attachmentBinaryMimeType, optimizedBinary, optimizedMimeType)) {
							if (toBase64(optimizedBinary, msg->attachment)) {
								msg->attachmentMimeType = optimizedMimeType;
							} else {
								msg->attachment.clear();
							}
						}
					}
				} else {
					msg->attachmentBinary.clear();
				}
			} else {
				msg->attachmentBinary.clear();
			}
		}
	}

	std::string url;
	if (feed.flag & RS_FEED_FLAG_SAVE_COMPLETE_PAGE) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") download page " << msg->link << std::endl;
#endif
		std::string content;
		CURLWrapper CURL(proxy);
		CURLcode code = CURL.downloadText(msg->link, content);

		if (code == CURLE_OK) {
			long responseCode = CURL.responseCode();

			switch (responseCode) {
			case 200:
				{
					std::string contentType = CURL.contentType();

					if (isContentType(CURL.contentType(), "text/html")) {
						/* ok */
						msg->description = content;
					} else {
						result = RS_FEED_ERRORSTATE_DOWNLOAD_UNKNOWN_CONTENT_TYPE;
						errorString = contentType;
					}
				}
				break;
			case 404:
				result = RS_FEED_ERRORSTATE_DOWNLOAD_NOT_FOUND;
				break;
			default:
				result = RS_FEED_ERRORSTATE_DOWNLOAD_UNKOWN_RESPONSE_CODE;
				rs_sprintf(errorString, "%ld", responseCode);
			}
		} else {
			result = RS_FEED_ERRORSTATE_DOWNLOAD_ERROR;
			errorString = curl_easy_strerror(code);
		}

		if (result != RS_FEED_ERRORSTATE_OK) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") cannot download page, CURLCode = " << code << ", error = " << errorString << std::endl;
#endif
			return result;
		}

		/* get effective url (moved location) */
		std::string effectiveUrl = CURL.effectiveUrl();
		url = getBaseLink(effectiveUrl.empty() ? msg->link : effectiveUrl);
	}

	/* check if string contains xml chars (very simple test) */
	if (msg->description.find('<') == std::string::npos && feed.transformationType == RS_FEED_TRANSFORMATION_TYPE_NONE) {
		return result;
	}

	if (isRunning()) {
		/* process description */
		bool processPostedFirstImage = (feed.flag & RS_FEED_FLAG_POSTED_FIRST_IMAGE) ? TRUE : FALSE;
		if (!msg->attachmentBinary.empty()) {
			/* use attachment as image */
			processPostedFirstImage = FALSE;
		}

		//long todo; // encoding
		HTMLWrapper html;
		if (html.readHTML(msg->description.c_str(), url.c_str())) {
			xmlNodePtr root = html.getRootElement();
			if (root) {
				std::list<xmlNodePtr> nodesToDelete;
				xmlNodePtr postedFirstImageNode = NULL;

				/* process all children */
				std::list<xmlNodePtr> nodes;
				nodes.push_back(root);

				while (!nodes.empty()) {
					if (!isRunning()) {
						break;
					}
					xmlNodePtr node = nodes.front();
					nodes.pop_front();

					switch (node->type) {
					case XML_ELEMENT_NODE:
						if (xmlStrcasecmp(node->name, BAD_CAST"img") == 0) {
							/* process images */

							if ((feed.flag & RS_FEED_FLAG_EMBED_IMAGES) == 0) {
								/* remove image */
								xmlUnlinkNode(node);
								nodesToDelete.push_back(node);
								continue;
							}
						} else if (xmlStrcasecmp(node->name, BAD_CAST"script") == 0) {
							/* remove script */
							xmlUnlinkNode(node);
							nodesToDelete.push_back(node);
							continue;
						}

						xmlNodePtr child;
						for (child = node->children; child; child = child->next) {
							nodes.push_back(child);
						}
						break;

					case XML_TEXT_NODE:
						{
							/* check for only space */
							std::string content;
							if (html.getContent(node, content, false)) {
								std::string newContent = content;

								/* trim */
								XMLWrapper::trimString(newContent);

								if (newContent.empty()) {
									xmlUnlinkNode(node);
									nodesToDelete.push_back(node);
								} else {
									if (content != newContent) {
										html.setContent(node, newContent.c_str());
									}
								}
							}
						}
						break;

					case XML_COMMENT_NODE:
//						xmlUnlinkNode(node);
//						nodesToDelete.push_back(node);
						break;

					case XML_ATTRIBUTE_NODE:
					case XML_CDATA_SECTION_NODE:
					case XML_ENTITY_REF_NODE:
					case XML_ENTITY_NODE:
					case XML_PI_NODE:
					case XML_DOCUMENT_NODE:
					case XML_DOCUMENT_TYPE_NODE:
					case XML_DOCUMENT_FRAG_NODE:
					case XML_NOTATION_NODE:
					case XML_HTML_DOCUMENT_NODE:
					case XML_DTD_NODE:
					case XML_ELEMENT_DECL:
					case XML_ATTRIBUTE_DECL:
					case XML_ENTITY_DECL:
					case XML_NAMESPACE_DECL:
					case XML_XINCLUDE_START:
					case XML_XINCLUDE_END:
#ifdef LIBXML_DOCB_ENABLED
					case XML_DOCB_DOCUMENT_NODE:
#endif
						break;
					}
				}

				std::list<xmlNodePtr>::iterator nodeIt;
				for (nodeIt = nodesToDelete.begin(); nodeIt != nodesToDelete.end(); ++nodeIt) {
					xmlFreeNode(*nodeIt);
				}
				nodesToDelete.clear();

				if (isRunning() && result == RS_FEED_ERRORSTATE_OK) {
					unsigned int xpathCount;
					unsigned int xpathIndex;
					XPathWrapper *xpath = html.createXPath();
					if (xpath) {
						/* process images */
						if (xpath->compile("//img")) {
							xpathCount = xpath->count();
							for (xpathIndex = 0; xpathIndex < xpathCount; ++xpathIndex) {
								if (!isRunning()) {
									break;
								}
								xmlNodePtr node = xpath->node(xpathIndex);

								if (node->type == XML_ELEMENT_NODE) {
									bool removeImage = true;

									if (feed.flag & RS_FEED_FLAG_EMBED_IMAGES) {
										/* embed image */
										std::string src = html.getAttr(node, "src");
										if (!src.empty()) {
											/* download image */
#ifdef FEEDREADER_DEBUG
											std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") download image " << src << std::endl;
#endif
											std::vector<unsigned char> data;
											CURLWrapper CURL(proxy);
											CURLcode code = CURL.downloadBinary(calculateLink(url, src), data);
											if (code == CURLE_OK && CURL.responseCode() == 200) {
												std::string contentType = CURL.contentType();
												if (isContentType(contentType, "image/")) {
													std::vector<unsigned char> optimizedData;
													std::string optimizedMimeType;
													if (mFeedReader->optimizeImage(FeedReaderOptimizeImageTask::SIZE, data, contentType, optimizedData, optimizedMimeType)) {
														std::string base64;
														if (toBase64(optimizedData, base64)) {
															std::string imageBase64;
															rs_sprintf(imageBase64, "data:%s;base64,%s", optimizedMimeType.c_str(), base64.c_str());
															if (html.setAttr(node, "src", imageBase64.c_str())) {
																removeImage = false;
															}
														}
													}
													if (processPostedFirstImage && postedFirstImageNode == NULL) {
														/* set first image */
														msg->postedFirstImage = data;
														msg->postedFirstImageMimeType = contentType;
														postedFirstImageNode = node;
													}
												}
											}
										}
									}

									if (removeImage) {
										/* remove image */
										xmlUnlinkNode(node);
										nodesToDelete.push_back(node);
										continue;
									}
								}
							}
						} else {
							// unable to compile xpath expression
							result = RS_FEED_ERRORSTATE_PROCESS_XPATH_INTERNAL_ERROR;
						}
						delete(xpath);
						xpath = NULL;
					} else {
						// unable to create xpath object
						result = RS_FEED_ERRORSTATE_PROCESS_XPATH_INTERNAL_ERROR;
						std::cerr << "p3FeedReaderThread::process - feed " << feed.feedId << " (" << feed.name << "), unable to create xpath object" << std::endl;
					}
				}

				for (nodeIt = nodesToDelete.begin(); nodeIt != nodesToDelete.end(); ++nodeIt) {
					xmlFreeNode(*nodeIt);
				}
				nodesToDelete.clear();

				if (result == RS_FEED_ERRORSTATE_OK) {
					if (isRunning()) {
						if (html.saveHTML(msg->description)) {
							if (postedFirstImageNode) {
								/* Remove first image and create description without the image */
								xmlUnlinkNode(postedFirstImageNode);
								xmlFreeNode(postedFirstImageNode);

								if (!html.saveHTML(msg->postedDescriptionWithoutFirstImage)) {
									errorString = html.lastError();
#ifdef FEEDREADER_DEBUG
									std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") cannot dump html" << std::endl;
									std::cerr << "  Error: " << errorString << std::endl;
#endif
									result = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
								}
							}
						} else {
							errorString = html.lastError();
#ifdef FEEDREADER_DEBUG
							std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") cannot dump html" << std::endl;
							std::cerr << "  Error: " << errorString << std::endl;
#endif
							result = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
						}
					}
				}
			} else {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") no root element" << std::endl;
#endif
				result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
			}
		} else {
			errorString = html.lastError();
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") cannot read html" << std::endl;
			std::cerr << "  Error: " << errorString << std::endl;
#endif
			result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
		}
	}

	return result;
}

RsFeedReaderErrorState p3FeedReaderThread::processTransformation(const RsFeedReaderFeed &feed, RsFeedReaderMsg *msg, std::string &errorString)
{
	RsFeedReaderErrorState result = RS_FEED_ERRORSTATE_OK;

	switch (feed.transformationType) {
	case RS_FEED_TRANSFORMATION_TYPE_NONE:
		break;
	case RS_FEED_TRANSFORMATION_TYPE_XPATH:
		msg->descriptionTransformed = msg->description;
		result = processXPath(feed.xpathsToUse.ids, feed.xpathsToRemove.ids, msg->descriptionTransformed, errorString);
		break;
	case RS_FEED_TRANSFORMATION_TYPE_XSLT:
		msg->descriptionTransformed = msg->description;
		result = processXslt(feed.xslt, msg->descriptionTransformed, errorString);
		break;
	}

	if (msg->descriptionTransformed == msg->description) {
		msg->descriptionTransformed.clear();
	}

	return result;
}

RsFeedReaderErrorState p3FeedReaderThread::processXPath(const std::list<std::string> &xpathsToUse, const std::list<std::string> &xpathsToRemove, HTMLWrapper &html, std::string &errorString)
{
#warning p3FeedReaderThread.cc TODO thunder2: fill_errorString;

	if (xpathsToUse.empty() && xpathsToRemove.empty()) {
		return RS_FEED_ERRORSTATE_OK;
	}

	XPathWrapper *xpath = html.createXPath();
	if (xpath == NULL) {
		// unable to create xpath object
		std::cerr << "p3FeedReaderThread::processXPath - unable to create xpath object" << std::endl;
		return RS_FEED_ERRORSTATE_PROCESS_XPATH_INTERNAL_ERROR;
	}

	RsFeedReaderErrorState result = RS_FEED_ERRORSTATE_OK;

	unsigned int xpathCount;
	unsigned int xpathIndex;
	std::list<std::string>::const_iterator xpathIt;

	if (!xpathsToUse.empty()) {
		HTMLWrapper htmlNew;
		if (htmlNew.createHTML()) {
			xmlNodePtr body = htmlNew.getBody();
			if (body) {
				/* process use list */
				for (xpathIt = xpathsToUse.begin(); xpathIt != xpathsToUse.end(); ++xpathIt) {
					if (xpath->compile(xpathIt->c_str())) {
						xpathCount = xpath->count();
						if (xpathCount) {
							for (xpathIndex = 0; xpathIndex < xpathCount; ++xpathIndex) {
								xmlNodePtr node = xpath->node(xpathIndex);
								xmlUnlinkNode(node);
								xmlAddChild(body, node);
							}
						} else {
							result = RS_FEED_ERRORSTATE_PROCESS_XPATH_NO_RESULT;
							errorString = *xpathIt;
							break;
						}
					} else {
						// unable to process xpath expression
#ifdef FEEDREADER_DEBUG
						std::cerr << "p3FeedReaderThread::processXPath - unable to process xpath expression" << std::endl;
#endif
						errorString = *xpathIt;
						result = RS_FEED_ERRORSTATE_PROCESS_XPATH_WRONG_EXPRESSION;
					}
				}
			} else {
				result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
			}
		} else {
			result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
		}

		if (result == RS_FEED_ERRORSTATE_OK) {
			html = htmlNew;
		}
	}

	if (result == RS_FEED_ERRORSTATE_OK) {
		std::list<xmlNodePtr> nodesToDelete;

		/* process remove list */
		for (xpathIt = xpathsToRemove.begin(); xpathIt != xpathsToRemove.end(); ++xpathIt) {
			if (xpath->compile(xpathIt->c_str())) {
				xpathCount = xpath->count();
				if (xpathCount) {
					for (xpathIndex = 0; xpathIndex < xpathCount; ++xpathIndex) {
						xmlNodePtr node = xpath->node(xpathIndex);

						xmlUnlinkNode(node);
						nodesToDelete.push_back(node);
					}
				} else {
					result = RS_FEED_ERRORSTATE_PROCESS_XPATH_NO_RESULT;
					errorString = *xpathIt;
					break;
				}
			} else {
				// unable to process xpath expression
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReaderThread::processXPath - unable to process xpath expression" << std::endl;
#endif
				errorString = *xpathIt;
				result = RS_FEED_ERRORSTATE_PROCESS_XPATH_WRONG_EXPRESSION;
				break;
			}
		}

		std::list<xmlNodePtr>::iterator nodeIt;
		for (nodeIt = nodesToDelete.begin(); nodeIt != nodesToDelete.end(); ++nodeIt) {
			xmlFreeNode(*nodeIt);
		}
		nodesToDelete.clear();
	}

	delete(xpath);

	return result;
}

RsFeedReaderErrorState p3FeedReaderThread::processXPath(const std::list<std::string> &xpathsToUse, const std::list<std::string> &xpathsToRemove, std::string &description, std::string &errorString)
{
	if (xpathsToUse.empty() && xpathsToRemove.empty()) {
		return RS_FEED_ERRORSTATE_OK;
	}

	RsFeedReaderErrorState result = RS_FEED_ERRORSTATE_OK;

	/* process description */
#warning p3FeedReaderThread.cc TODO thunder2: encoding
	HTMLWrapper html;
	if (html.readHTML(description.c_str(), "")) {
		xmlNodePtr root = html.getRootElement();
		if (root) {
			result = processXPath(xpathsToUse, xpathsToRemove, html, errorString);

			if (result == RS_FEED_ERRORSTATE_OK) {
				if (!html.saveHTML(description)) {
					errorString = html.lastError();
#ifdef FEEDREADER_DEBUG
					std::cerr << "p3FeedReaderThread::processXPath - cannot dump html" << std::endl;
					std::cerr << "  Error: " << errorString << std::endl;
#endif
					result = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
				}
			}
		} else {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReaderThread::processXPath - no root element" << std::endl;
#endif
			errorString = "No root element found";
			result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
		}
	} else {
		errorString = html.lastError();
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReaderThread::processXPath - cannot read html" << std::endl;
		std::cerr << "  Error: " << errorString << std::endl;
#endif
		result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
	}

	return result;
}

RsFeedReaderErrorState p3FeedReaderThread::processXslt(const std::string &xslt, HTMLWrapper &html, std::string &errorString)
{
	XMLWrapper style;
	if (!style.readXML(xslt.c_str())) {
		errorString = style.lastError();
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReaderThread::processXslt - error loading style" << std::endl;
		std::cerr << "  Error: " << errorString << std::endl;
#endif
		return RS_FEED_ERRORSTATE_PROCESS_XSLT_FORMAT_ERROR;
	}

	XMLWrapper xmlResult;
	if (!html.transform(style, xmlResult)) {
		errorString = html.lastError();
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReaderThread::processXslt - error transform" << std::endl;
		std::cerr << "  Error: " << errorString << std::endl;
#endif
		return RS_FEED_ERRORSTATE_PROCESS_XSLT_TRANSFORM_ERROR;
	}

	RsFeedReaderErrorState result = RS_FEED_ERRORSTATE_OK;

	xmlNodePtr root = xmlResult.getRootElement();
	if (root) {
		if (xmlResult.nodeName(root) == "html") {
			if (root->children && xmlResult.nodeName(root->children) == "body") {
				root = root->children->children;
			}
		}
		HTMLWrapper htmlNew;
		if (htmlNew.createHTML()) {
			xmlNodePtr body = htmlNew.getBody();
			if (body) {
				/* copy result nodes */
				xmlNodePtr node;
				for (node = root; node; node = node->next) {
					xmlNodePtr newNode = xmlCopyNode(node, 1);
					if (newNode) {
						if (!xmlAddChild(body, newNode)) {
							xmlFreeNode(newNode);
							break;
						}
					} else {
						result = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
#ifdef FEEDREADER_DEBUG
						std::cerr << "p3FeedReaderThread::processXslt - node copy error" << std::endl;
#endif
						break;
					}
				}
			} else {
				result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
			}
		} else {
			result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
		}

		if (result == RS_FEED_ERRORSTATE_OK) {
			html = htmlNew;
		}
	} else {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReaderThread::processXslt - no result" << std::endl;
#endif
		result = RS_FEED_ERRORSTATE_PROCESS_XSLT_NO_RESULT;
	}

	return result;
}

RsFeedReaderErrorState p3FeedReaderThread::processXslt(const std::string &xslt, std::string &description, std::string &errorString)
{
	if (xslt.empty()) {
		return RS_FEED_ERRORSTATE_OK;
	}

	RsFeedReaderErrorState result = RS_FEED_ERRORSTATE_OK;

	/* process description */
	//long todo; // encoding
	HTMLWrapper html;
	if (html.readHTML(description.c_str(), "")) {
		xmlNodePtr root = html.getRootElement();
		if (root) {
			result = processXslt(xslt, html, errorString);

			if (result == RS_FEED_ERRORSTATE_OK) {
				if (!html.saveHTML(description)) {
					errorString = html.lastError();
#ifdef FEEDREADER_DEBUG
					std::cerr << "p3FeedReaderThread::processXslt - cannot dump html" << std::endl;
					std::cerr << "  Error: " << errorString << std::endl;
#endif
					result = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
				}
			}
		} else {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReaderThread::processXslt - no root element" << std::endl;
#endif
			errorString = "No root element found";
			result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
		}
	} else {
		errorString = html.lastError();
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReaderThread::processXslt - cannot read html" << std::endl;
		std::cerr << "  Error: " << errorString << std::endl;
#endif
		result = RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR;
	}

	return result;
}
