/*******************************************************************************
 * gui/WikiPoos/WikiAddDialog.cpp                                              *
 *                                                                             *
 * Copyright (C) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
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

	
