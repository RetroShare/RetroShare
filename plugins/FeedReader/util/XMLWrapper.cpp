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

#include <iostream>
#include <string.h>

#include "XMLWrapper.h"
#include "XPathWrapper.h"

XMLWrapper::XMLWrapper()
{
	mDocument = NULL;
	mCharEncodingHandler = xmlFindCharEncodingHandler ("UTF8");

	if (!mCharEncodingHandler) {
		/* no encoding handler found */
		std::cerr << "XMLWrapper::XMLWrapper - no encoding handler found" << std::endl;
	}
}

XMLWrapper::~XMLWrapper()
{
	cleanup();
	xmlCharEncCloseFunc(mCharEncodingHandler);
}

void XMLWrapper::trimString(std::string &string)
{
	/* trim left */
	std::string::size_type find = string.find_first_not_of(" \t\r\n");
	if (find != std::string::npos) {
		string.erase(0, find);

		/* trim right */
		find = string.find_last_not_of(" \t\r\n");
		if (find != std::string::npos) {
			string.erase(find + 1);
		}
	} else {
		string.clear();
	}
}


XMLWrapper &XMLWrapper::operator=(const XMLWrapper &xml)
{
	cleanup();

	const xmlDocPtr document = xml.getDocument();
	if (document) {
		mDocument = xmlCopyDoc(document, 1);
	}

	return *this;
}

void XMLWrapper::cleanup()
{
	if (mDocument) {
		xmlFreeDoc(mDocument);
		mDocument = NULL;
	}
}

bool XMLWrapper::convertToString(const xmlChar *xmlText, std::string &text)
{
	bool result = false;

	xmlBufferPtr in = xmlBufferCreateStatic((void*) xmlText, xmlStrlen(xmlText));
	xmlBufferPtr out = xmlBufferCreate();
	int ret = xmlCharEncOutFunc(mCharEncodingHandler, out, in);
	if (ret >= 0) {
		result = true;
		text = (char*) xmlBufferContent(out);
	}

	xmlBufferFree(in);
	xmlBufferFree(out);

	return result;
}

bool XMLWrapper::convertFromString(const char *text, xmlChar *&xmlText)
{
	bool result = false;

	xmlBufferPtr in = xmlBufferCreateStatic((void*) text, strlen(text));
	xmlBufferPtr out = xmlBufferCreate();
	int ret = xmlCharEncInFunc(mCharEncodingHandler, out, in);
	if (ret >= 0) {
		result = true;
#if LIBXML_VERSION >= 20800
		xmlText = xmlBufferDetach(out);
#else
		xmlText = xmlStrdup(xmlBufferContent(out));
#endif
	}

	xmlBufferFree(in);
	xmlBufferFree(out);

	return result;
}

xmlDocPtr XMLWrapper::getDocument() const
{
	return mDocument;
}

xmlNodePtr XMLWrapper::getRootElement() const
{
	if (mDocument) {
		return xmlDocGetRootElement(mDocument);
	}

	return NULL;
}

bool XMLWrapper::readXML(const char *xml)
{
	cleanup();

	mDocument = xmlReadDoc(BAD_CAST xml, "", NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_COMPACT | XML_PARSE_NOENT | XML_PARSE_NOCDATA);
	if (mDocument) {
		return true;
	}

	return false;
}

bool XMLWrapper::getContent(xmlNodePtr node, std::string &content, bool trim)
{
	content.clear();

	if (!node) {
		return false;
	}

	xmlChar *xmlContent = xmlNodeGetContent(node);
	if (!xmlContent) {
		return true;
	}

	bool result = convertToString(xmlContent, content);
	xmlFree(xmlContent);

	if (result && trim) {
		trimString(content);
	}

	return result;
}

bool XMLWrapper::setContent(xmlNodePtr node, const char *content)
{
	if (!node) {
		return false;
	}

	xmlChar *xmlContent;
	if (!convertFromString(content, xmlContent)) {
		return false;
	}

	xmlNodeSetContent(node, xmlContent);
	xmlFree(xmlContent);

	return true;
}

bool XMLWrapper::nodeDump(xmlNodePtr node, std::string &content, bool trim)
{
	content.clear();

	if (!mDocument) {
		return false;
	}

	if (!node) {
		return false;
	}

	bool result = false;

	xmlBufferPtr buffer = xmlBufferCreate();
	if (buffer) {
		xmlOutputBufferPtr outputBuffer = xmlOutputBufferCreateBuffer(buffer, NULL);
		if (outputBuffer) {
			xmlNodeDumpOutput(outputBuffer, mDocument, node, 0, 0, "UTF8");
			xmlOutputBufferClose(outputBuffer);
			outputBuffer = NULL;

			result = convertToString(buffer->content, content);

			if (result && trim) {
				trimString(content);
			}
		}
		xmlBufferFree(buffer);
		buffer = NULL;
	}

	return result;
}

std::string XMLWrapper::nodeName(xmlNodePtr node)
{
	std::string name;

	if (node) {
		convertToString(node->name, name);
	}

	return name;
}

std::string XMLWrapper::attrName(xmlAttrPtr attr)
{
	std::string name;

	if (attr) {
		convertToString(attr->name, name);
	}

	return name;
}

xmlNodePtr XMLWrapper::findNode(xmlNodePtr node, const char *name, bool children)
{
	if (node->name) {
		if (xmlStrEqual(node->name, BAD_CAST name)) {
			return node;
		}
	}

	xmlNodePtr nodeFound = NULL;
	if (children) {
		if (node->children) {
			nodeFound = findNode(node->children, name, children);
			if (nodeFound) {
				return nodeFound;
			}
		}
	}

	if (node->next) {
		nodeFound = findNode(node->next, name, children);
		if (nodeFound) {
			return nodeFound;
		}
	}

	return NULL;
}

bool XMLWrapper::getChildText(xmlNodePtr node, const char *childName, std::string &text)
{
	if (node == NULL || node->children == NULL) {
		return false;
	}

	xmlNodePtr child = findNode(node->children, childName, true);
	if (!child) {
		return false;
	}

	if (child->type != XML_ELEMENT_NODE) {
		return false;
	}

	if (!child->children) {
		return false;
	}

	if (getAttr(child, "type") == "xhtml") {
		/* search div */
		xmlNodePtr div = findNode(child->children, "div", false);
		if (!div) {
			return false;
		}

		return nodeDump(div, text, true);
	}

	if (child->children->type != XML_TEXT_NODE) {
		return false;
	}

	if (child->children->content) {
		return convertToString(child->children->content, text);
	}

	return true;
}

std::string XMLWrapper::getAttr(xmlNodePtr node, xmlAttrPtr attr)
{
	return getAttr(node, (const char*) attr->name);
}

std::string XMLWrapper::getAttr(xmlNodePtr node, const char *name)
{
	if (!node || !name) {
		return "";
	}

	std::string value;

	xmlChar *xmlValue = xmlGetProp(node, BAD_CAST name);
	if (xmlValue) {
		convertToString(xmlValue, value);
		xmlFree(xmlValue);
	}

	return value;
}

bool XMLWrapper::setAttr(xmlNodePtr node, const char *name, const char *value)
{
	if (!node || !name) {
		return false;
	}

	xmlChar *xmlValue = NULL;
	if (!convertFromString(value, xmlValue)) {
		return false;
	}

	xmlAttrPtr xmlAttr = xmlSetProp (node, BAD_CAST name, xmlValue);
	xmlFree(xmlValue);

	return xmlAttr != NULL;
}

XPathWrapper *XMLWrapper::createXPath()
{
	if (mDocument) {
		return new XPathWrapper(*this);
	}

	return NULL;
}
