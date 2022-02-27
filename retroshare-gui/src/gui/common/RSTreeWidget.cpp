/*******************************************************************************
 * gui/common/RSTreeWidget.cpp                                                 *
 *                                                                             *
 * Copyright (C) 2012 RetroShare Team <retroshare.project@gmail.com>           *
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

#include "RSTreeWidget.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QWidgetAction>

#include "gui/settings/rsharesettings.h"
#include "gui/common/FilesDefs.h"

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
		header()->setHidden(Settings->value(objectName()+"HiddenHeader", false).toBool());
	} else {
		// Save settings

		// state of tree widget
		Settings->setValue(objectName(), header()->saveState());
		Settings->setValue(objectName()+"HiddenHeader", header()->isHidden());

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

	if(!mContextMenuActions.isEmpty() || !mContextMenuMenus.isEmpty() || mEnableColumnCustomize) {
		QFrame *widget = new QFrame(contextMenu);
		widget->setObjectName("gradFrame"); //Use qss
		//widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

		// create menu header
		QHBoxLayout *hbox = new QHBoxLayout(widget);
		hbox->setMargin(0);
		hbox->setSpacing(6);

		QLabel *iconLabel = new QLabel(widget);
		iconLabel->setObjectName("trans_Icon");
		QPixmap pix = FilesDefs::getPixmapFromQtResourcePath(":/icons/png/options2.png").scaledToHeight(QFontMetricsF(iconLabel->font()).height()*1.5);
		iconLabel->setPixmap(pix);
		iconLabel->setMaximumSize(iconLabel->frameSize().height() + pix.height(), pix.width());
		hbox->addWidget(iconLabel);

		QLabel *textLabel = new QLabel("<strong>" + tr("Tree View Options") + "</strong>", widget);
		textLabel->setObjectName("trans_Text");
		hbox->addWidget(textLabel);

		QSpacerItem *spacerItem = new QSpacerItem(40, 24, QSizePolicy::Expanding, QSizePolicy::Minimum);
		hbox->addItem(spacerItem);

		widget->setLayout(hbox);

		QWidgetAction *widgetAction = new QWidgetAction(this);
		widgetAction->setDefaultWidget(widget);
		contextMenu->addAction(widgetAction);
	}

	if (mEnableColumnCustomize) {
		QAction *actShowHeader = contextMenu->addAction(QIcon(), tr("Show Header"), this, SLOT(headerVisible()));
		actShowHeader->setCheckable(true);
		actShowHeader->setChecked(!isHeaderHidden());

		QTreeWidgetItem *item = headerItem();
		int columnCount = item->columnCount();

		if (isSortingEnabled() && isHeaderHidden())
		{
			QMenu *headerMenuSort = contextMenu->addMenu(QIcon(),tr("Sort by column …"));

			QActionGroup *actionGroupAsc = new QActionGroup(headerMenuSort);

			QAction *actionSortDescending = headerMenuSort->addAction(FilesDefs::getIconFromQtResourcePath(":/images/sort_decrease.png")
			                                                          , tr("Sort Descending Order"), this, SLOT(changeSortOrder()));
			actionSortDescending->setData("SortDesc");
			actionSortDescending->setCheckable(true);
			actionSortDescending->setChecked(header()->sortIndicatorOrder()==Qt::DescendingOrder);
			actionSortDescending->setActionGroup(actionGroupAsc);

			QAction *actionSortAscending = headerMenuSort->addAction(FilesDefs::getIconFromQtResourcePath(":/images/sort_incr.png")
			                                                         , tr("Sort Ascending Order"), this, SLOT(changeSortOrder()));
			actionSortAscending->setData("SortAsc");
			actionSortAscending->setCheckable(true);
			actionSortAscending->setChecked(header()->sortIndicatorOrder()==Qt::AscendingOrder);
			actionSortAscending->setActionGroup(actionGroupAsc);

			headerMenuSort->addSeparator();

			QActionGroup *actionGroupSort = new QActionGroup(headerMenuSort);

			for (int column = 0; column < columnCount; ++column)
			{
				QString txt = item->text(column) ;
				if(txt == "")
					txt = item->data(column,Qt::UserRole).toString() ;
				if(txt == "")
					txt = item->data(column,Qt::ToolTipRole).toString() ;

				if(txt=="")
					txt = QString::number(column) + tr(" [no title]") ;

				QAction *action = headerMenuSort->addAction(QIcon(), txt, this, SLOT(changeSortColumn()));
				action->setData(column);
				action->setCheckable(true);
				action->setChecked(header()->sortIndicatorSection() == column);
				action->setActionGroup(actionGroupSort);
			}
		}

		QMenu *headerMenuShowCol = contextMenu->addMenu(QIcon(),tr("Show column …"));

		for (int column = 0; column < columnCount; ++column)
		{
			QMap<int, bool>::iterator it = mColumnCustomizable.find(column);
			if (it != mColumnCustomizable.end() && *it == false) {
				continue;
			}
			QString txt = item->text(column) ;
			if(txt == "")
				txt = item->data(column,Qt::UserRole).toString() ;
			if(txt == "")
				txt = item->data(column,Qt::ToolTipRole).toString() ;

			if(txt=="")
				txt = QString::number(column) + tr(" [no title]") ;

			QAction *action = headerMenuShowCol->addAction(QIcon(), txt, this, SLOT(columnVisible()));
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

void RSTreeWidget::headerVisible()
{
	QAction *action = dynamic_cast<QAction*>(sender());
	if (!action) {
		return;
	}

	bool visible = action->isChecked();
	setHeaderHidden(!visible);

	emit headerVisibleChanged(visible);
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

void RSTreeWidget::changeSortColumn()
{
	QAction *action = dynamic_cast<QAction*>(sender());
	if (!action) {
		return;
	}

	if (action->data().canConvert<int>())
	{
		header()->setSortIndicator(action->data().toInt(),header()->sortIndicatorOrder());
	}
}

void RSTreeWidget::changeSortOrder()
{
	QAction *action = dynamic_cast<QAction*>(sender());
	if (!action) {
		return;
	}

	if (action->data().canConvert<QString>())
	{
		if (action->data().toString() == "SortDesc")
			header()->setSortIndicator(header()->sortIndicatorSection(),Qt::DescendingOrder);
		else
			header()->setSortIndicator(header()->sortIndicatorSection(),Qt::AscendingOrder);
	}
}
