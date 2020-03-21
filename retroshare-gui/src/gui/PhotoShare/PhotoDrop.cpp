/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoDialog.cpp                           *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#include <QtGui>
#include <QGridLayout>

#include "PhotoDrop.h"

#include <iostream>

// Helper Class
class gridIndex
{
	public:
		gridIndex()
		:row(0), column(0) { return; }

		gridIndex(int in_row, int in_column)
		:row(in_row), column(in_column) { return; }
		
		bool operator<(const gridIndex &other) const
		{
			if (row == other.row)
			{
				return (column < other.column);
			}
			return (row < other.row);
		}
		
		int row, column;
};


#define DEFAULT_ORDER_INCREMENT (100)

PhotoDrop::PhotoDrop(QWidget *parent)
	:QWidget(parent), mLastOrder(0)
{
	setAcceptDrops(true);

	mSelected = NULL;
	checkMoveButtons();
	reorderPhotos();
}


void PhotoDrop::clear()
{
}


PhotoItem *PhotoDrop::getSelectedPhotoItem()
{
	return mSelected;
}



void PhotoDrop::resizeEvent ( QResizeEvent * event ) 
{
	/* calculate the preferred number of columns for the PhotoItems */
	reorderPhotos();
}

int	PhotoDrop::getPhotoCount()
{
	std::cerr << "PhotoDrop::getPhotoCount()";
	std::cerr << std::endl;

	/* grab the first PhotoItem - and get it size */
	const QObjectList &items = children();
	QObjectList::const_iterator qit;
	int count = 0;
	for(qit = items.begin(); qit != items.end(); ++qit)
	{
		PhotoItem *item = dynamic_cast<PhotoItem *>(*qit);
		if (item)
		{
			std::cerr << "PhotoDrop::getPhotoCount() item: " << item;
			std::cerr << std::endl;

			++count;
		}
		else
		{
			std::cerr << "PhotoDrop::getPhotoCount() Found Child, which is not a PhotoItem???";
			std::cerr << std::endl;
		}
	}

	return count;
}



PhotoItem *PhotoDrop::getPhotoIdx(int idx)
{
	std::cerr << "PhotoDrop::getPhotoIdx(" << idx << ")";
	std::cerr << std::endl;

	/* grab the first PhotoItem - and get it size */
	const QObjectList &items = children();
	QObjectList::const_iterator qit;

	int count = 0;
	for(qit = items.begin(); qit != items.end(); ++qit)
	{
		PhotoItem *item = dynamic_cast<PhotoItem *>(*qit);
		if (item)
		{
			std::cerr << "PhotoDrop::getPhotoIdx() item: " << item;
			std::cerr << std::endl;
			if (count == idx)
			{
				return item;
			}

			++count;
		}
		else
		{
			std::cerr << "PhotoDrop::getPhotoIdx() Found Child, which is not a PhotoItem???";
			std::cerr << std::endl;
		}
	}

	return NULL;
}

void PhotoDrop::getPhotos(QSet<PhotoItem *> &photos)
{
	photos = mPhotos;
}


void PhotoDrop::reorderPhotos()
{
	std::cerr << "PhotoDrop::reorderPhotos()";
	std::cerr << std::endl;

	/* now move the qwidgets around */
	QLayout *alayout = layout();
	QGridLayout *glayout = dynamic_cast<QGridLayout *>(alayout);
	if (!glayout)
	{
		std::cerr << "PhotoDrop::reorderPhotos() not GridLayout... not much we can do!";
		std::cerr << std::endl;
		return;
	}

	/* grab the first PhotoItem - and get it size */
	std::map<gridIndex, PhotoItem *> photoItems;
	std::map<gridIndex, PhotoItem *>::iterator pit;

	int count = glayout->count();
	int i = 0;
	for(i = 0; i < count; ++i)
	{
		QLayoutItem *litem = glayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "PhotoDrop::reorderPhotos() missing litem";
			std::cerr << std::endl;
			continue;
		}

		PhotoItem *item = dynamic_cast<PhotoItem *>(litem->widget());
		if (item)
		{
			
			int selectedRow;
			int selectedColumn;
			int rowSpan;
			int colSpan;
			glayout->getItemPosition(i, &selectedRow, &selectedColumn, &rowSpan, &colSpan);

			std::cerr << "PhotoDrop::reorderPhotos() item: " << item;
			std::cerr << " layoutIdx: " << i;
			std::cerr << " item pos(" << selectedRow << ", " << selectedColumn;
			std::cerr << ")" << std::endl;
			
			gridIndex idx(selectedRow, selectedColumn);
			photoItems[idx] = item;
	
		}
		else
		{
			std::cerr << "PhotoDrop::reorderPhotos() Found Child, which is not a PhotoItem???";
			std::cerr << std::endl;
		}
	}

	pit = photoItems.begin(); 
	if (pit == photoItems.end())
	{
		std::cerr << "PhotoDrop::reorderPhotos() No PhotoItems.";
		std::cerr << std::endl;

		mColumns = 1;

		// no PhotoItems here.
		return;
	}

	int minWidth = (pit->second)->minimumWidth();

#define EXPECTED_WIDTH 	(200)
	if (minWidth < EXPECTED_WIDTH)
	{
		minWidth = EXPECTED_WIDTH;
	}
	int space = width();
	mColumns = space / minWidth;
	// incase its too thin!
	if (mColumns < 1)
	{
		mColumns = 1;
	}

	std::cerr << "PhotoDrop::reorderPhotos() minWidth: " << minWidth << " space: " << space;
	std::cerr << " columns: " << mColumns;
	std::cerr << std::endl;

	std::cerr << "PhotoDrop::reorderPhotos() Getting ordered Items from Layout";
	std::cerr << std::endl;


	for(pit = photoItems.begin(); pit != photoItems.end(); ++pit)
	{
		glayout->removeWidget(pit->second);
	}
	
	for(pit = photoItems.begin(), i = 0; pit != photoItems.end(); ++pit, ++i)
	{
		int row = i / mColumns;
		int column = i % mColumns;

		std::cerr << "Inserting item: " << pit->second << " at (" << row << "," << column << ")";
		std::cerr << std::endl;
		glayout->addWidget(pit->second, row, column, Qt::AlignCenter);
	}
}


void PhotoDrop::moveLeft()
{
	std::cerr << "PhotoDrop::moveLeft()";
	std::cerr << std::endl;

	QLayout *alayout = layout();
	if (!alayout)
	{
		std::cerr << "PhotoDrop::moveLeft() No Layout";
		std::cerr << std::endl;
		return;
	}

	QGridLayout *glayout = dynamic_cast<QGridLayout *>(alayout);
	if (!glayout)
	{
		std::cerr << "PhotoDrop::moveLeft() not GridLayout... not much we can do!";
		std::cerr << std::endl;
		return;
	}
	
	int count = alayout->count();
	if ((!mSelected) || (count < 2))
	{
		std::cerr << "PhotoDrop::moveLeft() Not enough items";
		std::cerr << std::endl;
		return;
	}

	int index = alayout->indexOf(mSelected);
	int selectedRow;
	int selectedColumn;
	int rowSpan;
	int colSpan;
	glayout->getItemPosition(index, &selectedRow, &selectedColumn, &rowSpan, &colSpan);

	if ((selectedRow == 0) && (selectedColumn == 0))
	{
		std::cerr << "PhotoDrop::moveLeft() Selected is first item";
		std::cerr << std::endl;
		return;
	}

	int swapRow = selectedRow;
	int swapColumn = selectedColumn - 1;

	if (swapColumn < 0)
	{
		swapRow--;
		swapColumn = mColumns - 1;
	}

	std::cerr << "PhotoDrop::moveLeft() Trying to swap (" << selectedRow << ",";
	std::cerr << selectedColumn << ") <-> (" << swapRow;
	std::cerr << "," << swapColumn << ")";
	std::cerr << std::endl;

	QLayoutItem *litem = glayout->itemAtPosition(swapRow, swapColumn);
	if (!litem)
	{
		std::cerr << "PhotoDrop::moveLeft() No layout item to the right";
		std::cerr << std::endl;
		return;
	}

	QWidget *widget = litem->widget();
	if (!widget)
	{
		std::cerr << "PhotoDrop::moveLeft() No item to the left";
		std::cerr << std::endl;
		return;
	}

	/* grab both items, and switch */
	std::cerr << "PhotoDrop::moveLeft() could move index: " << index;
	std::cerr << std::endl;

	glayout->removeWidget(widget);
	glayout->removeWidget(mSelected);

	glayout->addWidget(widget, selectedRow, selectedColumn, Qt::AlignCenter);
	glayout->addWidget(mSelected, swapRow, swapColumn, Qt::AlignCenter);

	checkMoveButtons();
}


void PhotoDrop::moveRight()
{
	std::cerr << "PhotoDrop::moveRight()";
	std::cerr << std::endl;

	QLayout *alayout = layout();
	if (!alayout)
	{
		std::cerr << "PhotoDrop::moveRight() No Layout";
		std::cerr << std::endl;
		return;
	}

	QGridLayout *glayout = dynamic_cast<QGridLayout *>(alayout);
	if (!glayout)
	{
		std::cerr << "PhotoDrop::moveRight() not GridLayout... not much we can do!";
		std::cerr << std::endl;
		return;
	}
	
	int count = alayout->count();
	if ((!mSelected) || (count < 2))
	{
		std::cerr << "PhotoDrop::moveRight() Not enough items";
		std::cerr << std::endl;
		return;
	}

	int index = alayout->indexOf(mSelected);
	int selectedRow;
	int selectedColumn;
	int rowSpan;
	int colSpan;
	glayout->getItemPosition(index, &selectedRow, &selectedColumn, &rowSpan, &colSpan);

	int maxRow = (count - 1) / mColumns;
	int maxCol = (count - 1) % mColumns;
	if ((selectedRow == maxRow) && (selectedColumn == maxCol))
	{
		std::cerr << "PhotoDrop::moveRight() Selected is last item";
		std::cerr << std::endl;
		return;
	}

	int swapRow = selectedRow;
	int swapColumn = selectedColumn + 1;

	if (swapColumn == mColumns)
	{
		++swapRow;
		swapColumn = 0;
	}

	std::cerr << "PhotoDrop::moveRight() Trying to swap (" << selectedRow << ",";
	std::cerr << selectedColumn << ") <-> (" << swapRow;
	std::cerr << "," << swapColumn << ")";
	std::cerr << std::endl;

	QLayoutItem *litem = glayout->itemAtPosition(swapRow, swapColumn);
	if (!litem)
	{
		std::cerr << "PhotoDrop::moveRight() No layout item to the right";
		std::cerr << std::endl;
		return;
	}

	QWidget *widget = litem->widget();

	if (!widget)
	{
		std::cerr << "PhotoDrop::moveRight() No item to the right";
		std::cerr << std::endl;
		return;
	}

	/* grab both items, and switch */
	std::cerr << "PhotoDrop::moveRight() could move index: " << index;
	std::cerr << std::endl;

	glayout->removeWidget(widget);
	glayout->removeWidget(mSelected);

	glayout->addWidget(widget, selectedRow, selectedColumn, Qt::AlignCenter);
	glayout->addWidget(mSelected, swapRow, swapColumn, Qt::AlignCenter);

	checkMoveButtons();
}


void PhotoDrop::checkMoveButtons()
{
	std::cerr << "PhotoDrop::checkMoveButtons()";
	std::cerr << std::endl;
	/* locate mSelected in the set */
	QLayout *alayout = layout();
	if (!alayout)
	{
		std::cerr << "PhotoDrop::checkMoveButtons() No Layout";
		std::cerr << std::endl;
		return;
	}
	
	int count = alayout->count();
	if ((!mSelected) || (count < 2))
	{
		buttonStatus(PHOTO_SHIFT_NO_BUTTONS);
		return;
	}

	QGridLayout *glayout = dynamic_cast<QGridLayout *>(alayout);
	if (!glayout)
	{
		std::cerr << "PhotoDrop::checkMoveButtons() not GridLayout... not much we can do!";
		std::cerr << std::endl;
		buttonStatus(PHOTO_SHIFT_NO_BUTTONS);
		return;
	}

	int index = alayout->indexOf(mSelected);
	int selectedRow;
	int selectedColumn;
	int rowSpan;
	int colSpan;
	glayout->getItemPosition(index, &selectedRow, &selectedColumn, &rowSpan, &colSpan);

	int maxRow = (count - 1) / mColumns;
	int maxCol = (count - 1) % mColumns;
	if ((selectedRow == 0) && (selectedColumn == 0))
	{
		buttonStatus(PHOTO_SHIFT_RIGHT_ONLY);
	}
	else if ((selectedRow == maxRow) && (selectedColumn == maxCol))
	{
		buttonStatus(PHOTO_SHIFT_LEFT_ONLY);
	}
	else
	{
		buttonStatus(PHOTO_SHIFT_BOTH);
	}
}


void PhotoDrop::clearPhotos()
{
	std::cerr << "PhotoDrop::clearPhotos()";
	std::cerr << std::endl;

	/* grab the first PhotoItem - and get it size */
	const QObjectList &items = children();
	QObjectList::const_iterator qit;
	std::list<PhotoItem *> photoItems;
	std::list<PhotoItem *>::iterator pit;

	for(qit = items.begin(); qit != items.end(); ++qit)
	{
		PhotoItem *item = dynamic_cast<PhotoItem *>(*qit);
		if (item)
		{
			photoItems.push_back(item);
			std::cerr << "PhotoDrop::clearPhotos() item: " << item;
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "PhotoDrop::clearPhotos() Found Child, which is not a PhotoItem???";
			std::cerr << std::endl;
		}
	}

	QLayout *alayout = layout();
	QGridLayout *glayout = dynamic_cast<QGridLayout *>(alayout);
	if (!glayout)
	{
		std::cerr << "PhotoDrop::clearPhotos() not GridLayout... not much we can do!";
		std::cerr << std::endl;
		return;
	}

	for(pit = photoItems.begin(); pit != photoItems.end(); ++pit)
	{
		PhotoItem *item = *pit;
		glayout->removeWidget(item);
		delete item;
	}

	mSelected = NULL;
}

	


void PhotoDrop::dragEnterEvent(QDragEnterEvent *event)
{
	// Only Accept DragEntetEvents from the Disk / URLs.
	// TODO: check that the data is suitable - and a photo

	std::cerr << "PhotoDrop::dragEnterEvent()";
	std::cerr << std::endl;

	if (event->mimeData()->hasUrls())
	{
		std::cerr << "PhotoDrop::dragEnterEvent() Accepting";
		std::cerr << std::endl;
		event->accept();
	}
	else
	{
		std::cerr << "PhotoDrop::dragEnterEvent() Ignoring";
		std::cerr << std::endl;
		event->ignore();
	}
}

void PhotoDrop::dragLeaveEvent(QDragLeaveEvent *event)
{
	// We can drag an existing Image to the "Album Spot" (or elsewhere).
	// But we cannot drag anything else out.

	std::cerr << "PhotoDrop::dragLeaveEvent()";
	std::cerr << std::endl;

	event->ignore();
}

void PhotoDrop::dragMoveEvent(QDragMoveEvent *event)
{

	std::cerr << "PhotoDrop::dragMoveEvent()";
	std::cerr << std::endl;

	event->accept();
	//event->ignore();
}

void PhotoDrop::dropEvent(QDropEvent *event)
{
	std::cerr << "PhotoDrop::dropEvent()";
	std::cerr << std::endl;


	if (event->mimeData()->hasUrls())
	{
		std::cerr << "PhotoDrop::dropEvent() Urls:" << std::endl;
		
		QList<QUrl> urls = event->mimeData()->urls();
		QList<QUrl>::iterator uit;
		for (uit = urls.begin(); uit != urls.end(); ++uit)
		{
			QString localpath = uit->toLocalFile();
			std::cerr << "Whole URL: " << uit->toString().toStdString() << std::endl;
			std::cerr << "or As Local File: " << localpath.toStdString() << std::endl;

			PhotoItem* item = new PhotoItem(mHolder, localpath, mLastOrder+DEFAULT_ORDER_INCREMENT);
			addPhotoItem(item);
		}
		event->setDropAction(Qt::CopyAction);
		event->accept();

		// Notify Listeners. (only happens for drop - not programmatically added).
		photosChanged();
	}
	else
	{
		std::cerr << "PhotoDrop::dropEvent Ignoring";
		std::cerr << std::endl;
		event->ignore();
	}

	checkMoveButtons();

}

void PhotoDrop::mousePressEvent(QMouseEvent *event)
{
	/* see if this is in the space of one of our children */
	QPoint pos = event->pos();

	std::cerr << "PhotoDrop::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
	std::cerr << std::endl;

	QWidget::mousePressEvent(event);
}

void PhotoDrop::setPhotoItemHolder(PhotoShareItemHolder *holder)
{
	mHolder = holder;
}

void PhotoDrop::addPhotoItem(PhotoItem *item)
{
	std::cerr << "PhotoDrop::addPhotoItem()";
	std::cerr << std::endl;

	// record lastOrder number
	// so any new photos can be added after this.
	if (item->getPhotoDetails().mOrder > mLastOrder) {
		mLastOrder = item->getPhotoDetails().mOrder;
	}

	mPhotos.insert(item);
	layout()->addWidget(item);
	
	//checkMoveButtons();
}

bool PhotoDrop::deletePhoto(PhotoItem *item)
{
	if (mPhotos.contains(item)) {
		mPhotos.remove(item);
		layout()->removeWidget(item);
		delete item;
	}
	else
		return false;
}
