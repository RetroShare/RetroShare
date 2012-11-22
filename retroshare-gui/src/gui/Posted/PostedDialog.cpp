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

#include "PostedDialog.h"

#include "PostedListDialog.h"
#include "PostedComments.h"

#include <retroshare/rsposted.h>

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

PostedDialog::PostedDialog(QWidget *parent)
: MainPage(parent)
{
    ui.setupUi(this);

    mPostedList = new PostedListDialog(NULL);
    mPostedComments = new PostedComments(NULL);

    QString list("List");
    ui.tabWidget->addTab(mPostedList, list);
    QString comments("Comments");
    ui.tabWidget->addTab(mPostedComments, comments);

    connect(mPostedList, SIGNAL(loadComments( std::string ) ), mPostedComments, SLOT(loadComments( std::string ) ) );
}

void PostedDialog::commentLoad(const RsGxsMessageId &msgId)
{
    mPostedComments->loadComments(msgId);
}


















