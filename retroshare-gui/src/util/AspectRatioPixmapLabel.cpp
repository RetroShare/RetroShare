/*******************************************************************************
 * retroshare-gui/src/util/AspectRatioPixmapLabel.cpp                           *
 *                                                                             *
 * Copyright (C) 2019  Retroshare Team       <retroshare.project@gmail.com>    *
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

#include <QApplication>
#include <QClipboard>
#include <QMenu>

#include "AspectRatioPixmapLabel.h"
#include <iostream>
#include "util/imageutil.h"
#include "gui/common/FilesDefs.h"

AspectRatioPixmapLabel::AspectRatioPixmapLabel(QWidget *parent) :
    QLabel(parent)
{
    this->setMinimumSize(1,1);
    setScaledContents(false);
}

void AspectRatioPixmapLabel::setPixmap ( const QPixmap & p)
{
    pix = p;
	QLabel::setPixmap(scaledPixmap());
	//std::cout << "Information size: " << pix.width() << 'x' << pix.height() << std::endl;
}

int AspectRatioPixmapLabel::heightForWidth( int width ) const
{
    return pix.isNull() ? this->height() : ((qreal)pix.height()*width)/pix.width();
}

QSize AspectRatioPixmapLabel::sizeHint() const
{
	return QSize(pix.width(), pix.height());
}

QPixmap AspectRatioPixmapLabel::scaledPixmap() const
{
    return pix.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void AspectRatioPixmapLabel::resizeEvent(QResizeEvent * e)
{
    if(!pix.isNull())
        QLabel::setPixmap(scaledPixmap());
	QLabel::resizeEvent(e);
	//std::cout << "Information resized: " << e->oldSize().width() << 'x' << e->oldSize().height() << " to " << e->size().width() << 'x' << e->size().height() << std::endl;
}

void AspectRatioPixmapLabel::addContextMenuAction(QAction *action)
{
	mContextMenuActions.push_back(action);
}

void AspectRatioPixmapLabel::contextMenuEvent(QContextMenuEvent *event)
{
	emit calculateContextMenuActions();

	QMenu *contextMenu = new QMenu();

	QAction *actionSaveImage = contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/document_save.png"), tr("Save image"), this, SLOT(saveImage()));
	QAction *actionCopyImage = contextMenu->addAction(tr("Copy image"), this, SLOT(copyImage()));

	if (pix.isNull()) {
		actionSaveImage->setEnabled(false);
		actionCopyImage->setEnabled(false);
	} else {
		actionSaveImage->setEnabled(true);
		actionCopyImage->setEnabled(true);
	}

	QList<QAction*>::iterator it;
	for (it = mContextMenuActions.begin(); it != mContextMenuActions.end(); ++it) {
		contextMenu->addAction(*it);
	}

	contextMenu->exec(event->globalPos());

	delete(contextMenu);
}

void AspectRatioPixmapLabel::copyImage()
{
	if (pix.isNull()) {
		return;
	}

	QApplication::clipboard()->setPixmap(pix, QClipboard::Clipboard);
}

void AspectRatioPixmapLabel::saveImage()
{
	if (pix.isNull()) {
		return;
	}

	ImageUtil::saveImage(window(), pix.toImage());
}
