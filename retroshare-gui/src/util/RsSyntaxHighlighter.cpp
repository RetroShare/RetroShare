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

#include "RsSyntaxHighlighter.h"

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

void RsSyntaxHighlighter::highlightBlock(const QString &text)
{
	if (text == "") return;

	QRegExp endl("[\\r\\n\\x2028]"); //Usually 0x2028 character is used for newline, no idea why
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
