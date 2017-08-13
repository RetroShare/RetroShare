/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2012, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef MIMETEXTEDIT_H
#define MIMETEXTEDIT_H

#include <QCompleter>
#include "RSTextEdit.h"
#include "util/RsSyntaxHighlighter.h"

//cppcheck-suppress noConstructor
class MimeTextEdit : public RSTextEdit
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorQuote READ textColorQuote WRITE setTextColorQuote)

public:
	MimeTextEdit(QWidget *parent = 0);

	//Form here: http://qt-project.org/doc/qt-4.8/tools-customcompleter.html
	void setCompleter(QCompleter *completer);
	QCompleter *completer() const;
	void setCompleterKeyModifiers(Qt::KeyboardModifier modifiers);
	Qt::KeyboardModifier getCompleterKeyModifiers() const;
	void setCompleterKey(Qt::Key key);
	Qt::Key getCompleterKey() const;
	void forceCompleterShowNextKeyEvent(QString startString = "");

	// Add QAction to context menu (action won't be deleted)
	void addContextMenuAction(QAction *action);

	QColor textColorQuote() const { return highliter->textColorQuote();}
	bool onlyPlainText() const {return mOnlyPlainText;}

	void setMaxBytes(int limit) {mMaxBytes = limit;}

public slots:
	void setTextColorQuote(QColor textColorQuote) { highliter->setTextColorQuote(textColorQuote);}
	void setOnlyPlainText(bool bOnlyPlainText) {mOnlyPlainText = bOnlyPlainText;}

signals:
	void calculateContextMenuActions();

protected:
	virtual bool canInsertFromMimeData(const QMimeData* source) const;
	virtual void insertFromMimeData(const QMimeData* source);
	virtual void contextMenuEvent(QContextMenuEvent *e);
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void focusInEvent(QFocusEvent *e);

private slots:
	void insertCompletion(const QString &completion);
	void pasteLink();
	void pasteOwnCertificateLink();
	void pastePlainText();
	void spoiler();

private:
	QString textUnderCursor() const;

private:
	QCompleter *mCompleter;
	Qt::KeyboardModifier mCompleterKeyModifiers;
	Qt::Key mCompleterKey;
	bool mForceCompleterShowNextKeyEvent;
	QString mCompleterStartString;
	QList<QAction*> mContextMenuActions;
	RsSyntaxHighlighter *highliter;
	bool mOnlyPlainText;
	int mMaxBytes = -1;	//limit content size, for pasting images
};

#endif // MIMETEXTEDIT_H
