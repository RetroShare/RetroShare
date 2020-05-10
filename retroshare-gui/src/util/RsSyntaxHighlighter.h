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
#include <QTextEdit>

class RsSyntaxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorQuoteGreen READ textColorQuoteGreen WRITE setTextColorQuoteLevel1)
	Q_PROPERTY(QColor textColorQuoteBlue READ textColorQuoteBlue WRITE setTextColorQuoteLevel2)
	Q_PROPERTY(QColor textColorQuoteRed READ textColorQuoteRed WRITE setTextColorQuoteLevel3)
	Q_PROPERTY(QColor textColorQuoteMagenta READ textColorQuoteMagenta WRITE setTextColorQuoteLevel4)
	Q_PROPERTY(QColor textColorQuoteTurquoise READ textColorQuoteTurquoise WRITE setTextColorQuoteLevel5)
	Q_PROPERTY(QColor textColorQuotePurple READ textColorQuotePurple WRITE setTextColorQuoteLevel6)
	Q_PROPERTY(QColor textColorQuoteMagenta READ textColorQuoteMaroon WRITE setTextColorQuoteLevel7)
	Q_PROPERTY(QColor textColorQuoteOlive READ textColorQuoteOlive WRITE setTextColorQuoteLevel8)

	
public:
	RsSyntaxHighlighter(QTextEdit *parent = 0);
	QColor textColorQuoteGreen() const { return quotationFormatGreen.foreground().color(); };
	QColor textColorQuoteBlue() const { return quotationFormatBlue.foreground().color(); };
	QColor textColorQuoteRed() const { return quotationFormatRed.foreground().color(); };
	QColor textColorQuoteMagenta() const { return quotationFormatMagenta.foreground().color(); };
	QColor textColorQuoteTurquoise() const { return quotationFormatTurquoise.foreground().color(); };
	QColor textColorQuotePurple() const { return quotationFormatPurple.foreground().color(); };
	QColor textColorQuoteMaroon() const { return quotationFormatMaroon.foreground().color(); };
	QColor textColorQuoteOlive() const { return quotationFormatOlive.foreground().color(); };



protected:
	void highlightBlock(const QString &text);

private:
	QTextCharFormat quotationFormatGreen;
	QTextCharFormat quotationFormatBlue;
	QTextCharFormat quotationFormatRed;
	QTextCharFormat quotationFormatMagenta;
	QTextCharFormat quotationFormatTurquoise;
	QTextCharFormat quotationFormatPurple;
	QTextCharFormat quotationFormatMaroon;
	QTextCharFormat quotationFormatOlive;

signals:

public slots:
	void setTextColorQuoteLevel1(QColor textColorQuoteGreen);
	void setTextColorQuoteLevel2(QColor textColorQuoteBlue);
	void setTextColorQuoteLevel3(QColor textColorQuoteRed);
	void setTextColorQuoteLevel4(QColor textColorQuoteMagenta);
	void setTextColorQuoteLevel5(QColor textColorQuoteTurquoise);
	void setTextColorQuoteLevel6(QColor textColorQuotePurple);
	void setTextColorQuoteLevel7(QColor textColorQuoteMaroon);
	void setTextColorQuoteLevel8(QColor textColorQuoteOlive);
};

#endif // RSSYNTAXHIGHLIGHTER_H
