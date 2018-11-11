/*******************************************************************************
 * plugins/FeedReader/util/XPathWrapper.h                                      *
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

#ifndef XPATHWRAPPER
#define XPATHWRAPPER

#include <libxml/xpath.h>

class XMLWrapper;

class XPathWrapper
{
	friend class XMLWrapper;

public:
	~XPathWrapper();

	void cleanup();

	bool compile(const char *expression);

	xmlXPathObjectType type();

	unsigned int count();
	xmlNodePtr node(unsigned int index);

protected:
	explicit XPathWrapper(XMLWrapper &xmlWrapper);

	XMLWrapper &mXMLWrapper;
	xmlXPathContextPtr mContext;
	xmlXPathObjectPtr mResult;
};

#endif
