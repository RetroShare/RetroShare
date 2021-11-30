/*******************************************************************************
 * gui/common/RSTreeView.cpp                                                   *
 *                                                                             *
 * Copyright (C) 2010 RetroShare Team <retroshare.project@gmail.com>           *
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

#include "RSTreeView.h"

#include "util/rsdebug.h"

#include <QPainter>
#include <QResizeEvent>

//#define DEBUG_RSTREEVIEW

RSTreeView::RSTreeView(QWidget *parent)
    : QTreeView(parent), autoSelect(false)
{
	setMouseTracking(false); // normally the default, but who knows if it's not going to change in the future.
}

void RSTreeView::wheelEvent(QWheelEvent *e)
{
	if(e->modifiers() == Qt::ControlModifier)
	{
		emit zoomRequested(e->angleDelta().y() > 0);
		return;
	}
	else
		QTreeView::wheelEvent(e);
}

void RSTreeView::mouseMoveEvent(QMouseEvent *e)
{
#ifdef DEBUG_RSTREEVIEW
	RS_DBG(e->localPos().x(), ":", e->localPos().y());
#endif
	if (autoSelect)
	{
		QModelIndex idx = indexAt(e->pos());

		if(idx.isValid() && idx != selectionModel()->currentIndex())
		{
#ifdef DEBUG_RSTREEVIEW
	RS_DBG("Selection changed");
#endif
			selectionModel()->setCurrentIndex(idx,QItemSelectionModel::ClearAndSelect);
		}
	}

	QTreeView::mouseMoveEvent(e);
}

void RSTreeView::leaveEvent(QEvent *e)
{
#ifdef DEBUG_RSTREEVIEW
	RS_DBG("");
#endif
	if (autoSelect)
	{
		auto fp = focusPolicy();
		setFocusPolicy(Qt::NoFocus); // To not select first index when resetting current index.
		selectionModel()->setCurrentIndex(QModelIndex(),QItemSelectionModel::Clear); // Close editor
		setFocusPolicy(fp);
	}

	QTreeView::leaveEvent(e);
}

void RSTreeView::setAutoSelect(bool b)
{
	autoSelect = b; // Keep this because setMouseTracking can be called outside.
	setMouseTracking(b);
}

void RSTreeView::resizeEvent(QResizeEvent *e)
{
	QTreeView::resizeEvent(e);
	emit sizeChanged(e->size());
}

void RSTreeView::setPlaceholderText(const QString &text)
{
	placeholderText = text;
	viewport()->repaint();
}

void RSTreeView::paintEvent(QPaintEvent *event)
{
	QTreeView::paintEvent(event);

	if (placeholderText.isEmpty() == false && model() && model()->rowCount() == 0) {
		QWidget *vieportWidget = viewport();
		QPainter painter(vieportWidget);

		QPen pen = painter.pen();
		QColor color = pen.color();
		color.setAlpha(128);
		pen.setColor(color);
		painter.setPen(pen);

		painter.drawText(QRect(QPoint(), vieportWidget->size()), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, placeholderText);
	}
};
