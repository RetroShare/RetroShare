/*******************************************************************************
 * util/RsSyntaxHighlighter.cpp                                                *
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

#include <QRegularExpression>
#include <QTextBlock>

#include "RsSyntaxHighlighter.h"
#include "util/RsMarkdown.h"

RsSyntaxHighlighter::RsSyntaxHighlighter(QTextEdit *parent)
	: QSyntaxHighlighter(parent)
{
	quotationFormats.append(QTextCharFormat());
}
QColor RsSyntaxHighlighter::textColorQuote () const
{
	return quotationFormats.at(0).foreground().color();
}
QVariant RsSyntaxHighlighter::textColorQuotes() const
{
	QList<QVariant> l;
	foreach(auto i, quotationFormats)
		l.append(QVariant(i.foreground().color()));

	return QVariant(l);
}

void RsSyntaxHighlighter::setTextColorQuote(QColor textColorQuote)
{
	quotationFormats[0].setForeground(textColorQuote);
	this->rehighlight();
}

void RsSyntaxHighlighter::setTextColorQuotes(QVariant textColorQuotes)
{
	QStringList parList = textColorQuotes.toStringList();
	if ((parList.size() == 2) && (parList.at(0).toLower() == "colorlist"))
	{
		QStringList colList = parList.at(1).split(" ");
		quotationFormats.clear();
		for(int i = 0; i < colList.size(); i++)
		{
			quotationFormats.append(QTextCharFormat());
			quotationFormats[i].setForeground(QColor(colList[i]));
		}
	}

	this->rehighlight();
}

int RsSyntaxHighlighter::quoteIndent(const QTextBlockFormat &blockFormat)
{
	// RsMarkdown sets a compact left indent and no right margin on quotes,
	// Qt's default for <blockquote> is 40px on both sides
	return qFuzzyIsNull(blockFormat.rightMargin()) ? RsMarkdown::QUOTE_INDENT : 40;
}

int RsSyntaxHighlighter::quoteLevel(const QTextBlockFormat &blockFormat)
{
	int level = 0;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	if (blockFormat.hasProperty(QTextFormat::BlockQuoteLevel)) {
		// cmark / HTML <blockquote>: the HTML importer always sets level 1
		// but adds one indent of left margin for each nested <blockquote>
		level = qMax(blockFormat.intProperty(QTextFormat::BlockQuoteLevel),
		             qRound(blockFormat.leftMargin() / quoteIndent(blockFormat)));
	}
#endif

	// Markdown rendered by Qt (GUI built without cmark): Qt's HTML export
	// drops the quote marker and only keeps the quote margins, 40px on the
	// right and 40px on the left for each nesting level.
	if (!level && qFuzzyCompare(blockFormat.rightMargin(), 40.0) && blockFormat.leftMargin() >= 40.0)
		level = qRound(blockFormat.leftMargin() / 40.0);

	return level;
}

QColor RsSyntaxHighlighter::quoteColor(int level) const
{
	level = qBound(1, level, quotationFormats.size());
	return quotationFormats.at(level - 1).foreground().color();
}

void RsSyntaxHighlighter::highlightBlock(const QString &text)
{
	if (text == "") return;

	// Rendered Markdown: the '>' characters are gone, the quote nesting
	// level comes from the block format instead (RSTextBrowser also draws
	// the quote bars from it).
	const QTextBlockFormat blockFormat = currentBlock().blockFormat();
	const int level = quoteLevel(blockFormat);
	if (level > 0) {
		setFormat(0, text.length(), quotationFormats.at(qBound(1, level, quotationFormats.size()) - 1));
		return;
	}

	// Code blocks (<pre>, Markdown ``` blocks) are shown verbatim: a line
	// starting with '>' there is not a quote.
	if (blockFormat.nonBreakableLines())
		return;

	QRegularExpression endl("[\\r\\n\\N{U+2028}]"); //Usually 0x2028 character is used for newline, no idea why
	int index = 0;
	QStringList lines = text.split(endl);
	foreach (const QString &cLine, lines) {
		QString line =cLine;
		line.replace(QChar::Nbsp,QChar::Space);
		int count = 0;
		for( int i=0; i<line.length(); i++ ){
			if( line[i] == '>' )
				count++;
			else if( line[i] != QChar::Space )
				break;
		}
		//Make it work with the compact chat style
		int start = line.indexOf(": >");
		if( start != -1 )
			start += 2; // Start at ">" not ":"
		else
			start = 0;

		if(count && start > count ) {
			// Found but already quotted
			start = 0;
		} else {
			if (start && !count) {
				// Start to count after name: >
				for(int i=start; i<line.length(); i++){
					if( line[i] == '>' )
						count++;
					else if( line[i] != QChar::Space )
						break;
				}
			}
		}

		if(count) {
			setFormat(index + start, line.length() - start, quotationFormats.at(qMin(count-1,quotationFormats.size()-1)));
		}
		index += line.length() + 1;
	}
}

//Dumping the raw unicode string into the console in Base64 encoding
/*
		QByteArray uniline;
		const QChar* qca = line.unicode();
		for(int i=0; qca[i]!='\0' ;++i)
		{
			uniline.append(qca[i].row());
			uniline.append(qca[i].cell());
		}
		std::cout << "Line: " << uniline.toBase64().toStdString() << std::endl;
 */
