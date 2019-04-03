/*******************************************************************************
 * plugins/FeedReader/util/XMLWrapper.h                                        *
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

#ifndef XMLWRAPPER
#define XMLWRAPPER

#include <string>
#include <libxml/parser.h>

class XPathWrapper;

class XMLWrapper
{
public:
	XMLWrapper();
	~XMLWrapper();

	// find better place
	static void trimString(std::string &string);

	XMLWrapper &operator=(const XMLWrapper &xml);

	void cleanup();
	std::string lastError() { return mLastErrorString; }

	bool readXML(const char *xml);

	xmlDocPtr getDocument() const;
	xmlNodePtr getRootElement() const;

	bool convertToString(const xmlChar *xmlText, std::string &text);
	bool convertFromString(const char *text, xmlChar *&xmlText);

	std::string nodeName(xmlNodePtr node);
	std::string attrName(xmlAttrPtr attr);

	xmlNodePtr findNode(xmlNodePtr node, const char *name, bool children = false);
	bool getChildText(xmlNodePtr node, const char *childName, std::string &text);

	bool getContent(xmlNodePtr node, std::string &content, bool trim);
	bool setContent(xmlNodePtr node, const char *content);

	bool nodeDump(xmlNodePtr node, std::string &content, bool trim);

	std::string getAttr(xmlNodePtr node, xmlAttrPtr attr);
	std::string getAttr(xmlNodePtr node, const char *name);
	bool setAttr(xmlNodePtr node, const char *name, const char *value);

	XPathWrapper *createXPath();

	bool transform(const XMLWrapper &style, XMLWrapper &result);

protected:
	void attach(xmlDocPtr document);
	void handleError(bool init, std::string &errorString);

protected:
	xmlDocPtr mDocument;
	xmlCharEncodingHandlerPtr mCharEncodingHandler;
	std::string mLastErrorString;
};

#endif
