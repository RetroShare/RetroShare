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

#include <string.h>

#include "HTMLWrapper.h"
#include <libxml/HTMLtree.h>

HTMLWrapper::HTMLWrapper() : XMLWrapper()
{
}

bool HTMLWrapper::readHTML(const char *html, const char *url)
{
	cleanup();

	mDocument = htmlReadMemory(html, strlen(html), url, "", HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_COMPACT | HTML_PARSE_NONET | HTML_PARSE_NOBLANKS);
	if (mDocument) {
		return true;
	}

	return false;
}

bool HTMLWrapper::saveHTML(std::string &html)
{
	if (!mDocument) {
		return false;
	}

	xmlChar *newHtml = NULL;
	int newHtmlSize = 0;
	htmlDocDumpMemoryFormat(mDocument, &newHtml, &newHtmlSize, 0);
	if (newHtml) {
		convertToString(newHtml, html);
		xmlFree(newHtml);

		return true;
	}

	return false;
}

bool HTMLWrapper::createHTML()
{
	/* easy way */
	return readHTML("<html><body></body></html>", "");
}

xmlNodePtr HTMLWrapper::getBody()
{
	xmlNodePtr root = getRootElement();
	if (!root) {
		return NULL;
	}
	return findNode(root->children, "body", false);
}
