/*******************************************************************************
 * util/MarkdownSyntaxHighlighter.h                                            *
 *                                                                             *
 * Copyright (c) 2026 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef MARKDOWNSYNTAXHIGHLIGHTER_H
#define MARKDOWNSYNTAXHIGHLIGHTER_H

#include <QList>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVariant>

/**
 * @brief Syntax highlighter for Markdown source text (MarkdownTextEdit).
 *
 * - > quotes use the same colors per nesting level as RsSyntaxHighlighter
 *   (qproperty-textColorQuotes in the stylesheets)
 * - # headings and **bold** are bold, *italic* / _italic_ are italic
 * - `code` and ``` code blocks ``` use a fixed pitch font
 * - [links](...), ![images](...), <autolinks> and bare URLs use the link color
 * - list markers and --- rules are dimmed
 */
class MarkdownSyntaxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit MarkdownSyntaxHighlighter(QTextDocument *document);

	QColor textColorQuote() const;
	QVariant textColorQuotes() const;

public slots:
	void setTextColorQuote(QColor textColorQuote);
	/** Same format as RsSyntaxHighlighter: ColorList(#rrggbb #rrggbb ...) */
	void setTextColorQuotes(QVariant textColorQuotes);

protected:
	void highlightBlock(const QString &text) override;

private:
	/* Merge fmt into the formats already set, so quote colors, bold and
	 * italic can be combined */
	void mergeFormat(int start, int count, const QTextCharFormat &fmt);

	QList<QTextCharFormat> mQuoteFormats;
	QTextCharFormat mHeadingFormat;
	QTextCharFormat mBoldFormat;
	QTextCharFormat mItalicFormat;
	QTextCharFormat mCodeFormat;
	QTextCharFormat mLinkFormat;
	QTextCharFormat mMarkupFormat;
};

#endif // MARKDOWNSYNTAXHIGHLIGHTER_H
