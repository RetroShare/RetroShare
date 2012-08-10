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

#include "p3FeedReaderThread.h"
#include "rsFeedReaderItems.h"
#include "util/rsstring.h"
#include "util/CURLWrapper.h"

#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <openssl/evp.h>

enum FeedFormat { FORMAT_RSS, FORMAT_RDF };

/*********
 * #define FEEDREADER_DEBUG
 *********/
#define FEEDREADER_DEBUG

p3FeedReaderThread::p3FeedReaderThread(p3FeedReader *feedReader, Type type) : RsThread(), mFeedReader(feedReader), mType(type)
{
	if (type == PROCESS) {
		mCharEncodingHandler = xmlFindCharEncodingHandler ("UTF8");

		if (!mCharEncodingHandler) {
			/* no encoding handler found */
			std::cerr << "p3FeedReaderThread::p3FeedReaderThread - no encoding handler found" << std::endl;
		}
	} else {
		mCharEncodingHandler = NULL;
	}
}

p3FeedReaderThread::~p3FeedReaderThread()
{
	xmlCharEncCloseFunc((xmlCharEncodingHandlerPtr) mCharEncodingHandler);
}

/***************************************************************************/
/****************************** Thread *************************************/
/***************************************************************************/

void p3FeedReaderThread::run()
{
	while (isRunning()) {
#ifdef WIN32
		Sleep(1000);
#else
		usleep(1000000);
#endif
		/* every second */

		switch (mType) {
		case DOWNLOAD:
			{
				RsFeedReaderFeed feed;
				if (mFeedReader->getFeedToDownload(feed)) {
					std::string content;
					std::string icon;
					std::string error;

					DownloadResult result = download(feed, content, icon, error);
					if (result == DOWNLOAD_SUCCESS) {
						mFeedReader->onDownloadSuccess(feed.feedId, content, icon);
					} else {
						mFeedReader->onDownloadError(feed.feedId, result, error);
					}
				}
			}
			break;
		case PROCESS:
			{
				RsFeedReaderFeed feed;
				if (mFeedReader->getFeedToProcess(feed)) {
					std::list<RsFeedReaderMsg*> msgs;
					std::string error;
					std::list<RsFeedReaderMsg*>::iterator it;

					ProcessResult result = process(feed, msgs, error);
					if (result == PROCESS_SUCCESS) {
						/* first, filter the messages */
						bool result = mFeedReader->onProcessSuccess_filterMsg(feed.feedId, msgs);
						if (isRunning()) {
							if (result) {
								long todo; // process new items
								/* second, process the descriptions */
								for (it = msgs.begin(); it != msgs.end(); ++it) {
									RsFeedReaderMsg *mi = *it;
									processMsg(feed, mi);
								}
							}
							/* third, add messages */
							mFeedReader->onProcessSuccess_addMsgs(feed.feedId, result, msgs);
						}
					} else {
						mFeedReader->onProcessError(feed.feedId, result);
					}

					for (it = msgs.begin(); it != msgs.end(); ++it) {
						delete (*it);
					}
					msgs.clear();
				}
			}
			break;
		}
	}
}

/***************************************************************************/
/****************************** Download ***********************************/
/***************************************************************************/

static bool isContentType(const std::string &contentType, const char *type)
{
	return (strncasecmp(contentType.c_str(), type, strlen(type)) == 0);
}

static bool toBase64(const std::vector<unsigned char> &data, std::string &base64)
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
	if (link.substr(0, 7) == "http://") {
		/* absolute link */
		return link;
	}

	/* calculate link of base link */
	std::string resultLink = baseLink;

	/* link should begin with "http://" */
	if (resultLink.substr(0, 7) != "http://") {
		resultLink.insert(0, "http://");
	}

	if (link.empty()) {
		/* no link */
		return resultLink;
	}

	if (*link.begin() == '/') {
		/* link begins with "/" */
		size_t found = resultLink.find('/', 7);
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
			if (isContentType(contentType, "image/x-icon") ||
				isContentType(contentType, "application/octet-stream") ||
				isContentType(contentType, "text/plain")) {
				if (!vicon.empty()) {
					long todo; // check it
					result = toBase64(vicon, icon);
				}
			}
		}
	}

	return result;
}

p3FeedReaderThread::DownloadResult p3FeedReaderThread::download(const RsFeedReaderFeed &feed, std::string &content, std::string &icon, std::string &error)
{
#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReaderThread::download - feed " << feed.feedId << " (" << feed.name << ")" << std::endl;
#endif

	content.clear();
	error.clear();

	DownloadResult result;

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
					isContentType(contentType, "application/rss+xml") ||
					isContentType(contentType, "application/xml") ||
					isContentType(contentType, "application/xhtml+xml")) {
					/* ok */
					result = DOWNLOAD_SUCCESS;
				} else {
					result = DOWNLOAD_UNKNOWN_CONTENT_TYPE;
					error = contentType;
				}
			}
			break;
		case 404:
			result = DOWNLOAD_NOT_FOUND;
			break;
		default:
			result = DOWNLOAD_UNKOWN_RESPONSE_CODE;
			rs_sprintf(error, "%ld", responseCode);
		}

		getFavicon(CURL, feed.url, icon);
	} else {
		result = DOWNLOAD_ERROR;
		error = curl_easy_strerror(code);
	}

#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReaderThread::download - feed " << feed.feedId << " (" << feed.name << "), result " << result << ", error = " << error << std::endl;
#endif

	return result;
}

/***************************************************************************/
/****************************** Process ************************************/
/***************************************************************************/

static bool convertToString(xmlCharEncodingHandlerPtr charEncodingHandler, const xmlChar *xmlText, std::string &text)
{
	bool result = false;

	xmlBufferPtr in = xmlBufferCreateStatic((void*) xmlText, xmlStrlen(xmlText));
	xmlBufferPtr out = xmlBufferCreate();
	int ret = xmlCharEncOutFunc(charEncodingHandler, out, in);
	if (ret >= 0) {
		result = true;
		text = (char*) xmlBufferContent(out);
	}

	xmlBufferFree(in);
	xmlBufferFree(out);

	return result;
}

static bool convertFromString(xmlCharEncodingHandlerPtr charEncodingHandler, const char *text, xmlChar *&xmlText)
{
	bool result = false;

	xmlBufferPtr in = xmlBufferCreateStatic((void*) text, strlen(text));
	xmlBufferPtr out = xmlBufferCreate();
	int ret = xmlCharEncOutFunc(charEncodingHandler, out, in);
	if (ret >= 0) {
		result = true;
		xmlText = xmlBufferDetach(out);
	}

	xmlBufferFree(in);
	xmlBufferFree(out);

	return result;
}

static xmlNodePtr findNode(xmlNodePtr node, const char *name, bool children = false)
{
	if (node->name) {
		if (xmlStrcasecmp (node->name, (xmlChar*) name) == 0) {
			return node;
		}
	}

	xmlNodePtr nodeFound = NULL;
	if (children) {
		if (node->children) {
			nodeFound = findNode(node->children, name, children);
			if (nodeFound) {
				return nodeFound;
			}
		}
	}

	if (node->next) {
		nodeFound = findNode(node->next, name, children);
		if (nodeFound) {
			return nodeFound;
		}
	}

	return NULL;
}

static xmlNodePtr getNextItem(FeedFormat feedFormat, xmlNodePtr channel, xmlNodePtr item)
{
	if (!channel) {
		return NULL;
	}

	if (!item) {
		switch (feedFormat) {
		case FORMAT_RSS:
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
		if (item->type == XML_ELEMENT_NODE && xmlStrcasecmp (item->name, (xmlChar*) "item") == 0) {
			break;
		}
	}

	return item;
}

static bool getChildText(/*xmlCharEncodingHandlerPtr*/ void *charEncodingHandler, xmlNodePtr node, const char *childName, std::string &text)
{
	if (node == NULL || node->children == NULL) {
		return false;
	}

	xmlNodePtr child = findNode(node->children, childName, true);
	if (!child) {
		return false;
	}

	if (child->type != XML_ELEMENT_NODE) {
		return false;
	}

	if (!child->children) {
		return false;
	}

	if (child->children->type != XML_TEXT_NODE) {
		return false;
	}

	if (child->children->content) {
		return convertToString((xmlCharEncodingHandlerPtr) charEncodingHandler, child->children->content, text);
	}

	return true;
}

static std::string xmlGetAttr(/*xmlCharEncodingHandlerPtr*/ void *charEncodingHandler, xmlNodePtr node, const char *name)
{
	if (!node || !name) {
		return "";
	}

	std::string value;

	xmlChar *xmlValue = xmlGetProp(node, (const xmlChar*) name);
	if (xmlValue) {
		convertToString((xmlCharEncodingHandlerPtr) charEncodingHandler, xmlValue, value);
		xmlFree(xmlValue);
	}

	return value;
}

static bool xmlSetAttr(/*xmlCharEncodingHandlerPtr*/ void *charEncodingHandler, xmlNodePtr node, const char *name, const char *value)
{
	if (!node || !name) {
		return false;
	}

	xmlChar *xmlValue = NULL;
	if (!convertFromString((xmlCharEncodingHandlerPtr) charEncodingHandler, value, xmlValue)) {
		return false;
	}

	xmlAttrPtr xmlAttr = xmlSetProp (node, (const xmlChar*) name, xmlValue);
	xmlFree(xmlValue);

	return xmlAttr != NULL;
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
			for (int i=0; known_zones[i].tzName != 0; i++) {
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

p3FeedReaderThread::ProcessResult p3FeedReaderThread::process(const RsFeedReaderFeed &feed, std::list<RsFeedReaderMsg*> &entries, std::string &error)
{
#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReaderThread::process - feed " << feed.feedId << " (" << feed.name << ")" << std::endl;
#endif

	ProcessResult result = PROCESS_SUCCESS;

	xmlDocPtr document = xmlReadDoc((const xmlChar*) feed.content.c_str(), "", NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_COMPACT | XML_PARSE_NOCDATA);
	if (document) {
		xmlNodePtr root = xmlDocGetRootElement(document);
		if (root) {
			FeedFormat feedFormat;
			if (xmlStrcmp (root->name, (xmlChar*) "rss") == 0) {
				feedFormat = FORMAT_RSS;
			} else if (xmlStrcmp (root->name, (xmlChar*) "rdf") != 0) {
				feedFormat = FORMAT_RDF;
			} else {
				result = PROCESS_UNKNOWN_FORMAT;
				error = "Only RSS or RDF supported";
			}

			if (result == PROCESS_SUCCESS) {
				xmlNodePtr channel = findNode(root->children, "channel");
				if (channel) {
					/* import header info */
					if (feed.flag & RS_FEED_FLAG_INFO_FROM_FEED) {
						std::string title;
						if (getChildText(mCharEncodingHandler, channel, "title", title) && !title.empty()) {
							std::string::size_type p;
							while ((p = title.find_first_of("\r\n")) != std::string::npos) {
								title.erase(p, 1);
							}
							std::string description;
							getChildText(mCharEncodingHandler, channel, "description", description);
							mFeedReader->setFeedInfo(feed.feedId, title, description);
						}
					}

					/* get item count */
					xmlNodePtr node;
					for (node = NULL; (node = getNextItem(feedFormat, channel, node)) != NULL; ) {
						if (!isRunning()) {
							break;
						}

						std::string title;
						if (!getChildText(mCharEncodingHandler, node, "title", title) || title.empty()) {
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
						if (!getChildText(mCharEncodingHandler, node, "origLink", item->link) || item->link.empty()) {
							getChildText(mCharEncodingHandler, node, "link", item->link);
						}

						long todo; // remove sid
//						// remove sid=
//						CString sLinkUpper = sLink;
//						sLinkUpper.MakeUpper ();
//						int nSIDStart = sLinkUpper.Find (TEXT("SID="));
//						if (nSIDStart != -1) {
//							int nSIDEnd1 = sLinkUpper.Find (TEXT(";"), nSIDStart);
//							int nSIDEnd2 = sLinkUpper.Find (TEXT("#"), nSIDStart);

//							if (nSIDEnd1 == -1) {
//								nSIDEnd1 = sLinkUpper.GetLength ();
//							}
//							if (nSIDEnd2 == -1) {
//								nSIDEnd2 = sLinkUpper.GetLength ();
//							}

//							if (nSIDStart > 0 && sLinkUpper [nSIDStart - 1] == '&') {
//								nSIDStart--;
//							}

//							int nSIDEnd = min (nSIDEnd1, nSIDEnd2);
//							sLink.Delete (nSIDStart, nSIDEnd - nSIDStart);
//						}

						getChildText(mCharEncodingHandler, node, "author", item->author);

						getChildText(mCharEncodingHandler, node, "description", item->description);

						std::string pubDate;
						if (getChildText(mCharEncodingHandler, node, "pubdate", pubDate)) {
							item->pubDate = parseRFC822Date(pubDate);
						}
						if (getChildText(mCharEncodingHandler, node, "date", pubDate)) {
							item->pubDate = parseISO8601Date (pubDate);
						}

						if (item->pubDate == 0) {
							/* use current time */
							item->pubDate = time(NULL);
						}

						entries.push_back(item);
					}
				} else {
					result = PROCESS_UNKNOWN_FORMAT;
					error = "Channel not found";
				}
			}
		} else {
			result = PROCESS_UNKNOWN_FORMAT;
			error = "Can't read document";
		}

		xmlFreeDoc(document);
	} else {
		result = PROCESS_ERROR_INIT;
	}

#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReaderThread::process - feed " << feed.feedId << " (" << feed.name << "), result " << result << ", error = " << error << std::endl;
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

bool p3FeedReaderThread::processMsg(const RsFeedReaderFeed &feed, RsFeedReaderMsg *msg)
{
	if (!msg) {
		return false;
	}

	std::string proxy = getProxyForFeed(feed);

	std::string url;
	if (feed.flag & RS_FEED_FLAG_SAVE_COMPLETE_PAGE) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") download page " << msg->link << std::endl;
#endif
		std::string content;
		CURLWrapper CURL(proxy);
		CURLcode code = CURL.downloadText(msg->link, content);

		if (code == CURLE_OK && CURL.responseCode() == 200 && isContentType(CURL.contentType(), "text/html")) {
			/* ok */
			msg->description = content;
		} else {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") cannot download page, CURLCode = " << code << ", responseCode = " << CURL.responseCode() << ", contentType = " << CURL.contentType() << std::endl;
#endif
			return false;
		}

		/* get effective url (moved location) */
		std::string effectiveUrl = CURL.effectiveUrl();
		url = getBaseLink(effectiveUrl.empty() ? msg->link : effectiveUrl);
	}

	/* check if string contains xml chars (very simple test) */
	if (msg->description.find('<') == std::string::npos) {
		return true;
	}

	bool result = true;

	/* process description */
	long todo; // encoding
	htmlDocPtr document = htmlReadMemory(msg->description.c_str(), msg->description.length(), url.c_str(), "", HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_COMPACT);
	if (document) {
		xmlNodePtr root = xmlDocGetRootElement(document);
		if (root) {
			/* process all children */
			std::list<xmlNodePtr> parents;
			parents.push_back(root);

			while (!parents.empty()) {
				if (!isRunning()) {
					break;
				}
				xmlNodePtr node = parents.front();
				parents.pop_front();

				if (node->type == XML_ELEMENT_NODE) {
					/* check for image */
					if (strcasecmp((char*) node->name, "img") == 0) {
						bool removeImage = true;

						if (feed.flag & RS_FEED_FLAG_EMBED_IMAGES) {
							/* embed image */
							std::string src = xmlGetAttr(mCharEncodingHandler, node, "src");
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
										std::string base64;
										if (toBase64(data, base64)) {
											std::string imageBase64;
											rs_sprintf(imageBase64, "data:%s;base64,%s", contentType.c_str(), base64.c_str());
											if (xmlSetAttr(mCharEncodingHandler, node, "src", imageBase64.c_str())) {
												removeImage = false;
											}
										}
									}
								}
							}
						}

						if (removeImage) {
							/* remove image */
							xmlUnlinkNode(node);
							xmlFreeNode(node);
							continue;
						}
					}

					xmlNodePtr child;
					for (child = node->children; child; child = child->next) {
						parents.push_back(child);
					}
				}
			}

			xmlChar *html = NULL;
			int htmlSize = 0;
			htmlDocDumpMemoryFormat(document, &html, &htmlSize, 0);
			if (html) {
				convertToString((xmlCharEncodingHandlerPtr) mCharEncodingHandler, html, msg->description);
				xmlFree(html);
			} else {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") cannot dump html" << std::endl;
#endif
				result = false;
			}
		} else {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReaderThread::processHTML - feed " << feed.feedId << " (" << feed.name << ") no root element" << std::endl;
#endif
			result = false;
		}

		xmlFreeDoc(document);
	}

	return result;
}
