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
#include "RSTreeWidget.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QWidgetAction>

#include "gui/settings/rsharesettings.h"

RSTreeWidget::RSTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
	mEnableColumnCustomize = false;
	mSettingsVersion = 0; // disabled
	mFilterReasonRole = -1; // disabled

	QHeaderView *h = header();
	h->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(h, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerContextMenuRequested(QPoint)));
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

void RSTreeWidget::setFilterReasonRole(int role /*=-1*/)
{
	if (role > Qt::UserRole)
		mFilterReasonRole = role;
}

void RSTreeWidget::filterItems(int filterColumn, const QString &text, int role)
{
	int count = topLevelItemCount();
	for (int index = 0; index < count; ++index) {
		filterItem(topLevelItem(index), filterColumn, text, role);
	}

	QTreeWidgetItem *item = currentItem();
	if (item && item->isHidden()) {
		// active item is hidden, deselect it
		setCurrentItem(NULL);
	}
}

bool RSTreeWidget::filterItem(QTreeWidgetItem *item, int filterColumn, const QString &text, int role)
{
	bool itemVisible = true;
	//Get who hide this item
	int filterReason = 0;
	if (mFilterReasonRole >= Qt::UserRole)
		filterReason = item->data(filterColumn, mFilterReasonRole).toInt();
	//Remove this filter for last test
	if (filterReason & FILTER_REASON_TEXT)
		filterReason -= FILTER_REASON_TEXT;

	if (!text.isEmpty()) {
		if (!item->data(filterColumn, role).toString().contains(text, Qt::CaseInsensitive)) {
			itemVisible = false;
		}
	}

	int visibleChildCount = 0;
	int count = item->childCount();
	for (int index = 0; index < count; ++index) {
		if (filterItem(item->child(index), filterColumn, text, role)) {
			++visibleChildCount;
		}
	}

	if (!itemVisible && !visibleChildCount) {
		filterReason |= FILTER_REASON_TEXT;
	}
	item->setHidden(filterReason != 0);
	//Update hiding reason
	if (mFilterReasonRole >= Qt::UserRole)
		item->setData(filterColumn, mFilterReasonRole, filterReason);

	return (itemVisible || visibleChildCount);
}

void RSTreeWidget::filterMinValItems(int filterColumn, const double &value, int role)
{
	int count = topLevelItemCount();
	for (int index = 0; index < count; ++index) {
		filterMinValItem(topLevelItem(index), filterColumn, value, role);
	}

	QTreeWidgetItem *item = currentItem();
	if (item && item->isHidden()) {
		// active item is hidden, deselect it
		setCurrentItem(NULL);
	}
}

bool RSTreeWidget::filterMinValItem(QTreeWidgetItem *item, int filterColumn, const double &value, int role)
{
	bool itemVisible = true;
	//Get who hide this item
	int filterReason = 0;
	if (mFilterReasonRole >= Qt::UserRole)
		filterReason = item->data(filterColumn, mFilterReasonRole).toInt();
	//Remove this filter for last test
	if (filterReason & FILTER_REASON_MINVAL)
		filterReason -= FILTER_REASON_MINVAL;

	bool ok = false;
	if ((item->data(filterColumn, role).toDouble(&ok) < value) && ok ) {
		itemVisible = false;
	}

	int visibleChildCount = 0;
	int count = item->childCount();
	for (int index = 0; index < count; ++index) {
		if (filterMinValItem(item->child(index), filterColumn, value, role)) {
			++visibleChildCount;
		}
	}

	if (!itemVisible && !visibleChildCount) {
		filterReason |= FILTER_REASON_MINVAL;
	}
	item->setHidden(filterReason != 0);
	//Update hiding reason
	if (mFilterReasonRole >= Qt::UserRole)
		item->setData(filterColumn, mFilterReasonRole, filterReason);

	return (itemVisible || visibleChildCount);
}

void RSTreeWidget::setSettingsVersion(qint32 version)
{
	mSettingsVersion = version;
}

void RSTreeWidget::processSettings(bool load)
{
	if (load) {
		// Load settings

		// State of tree widget
		if (mSettingsVersion == 0 || Settings->value(QString("%1Version").arg(objectName())) == mSettingsVersion) {
			// Compare version, because Qt can crash in restoreState after column changes
			header()->restoreState(Settings->value(objectName()).toByteArray());
		}
	} else {
		// Save settings

		// state of tree widget
		Settings->setValue(objectName(), header()->saveState());

		// Save version
		if (mSettingsVersion) {
			Settings->setValue(QString("%1Version").arg(objectName()), mSettingsVersion);
		}
	}
}

void RSTreeWidget::enableColumnCustomize(bool customizable)
{
	if (customizable == mEnableColumnCustomize) {
		return;
	}

	mEnableColumnCustomize = customizable;
}

void RSTreeWidget::setColumnCustomizable(int column, bool customizable)
{
	mColumnCustomizable[column] = customizable;
}

void RSTreeWidget::addContextMenuAction(QAction *action)
{
	mContextMenuActions.push_back(action);
}

void RSTreeWidget::addContextMenuMenu(QMenu *menu)
{
	mContextMenuMenus.push_back(menu);
}

QMenu *RSTreeWidget::createStandardContextMenu(QMenu *contextMenu)
{
    if (!contextMenu){
        contextMenu = new QMenu(this);
        contextMenu->addSeparator();
    }

    if(!mContextMenuActions.isEmpty() || mEnableColumnCustomize ||!mContextMenuActions.isEmpty() || !mContextMenuMenus.isEmpty()) {
        QWidget *widget = new QWidget(contextMenu);
        widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

        // create menu header
        QHBoxLayout *hbox = new QHBoxLayout(widget);
        hbox->setMargin(0);
        hbox->setSpacing(6);

        QLabel *iconLabel = new QLabel(widget);
        QPixmap pix = QPixmap("/:/images/settings.png").scaledToHeight(QFontMetricsF(iconLabel->font()).height()*1.5);          //hide icon
        iconLabel->setPixmap(pix);
        iconLabel->setMaximumSize(iconLabel->frameSize().height() + pix.height(), pix.width());
        hbox->addWidget(iconLabel);

        QLabel *textLabel = new QLabel("<strong>" + tr("Tree View Options") + "</strong>", widget);
        hbox->addWidget(textLabel);

        QSpacerItem *spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        hbox->addItem(spacerItem);

        widget->setLayout(hbox);

        QWidgetAction *widgetAction = new QWidgetAction(this);
        widgetAction->setDefaultWidget(widget);
        contextMenu->addAction(widgetAction);
    }

    if (mEnableColumnCustomize) {
        QMenu *headerMenu = contextMenu->addMenu(QIcon(),tr("Show column..."));

        QTreeWidgetItem *item = headerItem();
        int columnCount = item->columnCount();
        for (int column = 0; column < columnCount; ++column)
        {
            QMap<int, bool>::const_iterator it = mColumnCustomizable.find(column);
            if (it != mColumnCustomizable.end() && *it == false) {
                continue;
            }
            QString txt = item->text(column) ;
            if(txt == "")
                txt = item->data(column,Qt::UserRole).toString() ;

            if(txt=="")
                txt = tr("[no title]") ;

            QAction *action = headerMenu->addAction(QIcon(), txt, this, SLOT(columnVisible()));
            action->setCheckable(true);
            action->setData(column);
            action->setChecked(!isColumnHidden(column));
        }
    }

    if (!mContextMenuActions.isEmpty()) {
        bool addSeparator = false;
        if (!contextMenu->isEmpty()) {
            // Check for visible action
            foreach (QAction *action, mContextMenuActions) {
                if (action->isVisible()) {
                    addSeparator = true;
                    break;
                }
            }
        }

        if (addSeparator) {
            contextMenu->addSeparator();
        }

        contextMenu->addActions(mContextMenuActions);
    }

    if (!mContextMenuMenus.isEmpty()) {
        foreach(QMenu *menu, mContextMenuMenus) {
            contextMenu->addSeparator();
            contextMenu->addMenu(menu);
        }
    }

    return contextMenu;
}

void RSTreeWidget::headerContextMenuRequested(const QPoint &pos)
{
    QMenu *contextMenu = createStandardContextMenu(NULL);
    if (contextMenu->isEmpty()) {
        return;
    }

    contextMenu->exec(mapToGlobal(pos));
    delete contextMenu;
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

void RSTreeWidget::resort()
{
	if (isSortingEnabled()) {
		sortItems(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
	}
}
