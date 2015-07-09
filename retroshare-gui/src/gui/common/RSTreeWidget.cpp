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

#include <QPainter>
#include <QMouseEvent>
#include <QHeaderView>
#include <QMenu>

#include "RSTreeWidget.h"
#include "gui/settings/rsharesettings.h"

RSTreeWidget::RSTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
	mColumnCustomizable = false;
}

void RSTreeWidget::setPlaceholderText(const QString &text)
{
	mPlaceholderText = text;
	viewport()->repaint();
}

void RSTreeWidget::paintEvent(QPaintEvent *event)
{
	QTreeWidget::paintEvent(event);

	if (mPlaceholderText.isEmpty() == false && model() && model()->rowCount() == 0) {
		QWidget *vieportWidget = viewport();
		QPainter painter(vieportWidget);

		QPen pen = painter.pen();
		QColor color = pen.color();
		color.setAlpha(128);
		pen.setColor(color);
		painter.setPen(pen);

		painter.drawText(QRect(QPoint(), vieportWidget->size()), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, mPlaceholderText);
	}
}

void RSTreeWidget::mousePressEvent(QMouseEvent *event)
{
#if QT_VERSION < 0x040700
	if (event->buttons() & Qt::MidButton) {
#else
	if (event->buttons() & Qt::MiddleButton) {
#endif
		if (receivers(SIGNAL(signalMouseMiddleButtonClicked(QTreeWidgetItem*))) > 0) {
			QTreeWidgetItem *item = itemAt(event->pos());
			if (item) {
				setCurrentItem(item);
				emit signalMouseMiddleButtonClicked(item);
			}
			return; // eat event
		}
	}

	QTreeWidget::mousePressEvent(event);
}

void RSTreeWidget::filterItems(int filterColumn, const QString &text)
{
	int count = topLevelItemCount();
	for (int index = 0; index < count; ++index) {
		filterItem(topLevelItem(index), filterColumn, text);
	}

	QTreeWidgetItem *item = currentItem();
	if (item && item->isHidden()) {
		// active item is hidden, deselect it
		setCurrentItem(NULL);
	}
}

bool RSTreeWidget::filterItem(QTreeWidgetItem *item, int filterColumn, const QString &text)
{
	bool itemVisible = true;

	if (!text.isEmpty()) {
		if (!item->text(filterColumn).contains(text, Qt::CaseInsensitive)) {
			itemVisible = false;
		}
	}

	int visibleChildCount = 0;
	int count = item->childCount();
	for (int index = 0; index < count; ++index) {
		if (filterItem(item->child(index), filterColumn, text)) {
			++visibleChildCount;
		}
	}

	if (itemVisible || visibleChildCount) {
		item->setHidden(false);
	} else {
		item->setHidden(true);
	}

	return (itemVisible || visibleChildCount);
}

void RSTreeWidget::processSettings(bool load)
{
	if (load) {
		// load settings

		// state of tree widget
		header()->restoreState(Settings->value(objectName()).toByteArray());
	} else {
		// save settings

		// state of tree widget
		Settings->setValue(objectName(), header()->saveState());
	}
}

void RSTreeWidget::setColumnCustomizable(bool customizable)
{
	if (customizable == mColumnCustomizable) {
		return;
	}

	mColumnCustomizable = customizable;

	QHeaderView *h = header();

	if (mColumnCustomizable) {
		h->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(h, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerContextMenuRequested(QPoint)));
	} else {
		h->setContextMenuPolicy(Qt::DefaultContextMenu);
		disconnect(h, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerContextMenuRequested(QPoint)));
	}
}

void RSTreeWidget::headerContextMenuRequested(const QPoint &pos)
{
	if (!mColumnCustomizable) {
		return;
	}

	QMenu contextMenu(this);

	QTreeWidgetItem *item = headerItem();
	int columnCount = item->columnCount();
	for (int column = 0; column < columnCount; ++column) {
		QAction *action = contextMenu.addAction(QIcon(), item->text(column), this, SLOT(columnVisible()));
		action->setCheckable(true);
		action->setData(column);
		action->setChecked(!isColumnHidden(column));
	}

	contextMenu.exec(mapToGlobal(pos));
}

void RSTreeWidget::columnVisible()
{
	QAction *action = dynamic_cast<QAction*>(sender());
	if (!action) {
		return;
	}

	int column = action->data().toInt();
	bool visible = action->isChecked();
	setColumnHidden(column, !visible);

	emit columnVisibleChanged(column, visible);
}
