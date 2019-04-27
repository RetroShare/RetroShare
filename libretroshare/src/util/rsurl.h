#pragma once
/*
 * RetroShare
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

#include <string>
#include <map>

#include "serialiser/rsserializable.h"

/**
 * Very simplistic and minimal URL helper class for RetroShare, after looking
 * for a small and self-contained C/C++ URL parsing and manipulation library,
 * haven't found nothing satisfactory except for implementation like QUrl that
 * rely on bigger library.
 * ATM this implementation is not standard compliant and doesn't aim to be.
 * Improvements to this are welcome.
 *
 * Anyway this should support most common URLs of the form
 * scheme://host[:port][/path][?query][#fragment]
 */
struct RsUrl : RsSerializable
{
	RsUrl();
	RsUrl(const std::string& urlStr);

	RsUrl& fromString(const std::string& urlStr);
	std::string toString() const;

	const std::string& scheme() const;
	RsUrl& setScheme(const std::string& scheme);

	const std::string& host() const;
	RsUrl& setHost(const std::string& host);

	bool hasPort() const;
	uint16_t port(uint16_t def = 0) const;
	RsUrl& setPort(uint16_t port);
	RsUrl& unsetPort();

	const std::string& path() const;
	RsUrl& setPath(const std::string& path);

	const std::map<std::string, std::string>& query() const;
	RsUrl& setQuery(const std::map<std::string, std::string>& query);
	RsUrl& setQueryKV(const std::string& key, const std::string& value);
	RsUrl& delQueryK(const std::string& key);
	bool hasQueryK(const std::string& key);
	const std::string* getQueryV(const std::string& key);

	const std::string& fragment() const;
	RsUrl& setFragment(const std::string& fragment);

	static std::string UrlEncode(const std::string& str,
	                             const std::string& ignoreChars = "");
	static std::string UrlDecode(const std::string& str);

	inline bool operator<(const RsUrl& rhs) const
	{ return toString() < rhs.toString(); }
	inline bool operator>(const RsUrl& rhs) const
	{ return toString() > rhs.toString(); }
	inline bool operator<=(const RsUrl& rhs) const
	{ return toString() <= rhs.toString(); }
	inline bool operator>=(const RsUrl& rhs) const
	{ return toString() >= rhs.toString(); }
	inline bool operator==(const RsUrl& rhs) const
	{ return toString() == rhs.toString(); }
	inline bool operator!=(const RsUrl& rhs) const
	{ return toString() != rhs.toString(); }

	/// @see RsSerializable
	virtual void serial_process(RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx);

	static const std::string schemeSeparator;
	static const std::string ipv6WrapOpen;
	static const std::string ipv6Separator;
	static const std::string ipv6WrapClose;
	static const std::string portSeparator;
	static const std::string pathSeparator;
	static const std::string querySeparator;
	static const std::string queryAssign;
	static const std::string queryFieldSep;
	static const std::string fragmentSeparator;

private:

	std::string mScheme;
	std::string mHost;
	uint16_t mPort;
	bool mHasPort;
	std::string mPath;
	std::map<std::string, std::string> mQuery;
	std::string mFragment;
};

