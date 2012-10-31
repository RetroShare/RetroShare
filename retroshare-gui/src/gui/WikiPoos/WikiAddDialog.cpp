/*
 * Retroshare Wiki Plugin.
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

#include "gui/WikiPoos/WikiAddDialog.h"
#include <retroshare/rswiki.h>

#include <iostream>

/** Constructor */
WikiAddDialog::WikiAddDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_Cancel, SIGNAL( clicked( void ) ), this, SLOT( cancelGroup( void ) ) );
	connect(ui.pushButton_Create, SIGNAL( clicked( void ) ), this, SLOT( createGroup( void ) ) );


}


void WikiAddDialog::cancelGroup()
{
	clearDialog();
	hide();
}



void WikiAddDialog::createGroup()
{
	std::cerr << "WikiAddDialog::createGroup()";
	std::cerr << std::endl;


	RsWikiCollection group;

#if 0
	group.mShareOptions.mShareType = 0;
	group.mShareOptions.mShareGroupId = "unknown";
	group.mShareOptions.mPublishKey = "unknown";
	group.mShareOptions.mCommentMode = 0;
	group.mShareOptions.mResizeMode = 0;
#endif

	group.mMeta.mGroupName = ui.lineEdit_Name->text().toStdString();
	group.mCategory = "Unknown";

	uint32_t token;
	bool isNew = true;

#if 0
	// Don't worry about getting the response?
	rsWiki->createGroup(token, group, isNew);
#endif


	clearDialog();
	hide();
}


void WikiAddDialog::clearDialog()
{
	ui.lineEdit_Name->setText(QString("title"));
}

	
