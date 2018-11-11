/*******************************************************************************
 * plugins/FeedReader/util/CURLWrapper.cc                                      *
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

#include "CURLWrapper.h"
#include <string.h>

CURLWrapper::CURLWrapper(const std::string &proxy)
{
	mCurl = curl_easy_init();
	if (mCurl) {
		curl_easy_setopt(mCurl, CURLOPT_NOPROGRESS, 0);
//		curl_easy_setopt(mCurl, CURLOPT_PROGRESSFUNCTION, progressCallback);
//		curl_easy_setopt(mCurl, CURLOPT_PROGRESSDATA, feedReader);
		curl_easy_setopt(mCurl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(mCurl, CURLOPT_CONNECTTIMEOUT, 60);
		curl_easy_setopt(mCurl, CURLOPT_TIMEOUT, 120);

		if (!proxy.empty()) {
			curl_easy_setopt(mCurl, CURLOPT_PROXY, proxy.c_str());
		}
	}
}

CURLWrapper::~CURLWrapper()
{
	if (mCurl) {
		curl_easy_cleanup(mCurl);
	}
}

static size_t writeFunctionString (void *ptr, size_t size, size_t nmemb, void *stream)
{
	std::string *s = (std::string*) stream;
	s->append ((char*) ptr, size * nmemb);

	return nmemb * size;
}

CURLcode CURLWrapper::downloadText(const std::string &link, std::string &data)
{
	data.clear();

	if (!mCurl) {
		return CURLE_FAILED_INIT;
	}

	curl_easy_setopt(mCurl, CURLOPT_URL, link.c_str());
	curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, writeFunctionString);
	curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(mCurl, CURLOPT_SSL_VERIFYPEER, false);

	return curl_easy_perform(mCurl);
}

static size_t writeFunctionBinary (void *ptr, size_t size, size_t nmemb, void *stream)
{
	std::vector<unsigned char> *bytes = (std::vector<unsigned char>*) stream;

	std::vector<unsigned char> newBytes;
	newBytes.resize(size * nmemb);
	memcpy(newBytes.data(), ptr, newBytes.size());

	bytes->insert(bytes->end(), newBytes.begin(), newBytes.end());

	return nmemb * size;
}

CURLcode CURLWrapper::downloadBinary(const std::string &link, std::vector<unsigned char> &data)
{
	data.clear();

	if (!mCurl) {
		return CURLE_FAILED_INIT;
	}

	curl_easy_setopt(mCurl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(mCurl, CURLOPT_URL, link.c_str());
	curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, writeFunctionBinary);
	curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, &data);

	return curl_easy_perform(mCurl);
}

long CURLWrapper::longInfo(CURLINFO info)
{
	if (!mCurl) {
		return 0;
	}

	long value;
	curl_easy_getinfo(mCurl, info, &value);

	return value;
}

std::string CURLWrapper::stringInfo(CURLINFO info)
{
	if (!mCurl) {
		return "";
	}

	char *value;
	curl_easy_getinfo(mCurl, info, &value);

	return value ? value : "";
}
