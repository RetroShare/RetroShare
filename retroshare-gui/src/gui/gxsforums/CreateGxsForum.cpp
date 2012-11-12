/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include <QMessageBox>

#include "util/misc.h"
#include "CreateGxsForum.h"
#include "gui/common/PeerDefs.h"

#include <algorithm>

#include <retroshare/rsgxsforums.h>
#include <retroshare/rspeers.h>

#include <iostream>

#define CREATEGXSFORUMS_NEWFORUMID	1


/** Constructor */
CreateGxsForum::CreateGxsForum(QWidget *parent)
: QDialog(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	mForumQueue = new TokenQueue(rsGxsForums->getTokenService(), this);

	// connect up the buttons.
	connect( ui.cancelButton, SIGNAL( clicked ( bool ) ), this, SLOT( cancelForum( ) ) );
	connect( ui.createButton, SIGNAL( clicked ( bool ) ), this, SLOT( createForum( ) ) );
	connect( ui.pubKeyShare_cb, SIGNAL( clicked() ), this, SLOT( setShareList( ) ));

	if (!ui.pubKeyShare_cb->isChecked()) {
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}

	/* initialize key share list */
	ui.keyShareList->setHeaderText(tr("Contacts:"));
	ui.keyShareList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui.keyShareList->start();

	newForum();
}

void CreateGxsForum::newForum()
{
	/* enforce Public for the moment */
	ui.typePublic->setChecked(true);

	ui.typePrivate->setEnabled(false);
	ui.typeEncrypted->setEnabled(true);

#ifdef RS_RELEASE_VERSION
	ui.typePrivate->setVisible(false);
	ui.typeEncrypted->setVisible(true);
#endif

	ui.msgAnon->setChecked(true);
	//ui.msgAuth->setEnabled(false);

	ui.forumName->clear();
	ui.forumDesc->clear();

	ui.forumName->setFocus();
}

void CreateGxsForum::createForum()
{
	QString name = misc::removeNewLine(ui.forumName->text());
	QString desc = ui.forumDesc->toPlainText(); //toHtml();
	uint32_t flags = 0;

	if(name.isEmpty()) {
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Please add a Name"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty name!!
	}

#ifdef TOGXS
	if (ui.typePublic->isChecked()) {
		flags |= RS_DISTRIB_PUBLIC;
	} else if (ui.typePrivate->isChecked()) {
		flags |= RS_DISTRIB_PRIVATE;
	} else if (ui.typeEncrypted->isChecked()) {
		flags |= RS_DISTRIB_ENCRYPTED;
	}

	if (ui.msgAuth->isChecked()) {
		flags |= RS_DISTRIB_AUTHEN_REQ;
	} else if (ui.msgAnon->isChecked()) {
		flags |= RS_DISTRIB_AUTHEN_ANON;
	}
#endif

	if (rsGxsForums) {
		
		
		uint32_t token;
		RsGxsForumGroup grp;
		grp.mMeta.mGroupName = std::string(name.toUtf8());
		grp.mDescription = std::string(desc.toUtf8());
		grp.mMeta.mGroupFlags = flags;
		
		rsGxsForums->createGroup(token, grp);
		
        // get the Queue to handle response.
        mForumQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_SUMMARY, CREATEGXSFORUMS_NEWFORUMID);
		
	}
}

void CreateGxsForum::completeCreateNewForum(const RsGroupMetaData &newForumMeta)
{
	sendShareList(newForumMeta.mGroupId);
	
	close();
}


void CreateGxsForum::sendShareList(std::string forumId)
{
	if (!rsGxsForums)
	{
		std::cerr << "CreateGxsForum::sendShareList() GxsForums not active";
		std::cerr << std::endl;
		return;
	}
	
	if (ui.pubKeyShare_cb->isChecked()) 
	{
		std::list<std::string> shareList;
		ui.keyShareList->selectedSslIds(shareList, false);
#ifdef TOGXS
		rsGxsForums->groupShareKeys(forumId, shareList);
#endif
	}
	close();
}






void CreateGxsForum::setShareList()
{
	if (ui.pubKeyShare_cb->isChecked()){
		this->resize(this->size().width() + ui.contactsdockWidget->size().width(), this->size().height());
		ui.contactsdockWidget->show();
	} else {  // hide share widget
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}
}

void CreateGxsForum::cancelForum()
{
	close();
}




void CreateGxsForum::loadNewForumId(const uint32_t &token)
{
	std::cerr << "CreateGxsForum::loadNewForumId()";
	std::cerr << std::endl;
	
	std::list<RsGroupMetaData> groupInfo;
	rsGxsForums->getGroupSummary(token, groupInfo);
	
	if (groupInfo.size() == 1)
	{
		RsGroupMetaData fi = groupInfo.front();
		completeCreateNewForum(fi);
	}
	else
	{
		std::cerr << "CreateGxsForum::loadNewForumId() ERROR INVALID Number of Forums Created";
		std::cerr << std::endl;
	}
}








void CreateGxsForum::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "CreateGxsForum::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	if (queue == mForumQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
				
			case CREATEGXSFORUMS_NEWFORUMID:
				loadNewForumId(req.mToken);
				break;
			default:
				std::cerr << "CreateGxsForum::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;
				
		}
	}
}

		
		
