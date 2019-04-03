/*******************************************************************************
 * util/RetroStyleLabel.cpp                                                    *
 *                                                                             *
 * Copyright (c) 2006-2007, crypton                                            *
 * Copyright (c) 2006, Matt Edman, Justin Hipple                               *
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

#include <util/RetroStyleLabel.h>

#include <QPainter>
#include <QMouseEvent>

RetroStyleLabel::RetroStyleLabel(QWidget * parent, Mode mode, Qt::AlignmentFlag hAlign)
	: QLabel(parent), _mode(mode) {

	_parent = parent;
	_pressed = false;
	_selected = false;
	_toggled = false;

	//Default background color
	_backgroundColor = _parent->palette().color(QPalette::Window);
	//Default text color
	_textColor = _parent->palette().color(QPalette::Text);

	_alignment = Qt::AlignVCenter | hAlign;
}

RetroStyleLabel::~RetroStyleLabel() {
}

void RetroStyleLabel::paintEvent(QPaintEvent * /*event*/) {
/*
	qDebug() << "Paint event";
	QLabel::paintEvent( event );
	return;
*/
	QRect rect = this->rect();

	QPainter painter(this);
	rect.adjust(-1, -1, 1, 1);

	//painter.fillRect(rect,QBrush(_backgroundColor));

	if (!_pressed && !_selected) {
		//Draw the left side if any
		if (!_normalLeftPixmap.isNull()) {
			painter.drawPixmap(0,0,_normalLeftPixmap);
		}
		//Fill the the label
		if (!_normalFillPixmap.isNull()) {
			QBrush brush(_normalFillPixmap);

			QRect fillRect = rect;
			if (!_normalLeftPixmap.isNull()) {
				fillRect.adjust(_normalLeftPixmap.rect().width()-1,0,0,0);
			}
			if (!_normalRightPixmap.isNull()) {
				fillRect.adjust(0,0,0-_normalRightPixmap.rect().width(),0);
			}
			painter.fillRect(fillRect,brush);
		}
		//Draw the right side
		if (!_normalRightPixmap.isNull()) {
			painter.drawPixmap( (rect.width()-1) - _normalRightPixmap.rect().width(),0,_normalRightPixmap);
		}

	} //if (! _pressed )

	else {
		//Draw the left side if any
		if (!_pressedLeftPixmap.isNull()) {
			painter.drawPixmap(0, 0, _pressedLeftPixmap);
		}
		//Fill the the label
		if (!_pressedFillPixmap.isNull()) {
			QBrush brush(_pressedFillPixmap);

			QRect fillRect = rect;
			if (!_pressedLeftPixmap.isNull()) {
				fillRect.adjust(_pressedLeftPixmap.rect().width() - 1, 0, 0, 0);
			}
			if (!_pressedRightPixmap.isNull()) {
				fillRect.adjust(0, 0, 0 - _pressedRightPixmap.rect().width(), 0);
			}
			painter.fillRect(fillRect,brush);
		}
		//Draw the right side
		if (!_pressedRightPixmap.isNull()) {
			painter.drawPixmap((rect.width() - 1) - _pressedRightPixmap.rect().width(), 0, _pressedRightPixmap);
		}
	}
	painter.end();
	QPainter p(this);
	drawText(&p);
	p.end();
}

void RetroStyleLabel::drawText(QPainter * painter) {
	QRect rect = this->rect();

	painter->save();
	painter->setPen(_textColor);
	painter->drawText(rect, _alignment, this->text(), &rect);
	painter->restore();
}

void RetroStyleLabel::resizeEvent ( QResizeEvent * event) {
	QLabel::resizeEvent(event);
}

void RetroStyleLabel::setPixmaps(const QPixmap & normalLeftPixmap,
				const QPixmap & normalRightPixmap,
				const QPixmap & normalFillPixmap,
				const QPixmap & pressedLeftPixmap,
				const QPixmap & pressedRightPixmap,
				const QPixmap & pressedFillPixmap) {

	_normalLeftPixmap = normalLeftPixmap;
	_normalRightPixmap = normalRightPixmap;
	_normalFillPixmap = normalFillPixmap;

	_pressedLeftPixmap = pressedLeftPixmap;
	_pressedRightPixmap = pressedRightPixmap;
	_pressedFillPixmap = pressedFillPixmap;
}

/*void RetroStyleLabel::setLeftPixmaps(const QPixmap & normalLeftPixmap, const QPixmap & pressedLeftPixmap) {
	_normalLeftPixmap = normalLeftPixmap;
	_pressedLeftPixmap = pressedLeftPixmap;
}

void RetroStyleLabel::setRightPixmaps(const QPixmap & normalRightPixmap, const QPixmap & pressedRightPixmap) {
	_normalRightPixmap = normalRightPixmap;
	_pressedRightPixmap = pressedRightPixmap;
}*/

void RetroStyleLabel::setTextColor(const QColor & color) {
	_textColor = color;
}

/*void RetroStyleLabel::setBackgroundColor(const QColor & color) {
	_backgroundColor = color;
}*/

void RetroStyleLabel::mouseMoveEvent(QMouseEvent * event) {
	_pressed = false;
	QPoint mousePosition = event->pos();
	const QRect rect = this->rect();
	if ((mousePosition.x() >= rect.x()) && ( mousePosition.x() <= rect.width())) {
		if ((mousePosition.y() >= rect.y()) && ( mousePosition.y() <= rect.height())) {
			_pressed = true;
		}
	}
	update();
}

void RetroStyleLabel::mousePressEvent(QMouseEvent * event) {
	if (event->button() == Qt::LeftButton) {
		_pressed = true;
		update();
	}
}

void RetroStyleLabel::mouseReleaseEvent(QMouseEvent * event) {
	if (!_pressed) {
		return;
	}
	//Gets the widget rect
	const QRect rect = this->rect();

	//Gets the mouse position relative to this widget
	QPoint mousePosition = event->pos();
	if ((mousePosition.x() >= rect.x()) && (mousePosition.x() <= rect.width())) {
		if ((mousePosition.y() >= rect.y()) && (mousePosition.y() <= rect.height())) {
			clicked();
			if (_mode == Toggled) {
				_toggled = !_toggled;
				if (_toggled) {
					_pressed = true;
				} else {
					_pressed = false;
				}
			} else {
				_pressed = false;
			}
			update();
		}
	}
}

void RetroStyleLabel::setText(const QString & text) {
	QLabel::setText(text);
	QFontMetrics fm(font());
	int textWidth = fm.width(text);
	textWidth += 40;
	QSize s = size();
	if (textWidth > s.width()) {
		//setMaximumSize(textWidth,s.height());
		setMinimumSize(textWidth,s.height());
	}
}
