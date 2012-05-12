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

/* Flags for RsHtml::formatText */
#define RSHTML_FORMATTEXT_EMBED_SMILEYS  1
#define RSHTML_FORMATTEXT_EMBED_LINKS    2
#define RSHTML_FORMATTEXT_REMOVE_FONT    4
#define RSHTML_FORMATTEXT_REMOVE_COLOR   8
#define RSHTML_FORMATTEXT_CLEANSTYLE     (RSHTML_FORMATTEXT_REMOVE_FONT | RSHTML_FORMATTEXT_REMOVE_COLOR)
#define RSHTML_FORMATTEXT_OPTIMIZE      16
#define RSHTML_FORMATTEXT_REPLACE_LINKS 32

/* Flags for RsHtml::formatText */
#define RSHTML_OPTIMIZEHTML_REMOVE_FONT    2
#define RSHTML_OPTIMIZEHTML_REMOVE_COLOR   1

class QTextEdit;
class QTextDocument;
class QDomDocument;
class QDomElement;
class EmbedInHtml;
class RetroShareLink;

class RsHtml
{
public:
	RsHtml();

	static void    initEmoticons(const QHash< QString, QString >& hash);

	QString formatText(QTextDocument *textDocument, const QString &text, ulong flag);
	static bool    findAnchors(const QString &text, QStringList& urls);

	static void    optimizeHtml(QTextEdit *textEdit, QString &text, unsigned int flag = 0);
	static void    optimizeHtml(QString &text, unsigned int flag = 0);
	static QString toHtml(QString text, bool realHtml = true);

protected:
	void embedHtml(QTextDocument *textDocument, QDomDocument &doc, QDomElement &currentElement, EmbedInHtml& embedInfos, ulong flag);
	void replaceAnchorWithImg(QDomDocument& doc, QDomElement &element, QTextDocument *textDocument, const RetroShareLink &link);

	virtual bool   canReplaceAnchor(QDomDocument &doc, QDomElement &element, const RetroShareLink &link);
	virtual void   anchorTextForImg(QDomDocument &doc, QDomElement &element, const RetroShareLink &link, QString &text);
	virtual void   anchorStylesheetForImg(QDomDocument &doc, QDomElement &element, const RetroShareLink &link, QString &styleSheet);
};

#endif // HANDLE_RICH_TEXT_H_
