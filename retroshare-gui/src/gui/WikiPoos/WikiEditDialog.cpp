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


#include "gui/WikiPoos/WikiEditDialog.h"

#include <iostream>

/** Constructor */
WikiEditDialog::WikiEditDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_Cancel, SIGNAL( clicked( void ) ), this, SLOT( cancelEdit( void ) ) );
	connect(ui.pushButton_Revert, SIGNAL( clicked( void ) ), this, SLOT( revertEdit( void ) ) );
	connect(ui.pushButton_Submit, SIGNAL( clicked( void ) ), this, SLOT( submitEdit( void ) ) );

	mWikiQueue = new TokenQueue(rsWiki->getTokenService(), this);
        mRepublishMode = false;
}

void WikiEditDialog::setGroup(RsWikiCollection &group)
{
	std::cerr << "WikiEditDialog::setGroup(): " << group;
	std::cerr << std::endl;

	mWikiCollection = group;

	ui.lineEdit_Group->setText(QString::fromStdString(mWikiCollection.mMeta.mGroupName));
}


void WikiEditDialog::setPreviousPage(RsWikiSnapshot &page)
{
	std::cerr << "WikiEditDialog::setPreviousPage(): " << page;
	std::cerr << std::endl;

	mNewPage = false;
	mWikiSnapshot = page;

	ui.lineEdit_Page->setText(QString::fromStdString(mWikiSnapshot.mMeta.mMsgName));
	ui.lineEdit_PrevVersion->setText(QString::fromStdString(mWikiSnapshot.mMeta.mMsgId));
	ui.textEdit->setPlainText(QString::fromStdString(mWikiSnapshot.mPage));
}


void WikiEditDialog::setNewPage()
{
	mNewPage = true;
        mRepublishMode = false;
	ui.lineEdit_Page->setText("");
	ui.lineEdit_PrevVersion->setText("");
	ui.textEdit->setPlainText("");
}


void WikiEditDialog::setRepublishMode(RsGxsMessageId &origMsgId)
{
        mRepublishMode = true;
        mRepublishOrigId = origMsgId;
}



void WikiEditDialog::cancelEdit()
{
	hide();
}


void WikiEditDialog::revertEdit()
{
	if (mNewPage)
	{
		ui.textEdit->setPlainText("");
	}
	else
	{
		ui.textEdit->setPlainText(QString::fromStdString(mWikiSnapshot.mPage));
	}
}


void WikiEditDialog::submitEdit()
{
	std::cerr << "WikiEditDialog::submitEdit()";
	std::cerr << std::endl;

	if (mNewPage)
	{
		mWikiSnapshot.mMeta.mGroupId = mWikiCollection.mMeta.mGroupId;
		mWikiSnapshot.mMeta.mOrigMsgId = "";
		mWikiSnapshot.mMeta.mMsgId = "";
#if 0
		mWikiSnapshot.mPrevId = "";
#endif
		std::cerr << "WikiEditDialog::submitEdit() Is New Page";
		std::cerr << std::endl;
	}
	else if (mRepublishMode)
	{
		std::cerr << "WikiEditDialog::submitEdit() In Republish Mode";
		std::cerr << std::endl;
		// A New Version of the ThreadHead.
		mWikiSnapshot.mMeta.mGroupId = mWikiCollection.mMeta.mGroupId;
		mWikiSnapshot.mMeta.mOrigMsgId = mRepublishOrigId;
		mWikiSnapshot.mMeta.mParentId = "";
		mWikiSnapshot.mMeta.mThreadId = "";
		mWikiSnapshot.mMeta.mMsgId = "";
	}
	else
	{
		std::cerr << "WikiEditDialog::submitEdit() In Child Edit Mode";
		std::cerr << std::endl;

		// A Child of the current message.
		bool isFirstChild = false;
		if (mWikiSnapshot.mMeta.mParentId == "")
		{
			isFirstChild = true;
		}

		mWikiSnapshot.mMeta.mGroupId = mWikiCollection.mMeta.mGroupId;

		if (isFirstChild)
		{
			mWikiSnapshot.mMeta.mThreadId = mWikiSnapshot.mMeta.mOrigMsgId;
			// Special HACK here... parentId points to specific Msg, rather than OrigMsgId.
			// This allows versioning to work well.
			mWikiSnapshot.mMeta.mParentId = mWikiSnapshot.mMeta.mMsgId;
		}
		else
		{
			// ThreadId is the same.
			mWikiSnapshot.mMeta.mParentId = mWikiSnapshot.mMeta.mOrigMsgId;
		}

		mWikiSnapshot.mMeta.mMsgId = "";
		mWikiSnapshot.mMeta.mOrigMsgId = "";
	}


	mWikiSnapshot.mMeta.mMsgName = ui.lineEdit_Page->text().toStdString();
	mWikiSnapshot.mPage = ui.textEdit->toPlainText().toStdString();

	std::cerr << "WikiEditDialog::submitEdit() PageTitle: " << mWikiSnapshot.mMeta.mMsgName;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() GroupId: " << mWikiSnapshot.mMeta.mGroupId;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() OrigMsgId: " << mWikiSnapshot.mMeta.mOrigMsgId;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() MsgId: " << mWikiSnapshot.mMeta.mMsgId;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() ThreadId: " << mWikiSnapshot.mMeta.mThreadId;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() ParentId: " << mWikiSnapshot.mMeta.mParentId;
	std::cerr << std::endl;

	uint32_t token;
	//bool  isNew = mNewPage;
	//rsWiki->createPage(token, mWikiSnapshot, isNew);
	rsWiki->submitSnapshot(token, mWikiSnapshot);
	hide();
}

void WikiEditDialog::setupData(const std::string &groupId, const std::string &pageId)
{
        mRepublishMode = false;
	if (groupId != "")
	{
		requestGroup(groupId);
	}

	if (pageId != "")
	{
        	RsGxsGrpMsgIdPair msgId = std::make_pair(groupId, pageId);
		requestPage(msgId);
	}
}


/***** Request / Response for Data **********/

void WikiEditDialog::requestGroup(const std::string &groupId)
{
        std::cerr << "WikiEditDialog::requestGroup()";
        std::cerr << std::endl;

        std::list<std::string> ids;
	ids.push_back(groupId);

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;
        mWikiQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, ids, 0);
}

void WikiEditDialog::loadGroup(const uint32_t &token)
{
        std::cerr << "WikiEditDialog::loadGroup()";
        std::cerr << std::endl;

	std::vector<RsWikiCollection> groups;
	if (rsWiki->getCollections(token, groups))
	{
		if (groups.size() != 1)
		{
        		std::cerr << "WikiEditDialog::loadGroup() ERROR No group data";
        		std::cerr << std::endl;
			return;
		}
		setGroup(groups[0]);
	}
}

void WikiEditDialog::requestPage(const RsGxsGrpMsgIdPair &msgId)
{
        std::cerr << "WikiEditDialog::requestPage()";
        std::cerr << std::endl;

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

        GxsMsgReq msgIds;
        std::vector<RsGxsMessageId> &vect_msgIds = msgIds[msgId.first];
        vect_msgIds.push_back(msgId.second);

	uint32_t token;
        mWikiQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, 0);
}

void WikiEditDialog::loadPage(const uint32_t &token)
{
        std::cerr << "WikiEditDialog::loadPage()";
        std::cerr << std::endl;

	std::vector<RsWikiSnapshot> snapshots;

	if (rsWiki->getSnapshots(token, snapshots))
	{
		if (snapshots.size() != 1)
		{
        		std::cerr << "WikiEditDialog::loadGroup() ERROR No group data";
        		std::cerr << std::endl;
			return;
		}
		setPreviousPage(snapshots[0]);
	}
}

void WikiEditDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "WikiEditDialog::loadRequest()";
	std::cerr << std::endl;
		
	if (queue == mWikiQueue)
	{
		/* now switch on req */
		switch(req.mType)
		{
		case TOKENREQ_GROUPINFO:
			loadGroup(req.mToken);
			break;
		case TOKENREQ_MSGINFO:
			loadPage(req.mToken);
			break;
		default:
			std::cerr << "WikiEditDialog::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;
		break;
		}
	}
}




