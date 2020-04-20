/*
 * RetroShare
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>
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


#include <cstdio>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "rsurl.h"
#include "serialiser/rstypeserializer.h"
#include "util/rsnet.h"

using namespace std;

RsUrl::RsUrl() : mPort(0), mHasPort(false) {}

RsUrl::RsUrl(const std::string& urlStr) : mPort(0), mHasPort(false)
{ fromString(urlStr); }

RsUrl::RsUrl(const sockaddr_storage& addr): mPort(0), mHasPort(false)
{
	switch(addr.ss_family)
	{
	case AF_INET:  setScheme("ipv4"); break;
	case AF_INET6: setScheme("ipv6"); break;
	default:
	{
		std::string addrDump;
		sockaddr_storage_dump(addr, &addrDump);
		RsErr() << __PRETTY_FUNCTION__ << " got invalid addr: " << addrDump
		        << std::endl;
		return;
	}
	}

	setHost(sockaddr_storage_iptostring(addr));
	setPort(sockaddr_storage_port(addr));
}

RsUrl& RsUrl::fromString(const std::string& urlStr)
{
	size_t endI = urlStr.size()-1;

	size_t schemeEndI = urlStr.find(schemeSeparator);
	if(schemeEndI >= endI)
	{
		mScheme = urlStr;
		return *this;
	}

	mScheme = urlStr.substr(0, schemeEndI);

	size_t hostBeginI = schemeEndI + 3;
	if(hostBeginI >= endI) return *this;

	bool hasSquareBr = (urlStr[hostBeginI] == ipv6WrapOpen[0]);
	size_t hostEndI;
	if(hasSquareBr)
	{
		if(++hostBeginI >= endI) return *this;
		hostEndI = urlStr.find(ipv6WrapClose, hostBeginI);
		mHost = urlStr.substr(hostBeginI, hostEndI - hostBeginI);
		++hostEndI;
	}
	else
	{
		hostEndI =               urlStr.find(pathSeparator, hostBeginI);
		hostEndI = min(hostEndI, urlStr.find(portSeparator, hostBeginI));
		hostEndI = min(hostEndI, urlStr.find(querySeparator, hostBeginI));
		hostEndI = min(hostEndI, urlStr.find(fragmentSeparator, hostBeginI));

		mHost = urlStr.substr(hostBeginI, hostEndI - hostBeginI);
	}

	if( hostEndI >= endI ) return *this;

	mHasPort = (sscanf(&urlStr[hostEndI], ":%hu", &mPort) == 1);

	size_t pathBeginI = urlStr.find(pathSeparator, hostBeginI);
	size_t pathEndI = string::npos;
	if(pathBeginI < endI)
	{
		pathEndI =               urlStr.find(querySeparator, pathBeginI);
		pathEndI = min(pathEndI, urlStr.find(fragmentSeparator, pathBeginI));
		mPath = UrlDecode(urlStr.substr(pathBeginI, pathEndI - pathBeginI));
		if(pathEndI >= endI) return *this;
	}

	size_t queryBeginI = urlStr.find(querySeparator, hostBeginI);
	size_t queryEndI = urlStr.find(fragmentSeparator, hostBeginI);
	if(queryBeginI < endI)
	{
		string qStr = urlStr.substr(queryBeginI+1, queryEndI-queryBeginI-1);

		size_t kPos = 0;
		size_t assPos = qStr.find(queryAssign);
		do
		{
			size_t vEndPos = qStr.find(queryFieldSep, assPos);
			mQuery.insert( std::make_pair( qStr.substr(kPos, assPos-kPos),
			                  UrlDecode(qStr.substr(assPos+1, vEndPos-assPos-1))
			                              ) );
			kPos = vEndPos+1;
			assPos = qStr.find(queryAssign, vEndPos);
		}
		while(assPos < endI);

		if(queryEndI >= endI) return *this;
	}

	size_t fragmentBeginI = urlStr.find(fragmentSeparator, hostBeginI);
	if(fragmentBeginI < endI)
		mFragment = UrlDecode(urlStr.substr(++fragmentBeginI));

	return *this;
}

std::string RsUrl::toString() const
{
	std::string urlStr(mScheme);
	urlStr += schemeSeparator;

	if(!mHost.empty())
	{
		if(mHost.find(ipv6Separator) != string::npos &&
		        mHost[0] != ipv6WrapOpen[0] )
			urlStr += ipv6WrapOpen + mHost + ipv6WrapClose;
		else urlStr += mHost;
	}

	if(mHasPort) urlStr += portSeparator + std::to_string(mPort);

	urlStr += UrlEncode(mPath, pathSeparator);

	bool hasQuery = !mQuery.empty();
	if(hasQuery) urlStr += querySeparator;
	for(auto&& kv : mQuery)
	{
		urlStr += kv.first;
		urlStr += queryAssign;
		urlStr += UrlEncode(kv.second);
		urlStr += queryFieldSep;
	}
	if(hasQuery) urlStr.pop_back();

	if(!mFragment.empty()) urlStr += fragmentSeparator + UrlEncode(mFragment);

	return urlStr;
}

const std::string& RsUrl::scheme() const { return mScheme; }
RsUrl& RsUrl::setScheme(const std::string& scheme)
{
	mScheme = scheme;
	return *this;
}

const std::string& RsUrl::host() const { return mHost; }
RsUrl& RsUrl::setHost(const std::string& host)
{
	mHost = host;
	return *this;
}

bool RsUrl::hasPort() const { return mHasPort; }
uint16_t RsUrl::port(uint16_t def) const
{
	if(mHasPort) return mPort;
	return def;
}
RsUrl& RsUrl::setPort(uint16_t port)
{
	mPort = port;
	mHasPort = true;
	return *this;
}
RsUrl& RsUrl::unsetPort()
{
	mPort = 0;
	mHasPort = false;
	return *this;
}

const std::string& RsUrl::path() const { return mPath; }
RsUrl& RsUrl::setPath(const std::string& path)
{
	mPath = path;
	return *this;
}

const std::map<std::string, std::string>& RsUrl::query() const
{ return mQuery; }
RsUrl& RsUrl::setQuery(const std::map<std::string, std::string>& query)
{
	mQuery = query;
	return *this;
}
RsUrl& RsUrl::setQueryKV(const std::string& key, const std::string& value)
{
	mQuery.insert(std::make_pair(key, value));
	return *this;
}
RsUrl& RsUrl::delQueryK(const std::string& key)
{
	mQuery.erase(key);
	return *this;
}
bool RsUrl::hasQueryK(const std::string& key)
{ return (mQuery.find(key) != mQuery.end()); }
rs_view_ptr<const std::string> RsUrl::getQueryV(const std::string& key)
{
	if(hasQueryK(key)) return &(mQuery.find(key)->second);
	return nullptr;
}

const std::string& RsUrl::fragment() const { return mFragment; }
RsUrl& RsUrl::setFragment(const std::string& fragment)
{
	mFragment = fragment;
	return *this;
}

/*static*/ std::string RsUrl::UrlEncode( const std::string& str,
                                         const std::string& ignore )
{
	ostringstream escaped;
	escaped.fill('0');
	escaped << hex;

	for (string::value_type c : str)
	{
		// Keep alphanumeric and other accepted characters intact
		if ( isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'
		     || ignore.find(c) != string::npos )
		{
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << uppercase;
		escaped << '%' << setw(2) << int((unsigned char) c);
		escaped << nouppercase;
	}

	return escaped.str();
}

/*static*/ std::string RsUrl::UrlDecode(const std::string& str)
{
	ostringstream decoded;

	size_t len = str.size();
	size_t boundary = len-2; // % Encoded char must be at least 2 hex char
	for (size_t i = 0; i < len; ++i)
	{
		if(str[i] == '%' && i < boundary)
		{
			decoded << static_cast<char>(std::stoi(
			                                 str.substr(++i, 2), nullptr, 16 ));
			++i;
		}
		else decoded << str[i];
	}

	return decoded.str();
}

void RsUrl::serial_process( RsGenericSerializer::SerializeJob j,
                            RsGenericSerializer::SerializeContext& ctx )
{
	std::string urlString = toString();
	RS_SERIAL_PROCESS(urlString);
	fromString(urlString);
}

/*static*/ const std::string RsUrl::schemeSeparator("://");
/*static*/ const std::string RsUrl::ipv6WrapOpen("[");
/*static*/ const std::string RsUrl::ipv6Separator(":");
/*static*/ const std::string RsUrl::ipv6WrapClose("]");
/*static*/ const std::string RsUrl::portSeparator(":");
/*static*/ const std::string RsUrl::pathSeparator("/");
/*static*/ const std::string RsUrl::querySeparator("?");
/*static*/ const std::string RsUrl::queryAssign("=");
/*static*/ const std::string RsUrl::queryFieldSep("&");
/*static*/ const std::string RsUrl::fragmentSeparator("#");

