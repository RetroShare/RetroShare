/*******************************************************************************
 * gui/feeds/FeedHolder.cpp                                                    *
 *                                                                             *
 * Copyright (c) 2008, Robert Fernie   <retroshare.project@gmail.com>          *
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

#include <QLayout>
#include <QApplication>
#include <QScrollArea>

#include "FeedHolder.h"

/** Constructor */
FeedHolder::FeedHolder()
{
	mLockCount = 0;
}

// Workaround for QTBUG-3372
void FeedHolder::lockLayout(QWidget *feedItem, bool lock)
{
	if (lock) {
		if (mLockCount == 0) {
			QScrollArea *scrollArea = getScrollArea();
			if (scrollArea) {
				// disable update
				scrollArea->setUpdatesEnabled(false);

				QWidget *widget = scrollArea->widget();
				if (widget && widget->layout()) {
					// disable layout
					widget->layout()->setEnabled(false);
				}
			}
		}
		++mLockCount;
	} else {
		--mLockCount;
		if (mLockCount == 0) {
			QScrollArea *scrollArea = getScrollArea();
			if (scrollArea) {
				if (feedItem && !feedItem->isHidden()) {
					// show window without hide it
					// something in show causes a recalculation of the layout
					feedItem->setAttribute(Qt::WA_WState_Hidden, true);
					feedItem->show();
				}

				// enable update
				scrollArea->setUpdatesEnabled(true);

				// recalculate layout
				QWidget *widget = scrollArea->widget();
				if (widget && widget->layout()) {
					// enable layout
					widget->layout()->setEnabled(true);
				}

				// send layout request (without event queue) but with the newly calculated sizeHint from the call to :show
				QApplication::sendEvent(scrollArea, new QEvent(QEvent::LayoutRequest));
			}
		}
		mLockCount = qMax(mLockCount, 0);
	}
}
