/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCommentContainer.cpp                          *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#include "gui/gxs/GxsCommentContainer.h"
#include "gui/gxs/GxsCommentDialog.h"

#include <iostream>

#define MAX_COMMENT_TITLE 32

/****************************************************************
 * GxsCommentContainer
 *
 */

GxsCommentContainer::GxsCommentContainer(QWidget *parent)
: MainPage(parent)
{
	ui.setupUi(this);

	connect(ui.tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
}

void GxsCommentContainer::setup()
{
	mServiceDialog = createServiceDialog();

	QString name = getServiceName();
	ui.titleBarLabel->setText(name);
	ui.titleBarPixmap->setPixmap(getServicePixmap());

	QWidget *widget = dynamic_cast<QWidget *>(mServiceDialog);
	int index = ui.tabWidget->addTab(widget, name);
	ui.tabWidget->hideCloseButton(index);
}

void GxsCommentContainer::commentLoad(const RsGxsGroupId &grpId, const std::set<RsGxsMessageId>& msg_versions,const RsGxsMessageId &msgId, const QString &title)
{
	QString comments = title;
	if (title.length() > MAX_COMMENT_TITLE)
	{
		comments.truncate(MAX_COMMENT_TITLE - 3);
		comments += "...";
	}

	GxsCommentDialog *commentDialog = new GxsCommentDialog(this, getTokenService(), getCommentService());

	QWidget *commentHeader = createHeaderWidget(grpId, msgId);
	commentDialog->setCommentHeader(commentHeader);

	commentDialog->commentLoad(grpId, msg_versions, msgId);

	ui.tabWidget->addTab(commentDialog, comments);
}

void GxsCommentContainer::tabCloseRequested(int index)
{
	std::cerr << "GxsCommentContainer::tabCloseRequested(" << index << ")";
	std::cerr << std::endl;

	if (index != 0)
	{
		QWidget *comments = ui.tabWidget->widget(index);
		ui.tabWidget->removeTab(index);
		delete comments;
	}
	else
	{
		std::cerr << "GxsCommentContainer::tabCloseRequested() Not closing First Tab";
		std::cerr << std::endl;
	}
}
