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

#ifndef XMLWRAPPER
#define XMLWRAPPER

#include <string>
#include <libxml/parser.h>

class XMLWrapper
{
public:
	XMLWrapper();
	~XMLWrapper();

	void cleanup();

	bool readXML(const char *xml);

	std::string nodeName(xmlNodePtr node);
	std::string attrName(xmlAttrPtr attr);

	xmlNodePtr findNode(xmlNodePtr node, const char *name, bool children = false);
	bool getChildText(xmlNodePtr node, const char *childName, std::string &text);

	bool getContent(xmlNodePtr node, std::string &content);

	std::string getAttr(xmlNodePtr node, xmlAttrPtr attr);
	std::string getAttr(xmlNodePtr node, const char *name);
	bool setAttr(xmlNodePtr node, const char *name, const char *value);

	xmlNodePtr getRootElement();

protected:
	xmlDocPtr mDocument;
	xmlCharEncodingHandlerPtr mCharEncodingHandler;

	bool convertToString(const xmlChar *xmlText, std::string &text);
	bool convertFromString(const char *text, xmlChar *&xmlText);
};

#endif 
