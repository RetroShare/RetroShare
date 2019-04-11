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

	Q_PROPERTY(QColor textColorQuote READ textColorQuote WRITE setTextColorQuote)

public:
	RsSyntaxHighlighter(QTextEdit *parent = 0);
	QColor textColorQuote() const { return quotationFormat.foreground().color(); };

protected:
	void highlightBlock(const QString &text);

private:
	QTextCharFormat quotationFormat;

signals:

public slots:
	void setTextColorQuote(QColor textColorQuote);

};

#endif // RSSYNTAXHIGHLIGHTER_H
