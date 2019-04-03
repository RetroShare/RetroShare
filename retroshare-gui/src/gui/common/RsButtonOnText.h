/*******************************************************************************
 * gui/common/RsButtonOnText.h                                                 *
 *                                                                             *
 * Copyright (C) 2015, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef RSBUTTONONTEXT_H
#define RSBUTTONONTEXT_H

#include <QHelpEvent>
#include <QPushButton>
#include <QTextEdit>

class RSButtonOnText : public QPushButton
{
	Q_OBJECT

public:
	explicit RSButtonOnText(QWidget *parent = 0);
	explicit RSButtonOnText(const QString &text, QWidget *parent=0);
	RSButtonOnText(const QIcon& icon, const QString &text, QWidget *parent=0);
	RSButtonOnText(QTextEdit *textEdit, QWidget *parent = 0);
	RSButtonOnText(const QString &text, QTextEdit *textEdit, QWidget *parent = 0);
	RSButtonOnText(const QIcon& icon, const QString &text, QTextEdit *textEdit, QWidget *parent = 0);
	~RSButtonOnText();

	QString uuid();
	QString htmlText();
	void appendToText(QTextEdit *textEdit);
	void clear();
	void updateImage();

signals:
	void mouseEnter();
	void mouseLeave();

protected:
	bool eventFilter(QObject *obj, QEvent *event);

private:
	bool isEventForThis(QObject *obj, QEvent *event, QPoint &point);

	QString _uuid;
	QTextEdit* _textEdit;
	QWidget* _textEditViewPort;
	QTextCursor* _textCursor;
	int _lenght;//Because cursor end position move durring editing
	bool _mouseOver;
	bool _pressed;

};

#endif // RSBUTTONONTEXT_H
