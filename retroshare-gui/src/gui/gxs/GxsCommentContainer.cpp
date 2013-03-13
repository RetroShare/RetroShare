/*
 * Retroshare Posted List
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "gui/gxs/GxsCommentContainer.h"
#include "gui/gxs/GxsCommentDialog.h"

#include <iostream>
#include <sstream>

#include <QTimer>
#include <QMessageBox>

/******
 * #define PHOTO_DEBUG 1
 *****/


/****************************************************************
 * Posted Dialog
 *
 */

GxsCommentContainer::GxsCommentContainer(QWidget *parent)
: MainPage(parent)
{
	ui.setupUi(this);

	connect(ui.tabWidget, SIGNAL(tabCloseRequested( int )), this, SLOT(tabCloseRequested( int )));

}


void GxsCommentContainer::setup()
{
	mServiceDialog = createServiceDialog();
	QString name = getServiceName();

	QString list(name);
	QWidget *widget = dynamic_cast<QWidget *>(mServiceDialog);
	ui.tabWidget->addTab(widget, name);
}


void GxsCommentContainer::commentLoad(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId)
{
	QString comments(tr("Comments"));
        GxsCommentDialog *commentDialog = new GxsCommentDialog(this, getTokenService(), getCommentService());

	GxsCommentHeader *commentHeader = createHeaderWidget();
	commentDialog->setCommentHeader(commentHeader);

	commentDialog->commentLoad(grpId, msgId);
	//commentHeader->commentLoad(grpId, msgId);

	ui.tabWidget->addTab(commentDialog, comments);
}



void GxsCommentContainer::tabCloseRequested(int index)
{
	std::cerr << "GxsCommentContainer::tabCloseRequested(" << index << ")";
	if (index != 0)
	{
		QWidget *comments = ui.tabWidget->widget(index);
		ui.tabWidget->removeTab(index);
		delete comments;
	}
	std::cerr << "GxsCommentContainer::tabCloseRequested() Not closing First Tab";
}


















