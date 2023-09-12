/*******************************************************************************
 * util/HandleRichText.h                                                       *
 *                                                                             *
 * Copyright (c) 2010 Thomas Kister    <retroshare.project@gmail.com>          *
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

#include <gui/common/RSTextBrowser.h>

/**
 * This file provides helper functions and functors for translating data from/to
 * rich text format and HTML. Its main goal is to facilitate decoding of chat
 * messages, particularly embedding special information into HTML tags.
 */

#ifndef HANDLE_RICH_TEXT_H_
#define HANDLE_RICH_TEXT_H_

/* Flags for RsHtml::formatText */
#define RSHTML_FORMATTEXT_EMBED_SMILEYS       0x0001//1
#define RSHTML_FORMATTEXT_EMBED_LINKS         0x0002//2
#define RSHTML_FORMATTEXT_OPTIMIZE            0x0004//4
#define RSHTML_FORMATTEXT_REPLACE_LINKS       0x0008//8
#define RSHTML_FORMATTEXT_REMOVE_COLOR        0x0010//16
#define RSHTML_FORMATTEXT_FIX_COLORS          0x0020//32	/* Make text readable */
#define RSHTML_FORMATTEXT_REMOVE_FONT_WEIGHT  0x0040//64	/* Remove bold */
#define RSHTML_FORMATTEXT_REMOVE_FONT_STYLE   0x0080//128	/* Remove italics */
#define RSHTML_FORMATTEXT_REMOVE_FONT_FAMILY  0x0100//256
#define RSHTML_FORMATTEXT_REMOVE_FONT_SIZE    0x0200//512
#define RSHTML_FORMATTEXT_REMOVE_FONT         (RSHTML_FORMATTEXT_REMOVE_FONT_WEIGHT | RSHTML_FORMATTEXT_REMOVE_FONT_STYLE | RSHTML_FORMATTEXT_REMOVE_FONT_FAMILY | RSHTML_FORMATTEXT_REMOVE_FONT_SIZE)
#define RSHTML_FORMATTEXT_CLEANSTYLE          (RSHTML_FORMATTEXT_REMOVE_FONT | RSHTML_FORMATTEXT_REMOVE_COLOR)
#define RSHTML_FORMATTEXT_NO_EMBED            0x0400//1024
#define RSHTML_FORMATTEXT_USE_CMARK           0x0800//2048
/* Flags for RsHtml::optimizeHtml */
#define RSHTML_OPTIMIZEHTML_MASK              (RSHTML_FORMATTEXT_CLEANSTYLE | RSHTML_FORMATTEXT_FIX_COLORS | RSHTML_FORMATTEXT_OPTIMIZE)

class QTextEdit;
class QTextDocument;
class QDomDocument;
class QDomElement;
class EmbedInHtml;
class RetroShareLink;
class QTextCursor;

class RsHtml
{
public:
	RsHtml();

	static void    initEmoticons(const QHash<QString, QPair<QVector<QString>, QHash<QString, QString> > > &hash);

	QString formatText(QTextDocument *textDocument, const QString &text, ulong flag, const QColor &backgroundColor = Qt::white, qreal desiredContrast = 1.0, int desiredMinimumFontSize = 10);
	static bool    findAnchors(const QString &text, QStringList& urls);

	static void    optimizeHtml(QTextEdit *textEdit, QString &text, unsigned int flag = 0);
	static void    optimizeHtml(QString &text, unsigned int flag = 0, const QColor &backgroundColor = Qt::white, qreal desiredContrast = 1.0, int desiredMinimumFontSize = 10);
	static QString toHtml(QString text, bool realHtml = true);

	static bool    makeEmbeddedImage(const QString &fileName, QString &embeddedImage, const int maxPixels, const int maxBytes = -1);
    static bool    makeEmbeddedImage(const QImage &originalImage, QString &embeddedImage, const int maxPixels, const int maxBytes = -1);

	static QString plainText(const QString &text);
	static QString plainText(const std::string &text);

	static QString makeQuotedText(RSTextBrowser* browser);
	static void insertSpoilerText(QTextCursor cursor);
	static void findBestColor(QString &val, const QColor &backgroundColor = Qt::white, qreal desiredContrast = 1.0);

protected:
	void embedHtml(QTextDocument *textDocument, QDomDocument &doc, QDomElement &currentElement, EmbedInHtml& embedInfos, ulong flag);
	void replaceAnchorWithImg(QDomDocument& doc, QDomElement &element, QTextDocument *textDocument, const RetroShareLink &link);
	void filterEmbeddedImages(QDomDocument &doc, QDomElement &currentElement);

	virtual bool   canReplaceAnchor(QDomDocument &doc, QDomElement &element, const RetroShareLink &link);
	virtual void   anchorTextForImg(QDomDocument &doc, QDomElement &element, const RetroShareLink &link, QString &text);
	virtual void   anchorStylesheetForImg(QDomDocument &doc, QDomElement &element, const RetroShareLink &link, QString &styleSheet);

private:
	int indexInWithValidation(QRegExp &rx, const QString &text, EmbedInHtml &embedInfos, int pos = 0);
};

#endif // HANDLE_RICH_TEXT_H_
