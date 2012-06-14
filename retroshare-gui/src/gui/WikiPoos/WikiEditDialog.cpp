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

	mWikiQueue = new TokenQueue(rsWiki, this);
}

void WikiEditDialog::setGroup(RsWikiGroup &group)
{
	mWikiGroup = group;

	ui.lineEdit_Group->setText(QString::fromStdString(mWikiGroup.mMeta.mGroupName));
}


void WikiEditDialog::setPreviousPage(RsWikiPage &page)
{
	mNewPage = false;
	mWikiPage = page;

	ui.lineEdit_Page->setText(QString::fromStdString(mWikiPage.mMeta.mMsgName));
	ui.lineEdit_PrevVersion->setText(QString::fromStdString(mWikiPage.mMeta.mMsgId));
	ui.textEdit->setPlainText(QString::fromStdString(mWikiPage.mPage));
}


void WikiEditDialog::setNewPage()
{
	mNewPage = true;
	ui.lineEdit_Page->setText("");
	ui.lineEdit_PrevVersion->setText("");
	ui.textEdit->setPlainText("");
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
		ui.textEdit->setPlainText(QString::fromStdString(mWikiPage.mPage));
	}
}


void WikiEditDialog::submitEdit()
{
	if (mNewPage)
	{
		mWikiPage.mMeta.mGroupId = mWikiGroup.mMeta.mGroupId;
		mWikiPage.mMeta.mOrigMsgId = "";
		mWikiPage.mMeta.mMsgId = "";
		mWikiPage.mPrevId = "";
	}
	else
	{
		mWikiPage.mPrevId = mWikiPage.mMeta.mMsgId;
		mWikiPage.mMeta.mMsgId = "";
	}

	mWikiPage.mMeta.mMsgName = ui.lineEdit_Page->text().toStdString();
	mWikiPage.mPage = ui.textEdit->toPlainText().toStdString();

	rsWiki->createPage(mWikiPage);
	hide();
}

void WikiEditDialog::setupData(const std::string &groupId, const std::string &pageId)
{
	if (groupId != "")
	{
		requestGroup(groupId);
	}

	if (pageId != "")
	{
		requestPage(pageId);
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
	uint32_t token;
        mWikiQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, ids, 0);
}

void WikiEditDialog::loadGroup(const uint32_t &token)
{
        std::cerr << "WikiEditDialog::loadGroup()";
        std::cerr << std::endl;

	RsWikiGroup group;
	if (rsWiki->getGroupData(token, group))
	{
		setGroup(group);
	}
}

void WikiEditDialog::requestPage(const std::string &msgId)
{
        std::cerr << "WikiEditDialog::requestGroup()";
        std::cerr << std::endl;

        std::list<std::string> ids;
	ids.push_back(msgId);
        RsTokReqOptions opts;

	uint32_t token;
        mWikiQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, ids, 0);
}

void WikiEditDialog::loadPage(const uint32_t &token)
{
        std::cerr << "WikiEditDialog::loadPage()";
        std::cerr << std::endl;

	RsWikiPage page;
	if (rsWiki->getMsgData(token, page))
	{
		setPreviousPage(page);
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
		case TOKENREQ_MSGRELATEDINFO:
			loadPage(req.mToken);
			break;
		default:
			std::cerr << "WikiEditDialog::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;
		break;
		}
	}
}




