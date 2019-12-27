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

	Q_PROPERTY(QColor textColorQuoteGreen READ textColorQuoteGreen WRITE setTextColorQuoteGreen)
	Q_PROPERTY(QColor textColorQuoteBlue READ textColorQuoteBlue WRITE setTextColorQuoteBlue)
	Q_PROPERTY(QColor textColorQuoteRed READ textColorQuoteRed WRITE setTextColorQuoteRed)
	Q_PROPERTY(QColor textColorQuoteMagenta READ textColorQuoteMagenta WRITE setTextColorQuoteMagenta)
	Q_PROPERTY(QColor textColorQuoteTurquoise READ textColorQuoteTurquoise WRITE setTextColorQuoteTurquoise)
	Q_PROPERTY(QColor textColorQuotePurple READ textColorQuotePurple WRITE setTextColorQuotePurple)
	Q_PROPERTY(QColor textColorQuoteMaroon READ textColorQuoteMaroon WRITE setTextColorQuoteMaroon)
	Q_PROPERTY(QColor textColorQuoteOlive READ textColorQuoteOlive WRITE setTextColorQuoteOlive)

	
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
	void setTextColorQuoteGreen(QColor textColorQuoteGreen);
	void setTextColorQuoteBlue(QColor textColorQuoteBlue);
	void setTextColorQuoteRed(QColor textColorQuoteRed);
	void setTextColorQuoteMagenta(QColor textColorQuoteMagenta);
	void setTextColorQuoteTurquoise(QColor textColorQuoteTurquoise);
	void setTextColorQuotePurple(QColor textColorQuotePurple);
	void setTextColorQuoteMaroon(QColor textColorQuoteMaroon);
	void setTextColorQuoteOlive(QColor textColorQuoteOlive);
};

#endif // RSSYNTAXHIGHLIGHTER_H
