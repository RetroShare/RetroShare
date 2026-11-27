/*******************************************************************************
 * util/RsSyntaxHighlighter.h                                                  *
 *                                                                             *
 * Copyright (c) 2014 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef RSSYNTAXHIGHLIGHTER_H
#define RSSYNTAXHIGHLIGHTER_H

#include <QObject>
#include <QSyntaxHighlighter>
#include <QTextBlockFormat>
#include <QTextEdit>

class RsSyntaxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorQuote READ textColorQuote WRITE setTextColorQuote)
	Q_PROPERTY(QVariant textColorQuotes READ textColorQuotes WRITE setTextColorQuotes)

public:
	RsSyntaxHighlighter(QTextEdit *parent = 0);
	QColor textColorQuote () const;
	QVariant textColorQuotes() const;

	/** Quote nesting level of a rendered block (Markdown / HTML blockquote),
	 *  0 when the block is not a quote */
	static int quoteLevel(const QTextBlockFormat &blockFormat);
	/** Left indent of one quote level of a rendered block (px): compact for
	 *  Markdown rendered with cmark, 40 for other HTML / Qt's Markdown */
	static int quoteIndent(const QTextBlockFormat &blockFormat);
	/** Color of a quote level (1 = top level) from textColorQuotes */
	QColor quoteColor(int level) const;

public slots:
	void setTextColorQuote (QColor textColorQuote);
	void setTextColorQuotes(QVariant textColorQuotes);

protected:
	void highlightBlock(const QString &text);

private:
	QList<QTextCharFormat> quotationFormats;
};

#endif // RSSYNTAXHIGHLIGHTER_H
