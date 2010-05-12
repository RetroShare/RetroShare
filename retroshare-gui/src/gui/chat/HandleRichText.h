/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, Thomas Kister
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

/**
 * This file provides helper functions and functors for translating data from/to
 * rich text format and HTML. Its main goal is to facilitate decoding of chat
 * messages, particularly embedding special information into HTML tags.
 */


#ifndef HANDLE_RICH_TEXT_H_
#define HANDLE_RICH_TEXT_H_


#include <QRegExp>
#include <QtXml>


namespace RsChat {


/**
 * The type of embedding we'd like to do
 */
enum EmbeddedType
{
	Ahref,		///< into <a></a>
	Img,		///< into <img/>
};


/**
 * Base class for storing information about a given kind of embedding.
 *
 * Its only constructor is protected so it is impossible to instantiate it, and
 * at the same time derived classes have to provide a type.
 */
class EmbedInHtml
{
protected:
	EmbedInHtml(EmbeddedType newType) :
		myType(newType)
	{}

public:
	const EmbeddedType myType;
	QRegExp myRE;
};


/**
 * This class is used to store information for embedding links into <a></a> tags.
 */
class EmbedInHtmlAhref : public EmbedInHtml
{
public:
	EmbedInHtmlAhref() :
		EmbedInHtml(Ahref)
	{
		myRE.setPattern("(\\bretroshare://[^\\s]*)|(\\bhttps?://[^\\s]*)|(\\bwww\\.[^\\s]*)");
	}
};


/** 
  * This class is used to store information for embedding smileys into <img/> tags.
  *
  * By default the QRegExp the variables are empty, which means it must be
  * filled at runtime, typically when the smileys set is loaded. It can be
  * either done by hand or by using one of the helper methods available.
  *
  * Note: The QHash uses only *one* smiley per key (unlike soon-to-be-upgraded
  * code out there).
  */
class EmbedInHtmlImg : public EmbedInHtml
{
public:
	EmbedInHtmlImg() :
		EmbedInHtml(Img)
	{}

	void InitFromAwkwardHash(const QHash<QString,QString>& hash);

	QHash<QString,QString> smileys;
};


void embedHtml(QDomDocument& doc, QDomElement& currentElement, const EmbedInHtml& embedInfos);


} // namespace RsChat


#endif // HANDLE_RICH_TEXT_H_
