/*******************************************************************************
 * util/MarkdownSyntaxHighlighter.cpp                                          *
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

#include "util/MarkdownSyntaxHighlighter.h"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPair>
#include <QPalette>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>

namespace {

enum BlockState {
	StateNormal = 0,
	StateCodeFence = 1   // inside a ``` or ~~~ code block
};

// Same default colors as in the stylesheets (RSTextBrowser, MimeTextEdit)
const char *DEFAULT_QUOTE_COLORS[] = { "#789922", "#039bd5", "#800000", "#800080",
                                       "#008080", "#b10dc9", "#85144b", "#3d9970" };

typedef QVector<QPair<int, int> > Ranges;

bool overlaps(const Ranges &ranges, int start, int length)
{
	for (const QPair<int, int> &range : ranges)
		if (start < range.first + range.second && range.first < start + length)
			return true;
	return false;
}

} // namespace

MarkdownSyntaxHighlighter::MarkdownSyntaxHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
	for (const char *color : DEFAULT_QUOTE_COLORS) {
		QTextCharFormat format;
		format.setForeground(QColor(color));
		mQuoteFormats.append(format);
	}

	mHeadingFormat.setFontWeight(QFont::Bold);
	mBoldFormat.setFontWeight(QFont::Bold);
	mItalicFormat.setFontItalic(true);

	const QColor dimmed(128, 128, 128); // readable on light and dark themes

	mCodeFormat.setFontFamily(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
	mCodeFormat.setFontFixedPitch(true);
	mCodeFormat.setForeground(dimmed);

	mLinkFormat.setForeground(QGuiApplication::palette().color(QPalette::Link));
	mLinkFormat.setFontUnderline(true);

	mMarkupFormat.setForeground(dimmed);
	mMarkupFormat.setFontWeight(QFont::Bold);
}

QColor MarkdownSyntaxHighlighter::textColorQuote() const
{
	return mQuoteFormats.at(0).foreground().color();
}

QVariant MarkdownSyntaxHighlighter::textColorQuotes() const
{
	QList<QVariant> colors;
	for (const QTextCharFormat &format : mQuoteFormats)
		colors.append(QVariant(format.foreground().color()));
	return QVariant(colors);
}

void MarkdownSyntaxHighlighter::setTextColorQuote(QColor textColorQuote)
{
	mQuoteFormats[0].setForeground(textColorQuote);
	rehighlight();
}

void MarkdownSyntaxHighlighter::setTextColorQuotes(QVariant textColorQuotes)
{
	const QStringList parList = textColorQuotes.toStringList();
	if (parList.size() == 2 && parList.at(0).toLower() == "colorlist") {
		QStringList colors;
		for (const QString &color : parList.at(1).split(QLatin1Char(' ')))
			if (!color.isEmpty())
				colors.append(color);

		if (!colors.isEmpty()) {
			mQuoteFormats.clear();
			for (const QString &color : colors) {
				QTextCharFormat format;
				format.setForeground(QColor(color));
				mQuoteFormats.append(format);
			}
		}
	}

	rehighlight();
}

void MarkdownSyntaxHighlighter::mergeFormat(int start, int count, const QTextCharFormat &fmt)
{
	const int end = start + count;
	int i = start;
	while (i < end) {
		QTextCharFormat current = format(i);
		int j = i + 1;
		while (j < end && format(j) == current)
			++j;
		current.merge(fmt);
		setFormat(i, j - i, current);
		i = j;
	}
}

void MarkdownSyntaxHighlighter::highlightBlock(const QString &text)
{
	static const QRegularExpression fenceRe("^ {0,3}(`{3,}|~{3,})");
	static const QRegularExpression quoteRe("^ {0,3}((?:>[ \\t]?)+)");
	static const QRegularExpression headingRe("^ {0,3}#{1,6}(?:[ \\t].*)?$");
	static const QRegularExpression ruleRe("^ {0,3}([-*_])(?:[ \\t]*\\1){2,}[ \\t]*$");
	static const QRegularExpression listRe("^[ \\t]*(?:[-*+]|\\d{1,9}[.)])(?=[ \\t])");
	static const QRegularExpression codeRe("(?<!`)(`+)(?!`)(.+?)(?<!`)\\1(?!`)");
	static const QRegularExpression boldRe("(\\*\\*|__)(?=\\S)(.+?)(?<=\\S)\\1");
	static const QRegularExpression italicRe("(?<![*\\w])\\*(?=[^*\\s])(.+?)(?<=[^*\\s])\\*(?![*\\w])"
	                                         "|(?<![_\\w])_(?=[^_\\s])(.+?)(?<=[^_\\s])_(?![_\\w])");
	static const QRegularExpression linkRe("!?\\[[^\\]\\n]*\\]\\([^)\\n]*\\)"
	                                       "|<(?:https?|retroshare)://[^>\\s]+>"
	                                       "|(?:https?|retroshare)://[^\\s<>()\\[\\]]+",
	                                       QRegularExpression::CaseInsensitiveOption);

	/* fenced code blocks */
	const bool inFence = previousBlockState() == StateCodeFence;
	const bool fenceLine = fenceRe.match(text).hasMatch();

	if (inFence || fenceLine) {
		setFormat(0, text.length(), mCodeFormat);
		// a fence line opens a block, or closes the one we are in
		setCurrentBlockState((inFence != fenceLine) ? StateCodeFence : StateNormal);
		return;
	}
	setCurrentBlockState(StateNormal);

	if (text.isEmpty())
		return;

	/* block level */
	QRegularExpressionMatch match = quoteRe.match(text);
	if (match.hasMatch()) {
		const int level = qMax(1, int(match.captured(1).count(QLatin1Char('>'))));
		setFormat(0, text.length(), mQuoteFormats.at(qMin(level, mQuoteFormats.size()) - 1));
	}

	if (ruleRe.match(text).hasMatch()) {
		mergeFormat(0, text.length(), mMarkupFormat);
		return;
	}

	if (headingRe.match(text).hasMatch())
		mergeFormat(0, text.length(), mHeadingFormat);

	match = listRe.match(text);
	if (match.hasMatch())
		mergeFormat(match.capturedStart(), match.capturedLength(), mMarkupFormat);

	/* inline: code spans first, nothing else is highlighted inside them */
	Ranges codeSpans;
	QRegularExpressionMatchIterator it = codeRe.globalMatch(text);
	while (it.hasNext()) {
		match = it.next();
		codeSpans.append(qMakePair(match.capturedStart(), match.capturedLength()));
	}

	it = linkRe.globalMatch(text);
	while (it.hasNext()) {
		match = it.next();
		if (!overlaps(codeSpans, match.capturedStart(), match.capturedLength()))
			mergeFormat(match.capturedStart(), match.capturedLength(), mLinkFormat);
	}

	it = boldRe.globalMatch(text);
	while (it.hasNext()) {
		match = it.next();
		if (!overlaps(codeSpans, match.capturedStart(), match.capturedLength()))
			mergeFormat(match.capturedStart(), match.capturedLength(), mBoldFormat);
	}

	it = italicRe.globalMatch(text);
	while (it.hasNext()) {
		match = it.next();
		if (!overlaps(codeSpans, match.capturedStart(), match.capturedLength()))
			mergeFormat(match.capturedStart(), match.capturedLength(), mItalicFormat);
	}

	for (const QPair<int, int> &span : codeSpans)
		mergeFormat(span.first, span.second, mCodeFormat);
}
