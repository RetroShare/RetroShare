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

#include "XPathWrapper.h"
#include "XMLWrapper.h"

XPathWrapper::XPathWrapper(XMLWrapper &xmlWrapper) : mXMLWrapper(xmlWrapper)
{
	mContext = NULL;
	mResult = NULL;
}

XPathWrapper::~XPathWrapper()
{
	cleanup();
}

void XPathWrapper::cleanup()
{
	if (mResult) {
		xmlXPathFreeObject(mResult);
		mResult = NULL;
	}
	if (mContext) {
		xmlXPathFreeContext(mContext);
		mContext = NULL;
	}
}

bool XPathWrapper::compile(const char *expression)
{
	cleanup();

	xmlDocPtr document = mXMLWrapper.getDocument();
	if (!document) {
		return false;
	}

	mContext = xmlXPathNewContext(document);
	if (!mContext) {
		cleanup();
		return false;
	}

	xmlChar *xmlExpression = NULL;
	if (!mXMLWrapper.convertFromString(expression, xmlExpression)) {
		cleanup();
		return false;
	}
	mResult = xmlXPathEvalExpression(xmlExpression, mContext);
	xmlFree(xmlExpression);

	return true;
}

xmlXPathObjectType XPathWrapper::type()
{
	if (mResult) {
		return mResult->type;
	}

	return XPATH_UNDEFINED;
}

unsigned int XPathWrapper::count()
{
	if (!mResult) {
		return 0;
	}

	if (mResult->type != XPATH_NODESET) {
		return 0;
	}

	if (xmlXPathNodeSetIsEmpty(mResult->nodesetval)) {
		return 0;
	}

	return mResult->nodesetval->nodeNr;
}

xmlNodePtr XPathWrapper::node(unsigned int index)
{
	if (!mResult) {
		return NULL;
	}

	if (mResult->type != XPATH_NODESET) {
		return NULL;
	}

	if (xmlXPathNodeSetIsEmpty(mResult->nodesetval)) {
		return NULL;
	}

	if (index >= (unsigned int) mResult->nodesetval->nodeNr) {
		return NULL;
	}

	return mResult->nodesetval->nodeTab[index];
}
