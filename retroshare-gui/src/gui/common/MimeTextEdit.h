/*******************************************************************************
 * gui/common/MimeTextEdit.h                                                   *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef MIMETEXTEDIT_H
#define MIMETEXTEDIT_H

#include <QCompleter>
#include "RSTextEdit.h"
#include "util/RsSyntaxHighlighter.h"

//cppcheck-suppress noConstructor
class MimeTextEdit : public RSTextEdit
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

	QColor textColorQuoteGreen() const { return highliter->textColorQuoteGreen();}
	QColor textColorQuoteBlue() const { return highliter->textColorQuoteBlue();}
	QColor textColorQuoteRed() const { return highliter->textColorQuoteRed();}
	QColor textColorQuoteMagenta() const { return highliter->textColorQuoteMagenta();}
	QColor textColorQuoteTurquoise() const { return highliter->textColorQuoteTurquoise();}
	QColor textColorQuotePurple() const { return highliter->textColorQuotePurple(); }
	QColor textColorQuoteMaroon() const { return highliter->textColorQuoteMaroon(); }
	QColor textColorQuoteOlive() const { return highliter->textColorQuoteOlive(); }

	bool onlyPlainText() const {return mOnlyPlainText;}

	void setMaxBytes(int limit) {mMaxBytes = limit;}

public slots:
	void setTextColorQuoteGreen(QColor textColorQuoteGreen);
	void setTextColorQuoteBlue(QColor textColorQuoteBlue);
	void setTextColorQuoteRed(QColor textColorQuoteRed);
	void setTextColorQuoteMagenta(QColor textColorQuoteMagenta);
	void setTextColorQuoteTurquoise(QColor textColorQuoteTurquoise);
	void setTextColorQuotePurple(QColor textColorQuotePurple);
	void setTextColorQuoteMaroon(QColor textColorQuoteMaroon);
	void setTextColorQuoteOlive(QColor textColorQuoteOlive);

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
