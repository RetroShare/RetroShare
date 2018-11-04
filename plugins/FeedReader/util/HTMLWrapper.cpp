/*******************************************************************************
 * plugins/FeedReader/util/HTMLWrapper.cc                                      *
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

#include <string.h>

#include "HTMLWrapper.h"
#include <libxml/HTMLtree.h>

HTMLWrapper::HTMLWrapper() : XMLWrapper()
{
}

bool HTMLWrapper::readHTML(const char *html, const char *url)
{
	cleanup();

	handleError(true, mLastErrorString);
	mDocument = htmlReadMemory(html, strlen(html), url, "", /*HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | */HTML_PARSE_COMPACT | HTML_PARSE_NONET | HTML_PARSE_NOBLANKS);
	handleError(false, mLastErrorString);

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
	handleError(true, mLastErrorString);
	htmlDocDumpMemoryFormat(mDocument, &newHtml, &newHtmlSize, 0);
	handleError(false, mLastErrorString);

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
