/*
 * libretroshare/src/dht: opendhtstr.cc
 *
 * Interface with OpenDHT for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <string>
#include <sstream>

#include "opendhtstr.h"
#include "b64.h"

std::string createHttpHeader(std::string host, uint16_t port, std::string agent, uint32_t length)
{
	std::ostringstream req;

	req << "POST / HTTP/1.0\r\n";
	req << "Host: " << host << ":" << port << "\r\n";
	req << "User-Agent: " << agent << "\r\n";
	req << "Content-Type: text/xml\r\n";
	req << "Content-Length: " << length << "\r\n";
	req << "\r\n";

	return req.str();
};

std::string createOpenDHT_put(std::string key, std::string value, uint32_t ttl, std::string client)
{
	std::ostringstream req;

	req << "<?xml version=\"1.0\"?>" << std::endl;
	req << "<methodCall>" << std::endl;
	req << "\t<methodName>put</methodName>" << std::endl;
	req << "\t<params>" << std::endl;

	req << "\t\t<param><value><base64>";
	req << convertToBase64(key);
	req << "</base64></value></param>" << std::endl;

	req << "\t\t<param><value><base64>";
	req << convertToBase64(value);
	req << "</base64></value></param>" << std::endl;

	req << "\t\t<param><value><int>";
	req << ttl;
	req << "</int></value></param>" << std::endl;

	req << "\t\t<param><value><string>";
	req << client;
	req << "</string></value></param>" << std::endl;

	req << "\t</params>" << std::endl;
	req << "</methodCall>" << std::endl;

	return req.str();
}


std::string createOpenDHT_get(std::string key, uint32_t maxresponse, std::string client)
{
	std::ostringstream req;

	req << "<?xml version=\"1.0\"?>" << std::endl;
	req << "<methodCall>" << std::endl;
	req << "\t<methodName>get</methodName>" << std::endl;
	req << "\t<params>" << std::endl;

	/* key */
	req << "\t\t<param><value><base64>";
	req << convertToBase64(key);
	req << "</base64></value></param>" << std::endl;

	/* max response */
	req << "\t\t<param><value><int>";
	req << maxresponse;
	req << "</int></value></param>" << std::endl;

	/* placemark (NULL) */
	req << "\t\t<param><value><base64>";
	req << "</base64></value></param>" << std::endl;

	req << "\t\t<param><value><string>";
	req << client;
	req << "</string></value></param>" << std::endl;

	req << "\t</params>" << std::endl;
	req << "</methodCall>" << std::endl;

	return req.str();
}




