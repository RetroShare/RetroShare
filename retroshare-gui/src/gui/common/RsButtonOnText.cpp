/*******************************************************************************
 * gui/common/RsButtonOnText.cpp                                               *
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
#include "RsButtonOnText.h"

#include <QHelpEvent>
#include <QPointer>
#include <QTextDocumentFragment>
#include <QToolTip>
#include <QUrl>
#include <QUuid>
#include <QStylePainter>
#include <QStyleOption>
#include <iostream>

RSButtonOnText::RSButtonOnText(QWidget *parent)
  : QPushButton(parent), _textEdit(NULL), _textEditViewPort(NULL), _textCursor(NULL)
  , _lenght(-1), _mouseOver(false), _pressed(false)
{
	_uuid = QUuid::createUuid().toString();
}
RSButtonOnText::RSButtonOnText(const QString &text, QWidget *parent)
  : QPushButton(parent), _textEdit(NULL), _textEditViewPort(NULL), _textCursor(NULL)
  , _lenght(-1), _mouseOver(false), _pressed(false)
  //: RSButtonOnText(parent)//delegating constructors only available with -std=c++11 or -std=gnu++11
{
	_uuid = QUuid::createUuid().toString();
	setText(text);
}

RSButtonOnText::RSButtonOnText(const QIcon& icon, const QString &text, QWidget *parent)
  : QPushButton(parent), _textEdit(NULL), _textEditViewPort(NULL), _textCursor(NULL)
  , _lenght(-1), _mouseOver(false), _pressed(false)
  //: RSButtonOnText(text, parent)//delegating constructors only available with -std=c++11 or -std=gnu++11
{
	_uuid = QUuid::createUuid().toString();
	setIcon(icon);
	setText(text);
}

RSButtonOnText::RSButtonOnText(QTextEdit *textEdit, QWidget *parent)
  : QPushButton(parent)
  , _lenght(-1), _mouseOver(false), _pressed(false)
  //: RSButtonOnText(parent)//delegating constructors only available with -std=c++11 or -std=gnu++11
{
	_uuid = QUuid::createUuid().toString();
	appendToText(textEdit);
}

RSButtonOnText::RSButtonOnText(const QString &text, QTextEdit *textEdit, QWidget *parent)
  : QPushButton(parent)
  , _lenght(-1), _mouseOver(false), _pressed(false)
  //: RSButtonOnText(parent)//delegating constructors only available with -std=c++11 or -std=gnu++11
{
	_uuid = QUuid::createUuid().toString();
	setText(text);
	appendToText(textEdit);
}

RSButtonOnText::RSButtonOnText(const QIcon& icon, const QString &text, QTextEdit *textEdit, QWidget *parent)
  : QPushButton(parent)
  , _lenght(-1), _mouseOver(false), _pressed(false)
  //: RSButtonOnText(parent)//delegating constructors only available with -std=c++11 or -std=gnu++11
{
	_uuid = QUuid::createUuid().toString();
	setIcon(icon);
	setText(text);
	appendToText(textEdit);
}

RSButtonOnText::~RSButtonOnText()
{
	clear();
}

bool RSButtonOnText::eventFilter(QObject *obj, QEvent *event)
{
    // comment electron:
    // the guard is here, because someone deletes this object in callbacks(slots) called from here
    // the QPointer can detect this
    // but this is bad practice, because it hides the root cause for the problem,
    // which is the deletion of objects in their own signals
    // TODO: better use the Qt function deleteLater
	QPointer<QAbstractButton> guard(this);
	QPoint point;
	if (isEventForThis(obj, event, point)) {
		if (event->type() == QEvent::ToolTip)	{
			QToolTip::showText(point, this->toolTip());
			event->ignore();//For other don't clear this one
			return true;
		}
		if (event->type() == QEvent::MouseButtonPress)	{
			QMouseEvent* mouseEvent = new QMouseEvent(QEvent::MouseButtonPress
			                                          ,QPoint(1,1),Qt::LeftButton,Qt::LeftButton,Qt::KeyboardModifier::NoModifier);
			QPushButton::mousePressEvent(mouseEvent);
			delete mouseEvent;
            if (guard) _pressed = true;
			//if (guard) emit pressed();
			if (guard) updateImage();
		}
		if (event->type() == QEvent::MouseButtonRelease)	{
			QMouseEvent* mouseEvent = new QMouseEvent(QEvent::MouseButtonPress
			                                          ,QPoint(1,1),Qt::LeftButton,Qt::LeftButton,Qt::KeyboardModifier::NoModifier);
			QPushButton::mouseReleaseEvent(mouseEvent);
			delete mouseEvent;
            if (guard) _pressed = false;
			//if (guard) emit released();
			//if (guard) emit clicked();
			//if (guard) if (isCheckable()) emit clicked(QPushButton::isChecked());
			//if (guard) if (isCheckable()) emit toggled(QPushButton::isChecked());
			if (guard) updateImage();
		}
		if (event->type() == QEvent::MouseMove)	{
			if (!_mouseOver){
				//QMouseEvent* mouseEvent = new QMouseEvent(QEvent::MouseButtonPress
				//                                          ,QPoint(1,1),Qt::LeftButton,Qt::LeftButton,0);
				//QPushButton::enterEvent(mouseEvent);//Do nothing
				//delete mouseEvent;
				//QPushButton::setDown(true);
				if (guard) emit mouseEnter();
			}
            if (guard) _mouseOver = true;
			if (guard) updateImage();
		}
	} else {
		if (event->type() == QEvent::MouseMove)	{
			if (_mouseOver) {
				//QMouseEvent* mouseEvent = new QMouseEvent(QEvent::MouseButtonPress
				//                                          ,QPoint(1,1),Qt::LeftButton,Qt::LeftButton,0);
				//QPushButton::leaveEvent(mouseEvent);//Do nothing
				//delete mouseEvent;
				//QPushButton::setDown(false);
				_mouseOver = false;
				if (guard) emit mouseLeave();
				if (guard) updateImage();
			}
			if (_pressed){
				QMouseEvent* mouseEvent = new QMouseEvent(QEvent::MouseButtonPress
				                                          ,QPoint(1,1),Qt::LeftButton,Qt::LeftButton,Qt::KeyboardModifier::NoModifier);
				QPushButton::mouseReleaseEvent(mouseEvent);
				delete mouseEvent;
				//if (guard) emit released();
				if (guard) updateImage();
			}
		}
	}

	if (!guard)
		std::cerr << "BIG FAT WARNING from RSButtonOnText::eventFilter(): i was deleted in my own event handler. This is bad practice. Please make a patch and use deleteLater to delay deletion." << std::endl ;

	// pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}

bool RSButtonOnText::isEventForThis(QObject *obj, QEvent *event, QPoint &point)
{
	switch (event->type()) {
		case QEvent::MouseButtonPress://2
		case QEvent::MouseButtonRelease://3
		case QEvent::MouseButtonDblClick://4
		case QEvent::MouseMove://5
		{
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			point = mouseEvent->pos();
		}
		break;
		case QEvent::ToolTip://110
		{
			QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
			point = helpEvent->globalPos();
		}
		break;
		default:
		return false;
	}

	if (!event->isAccepted()) return false;//Already other take this event (true by default)

	if (obj ==_textEditViewPort) {
		if (_textEdit){

			QTextCursor cursor = _textEdit->cursorForPosition(point);
			if ( (_textCursor->anchor() <= cursor.anchor())
			     && (cursor.position() <= _textCursor->anchor()+_lenght)){
				return true;
			}
		}
	}
	return false;
}

QString RSButtonOnText::uuid()
{
	return _uuid;
}

QString RSButtonOnText::htmlText()
{
	return "<img src=\"" + _uuid + "\" /></a>";
}

void RSButtonOnText::appendToText(QTextEdit *textEdit)
{
	clear();
	_textEdit = textEdit;
	_textEditViewPort = textEdit->viewport();

	updateImage();

	_textCursor = new QTextCursor(textEdit->textCursor());
	_textCursor->movePosition(QTextCursor::End);
	_textEdit->insertPlainText(QString(QChar::Nbsp));//To get cursorForPosition, else it returns next char after middle
	int textCursorSavePos = _textCursor->position();
	_textCursor->insertHtml(htmlText());
	_textCursor->setPosition(textCursorSavePos);
	_textCursor->movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
	_lenght = _textCursor->position() - _textCursor->anchor();
	_textEdit->insertPlainText(QString(QChar::Nbsp));//To get cursorForPosition, else it returns next char after middle
	_textCursor->setPosition(textCursorSavePos);

	_textEditViewPort->installEventFilter(this);
}

void RSButtonOnText::clear()
{
	if(_lenght > 0){
		_textCursor->setPosition(_textCursor->anchor()-1);//Remove Space too
		_textCursor->setPosition(_textCursor->anchor() + _lenght + 2, QTextCursor::KeepAnchor);
		_textCursor->deleteChar();
		_lenght = -1;
	}
}

void RSButtonOnText::updateImage()
{
	if (_textEdit){
		adjustSize();
		QPixmap pixmap;

// Can't get Qt's grab functionality to draw a transparent background so we use our own drawing on a pixmap
//#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
//		pixmap = this->grab();//QT5
//#else
//		pixmap = QPixmap::grabWidget(this);
//#endif

		// get a transparent pixmap
		pixmap = QPixmap(size());
		pixmap.fill(Qt::transparent);

		// init options
		QStyleOptionButton option;
		initStyleOption(&option);

		// draw the button onto the pixmap
		QStylePainter painter(&pixmap, this);
		painter.drawControl(QStyle::CE_PushButton, option);
		painter.end();

		_textEdit->setUpdatesEnabled(false);
		_textEdit->document()->addResource(QTextDocument::ImageResource,QUrl(_uuid),QVariant(pixmap));
		_textEdit->setUpdatesEnabled(true);
	}
}
