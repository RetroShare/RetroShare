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

#ifndef CURLWRAPPER
#define CURLWRAPPER

#include <string>
#include <vector>
#include <curl/curl.h>

class CURLWrapper
{
public:
	CURLWrapper(const std::string &proxy);
	~CURLWrapper();

	CURLcode downloadText(const std::string &link, std::string &data);
	CURLcode downloadBinary(const std::string &link, std::vector<unsigned char> &data);

	long responseCode() { return longInfo(CURLINFO_RESPONSE_CODE); }
	std::string contentType() { return stringInfo(CURLINFO_CONTENT_TYPE); }
	std::string effectiveUrl() { return stringInfo(CURLINFO_EFFECTIVE_URL); }

protected:
	long longInfo(CURLINFO info);
	std::string stringInfo(CURLINFO info);

private:
	CURL *mCurl;
};

#endif 
