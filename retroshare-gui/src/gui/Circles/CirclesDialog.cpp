/*
 * Retroshare Circle Plugin.
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

#include <QFile>
#include <QFileInfo>

#include "gui/Circles/CirclesDialog.h"

#include <retroshare/rsgxscircles.h>

#include <iostream>
#include <sstream>

#include <QTimer>

/******
 * #define CIRCLE_DEBUG 1
 *****/


#if 0
#define WIKIDIALOG_LISTING_GROUPDATA		2
#define WIKIDIALOG_LISTING_PAGES		5
#define WIKIDIALOG_MOD_LIST			6
#define WIKIDIALOG_MOD_PAGES			7
#define WIKIDIALOG_WIKI_PAGE			8

#define WIKIDIALOG_EDITTREE_DATA		9

#endif


/** Constructor */
CirclesDialog::CirclesDialog(QWidget *parent)
: MainPage(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

#if 0
	mAddPageDialog = NULL;
	mAddGroupDialog = NULL;
	mEditDialog = NULL;

	connect( ui.toolButton_NewGroup, SIGNAL(clicked()), this, SLOT(OpenOrShowAddGroupDialog()));
	connect( ui.toolButton_NewPage, SIGNAL(clicked()), this, SLOT(OpenOrShowAddPageDialog()));
	connect( ui.toolButton_Edit, SIGNAL(clicked()), this, SLOT(OpenOrShowEditDialog()));
	connect( ui.toolButton_Republish, SIGNAL(clicked()), this, SLOT(OpenOrShowRepublishDialog()));

	// Usurped until Refresh works normally
	connect( ui.toolButton_Delete, SIGNAL(clicked()), this, SLOT(insertWikiGroups()));

	connect( ui.treeWidget_Pages, SIGNAL(itemSelectionChanged()), this, SLOT(groupTreeChanged()));

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	/* setup TokenQueue */
        rsWiki->generateDummyData();
        mCircleQueue = new TokenQueue(rsWiki->getTokenService(), this);
#endif

	connect( ui.treeWidget_membership, SIGNAL(itemSelectionChanged()), this, SLOT(circle_selected()));
	connect( ui.treeWidget_friends, SIGNAL(itemSelectionChanged()), this, SLOT(friend_selected()));
	connect( ui.treeWidget_category, SIGNAL(itemSelectionChanged()), this, SLOT(category_selected()));


}


#if 0

void CirclesDialog::checkUpdate()
{

	/* update */
	if (!rsWiki)
		return;

	if (rsWiki->updated())
	{
		insertWikiGroups();
	}

	return;
}




void CirclesDialog::update()
{
	/* load all circles, personal groups 
	 * into the lists.
	 */

	/* load friend list */

	/* load categories */

	std::list<RsGxsCircleId> public_circles;
	rsGxsCircles->getPublicCircles(public_circle);



}


void CirclesDialog::circle_selected()
{
	/* update friend lists */

	/* if personal group, update permissions */
}


void CirclesDialog::friend_selected()
{
	/* update circle lists */


	/* update permission lists */
}

void CirclesDialog::category_selected()
{



}


#endif


#if 0


void CirclesDialog::OpenOrShowAddPageDialog()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "CirclesDialog::OpenOrShowAddPageDialog() No Group selected";
		std::cerr << std::endl;
		return;
	}

	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

	std::cerr << "CirclesDialog::OpenOrShowAddPageDialog() GroupId: " << groupId;
	std::cerr << std::endl;

	mEditDialog->setupData(groupId, "");
	mEditDialog->setNewPage();

	mEditDialog->show();
}



void CirclesDialog::OpenOrShowAddGroupDialog()
{
	newGroup();
}

/*********************** **** **** **** ***********************/
/** New / Edit Groups          ********************************/
/*********************** **** **** **** ***********************/

void CirclesDialog::newGroup()
{
        WikiGroupDialog cf(mWikiQueue, this);
        cf.wikitype();

        cf.exec ();
}

void CirclesDialog::showGroupDetails()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "CirclesDialog::showGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}
}

void CirclesDialog::editGroupDetails()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "CirclesDialog::editGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}


        //WikiGroupDialog cf (this);
        //cf.existingGroup(groupId,  GXS_GROUP_DIALOG_EDIT_MODE);

        //cf.exec ();
}



void CirclesDialog::OpenOrShowEditDialog()
{
	std::string groupId;
	std::string pageId;
	std::string origPageId;

	if (!getSelectedPage(groupId, pageId, origPageId))
	{
		std::cerr << "CirclesDialog::OpenOrShowAddPageDialog() No Group or PageId selected";
		std::cerr << std::endl;
		return;
	}

	std::cerr << "CirclesDialog::OpenOrShowAddPageDialog()";
	std::cerr << std::endl;

	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

	mEditDialog->setupData(groupId, pageId);
	mEditDialog->show();
}

void CirclesDialog::OpenOrShowRepublishDialog()
{
	OpenOrShowEditDialog();

	std::string groupId;
	std::string pageId;
	std::string origPageId;

	if (!getSelectedPage(groupId, pageId, origPageId))
	{
		std::cerr << "CirclesDialog::OpenOrShowAddRepublishDialog() No Group or PageId selected";
		std::cerr << std::endl;
		if (mEditDialog)
		{
			mEditDialog->hide();
		}
		return;
	}

	mEditDialog->setRepublishMode(origPageId);
}


void CirclesDialog::groupTreeChanged()
{
	/* */
	std::string groupId;
	std::string pageId;
	std::string origPageId;

	getSelectedPage(groupId, pageId, origPageId);
	if (pageId == mPageSelected)
	{
		return; /* nothing changed */
	}

	if (pageId == "")
	{
		/* clear Mods */
		clearGroupTree();
		return;
	}

        RsGxsGrpMsgIdPair origPagePair = std::make_pair(groupId, origPageId);
        RsGxsGrpMsgIdPair pagepair = std::make_pair(groupId, pageId);
	requestWikiPage(pagepair);
}

void CirclesDialog::updateWikiPage(const RsWikiSnapshot &page)
{
#ifdef USE_PEGMMD_RENDERER
	/* render as HTML */
	int extensions = 0;
	char *answer = markdown_to_string((char *) page.mPage.c_str(), extensions, HTML_FORMAT);

	QString renderedText = QString::fromUtf8(answer);
	ui.textBrowser->setHtml(renderedText);

	// free answer.
	free(answer);
#else
	/* render as HTML */
	QString renderedText = "IN (dummy) RENDERED TEXT MODE:\n";
	renderedText += QString::fromStdString(page.mPage);
	ui.textBrowser->setPlainText(renderedText);
#endif
}


void CirclesDialog::clearWikiPage()
{
	ui.textBrowser->setPlainText("");
}


void 	CirclesDialog::clearGroupTree()
{
	ui.treeWidget_Pages->clear();
}


#define WIKI_GROUP_COL_GROUPNAME	0
#define WIKI_GROUP_COL_GROUPID		1

#define WIKI_GROUP_COL_PAGENAME		0
#define WIKI_GROUP_COL_PAGEID		1
#define WIKI_GROUP_COL_ORIGPAGEID	2


bool CirclesDialog::getSelectedPage(std::string &groupId, std::string &pageId, std::string &origPageId)
{
#ifdef WIKI_DEBUG 
	std::cerr << "CirclesDialog::getSelectedPage()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "CirclesDialog::getSelectedPage() Nothing selected" << std::endl;
#endif
		return false;
	}

	QTreeWidgetItem *parent = item->parent();

	if (!parent)
	{
#ifdef WIKI_DEBUG 
		std::cerr << "CirclesDialog::getSelectedPage() No Parent -> Group Selected" << std::endl;
#endif
		return false;
	}


	/* check if it has changed */
	groupId = parent->text(WIKI_GROUP_COL_GROUPID).toStdString();
	pageId = item->text(WIKI_GROUP_COL_PAGEID).toStdString();
	origPageId = item->text(WIKI_GROUP_COL_ORIGPAGEID).toStdString();

#ifdef WIKI_DEBUG 
	std::cerr << "CirclesDialog::getSelectedPage() PageId: " << pageId << std::endl;
#endif
	return true;
}


std::string CirclesDialog::getSelectedGroup()
{
	std::string groupId;
#ifdef WIKI_DEBUG 
	std::cerr << "CirclesDialog::getSelectedGroup()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "CirclesDialog::getSelectedGroup() Nothing selected" << std::endl;
#endif
		return groupId;
	}

	QTreeWidgetItem *parent = item->parent();

	if (parent)
	{
		groupId = parent->text(WIKI_GROUP_COL_GROUPID).toStdString();
#ifdef WIKI_DEBUG 
		std::cerr << "CirclesDialog::getSelectedGroup() Page -> GroupId: " << groupId << std::endl;
#endif
		return groupId;
	}

	/* otherwise, we are on the group already */
	groupId = item->text(WIKI_GROUP_COL_GROUPID).toStdString();
#ifdef WIKI_DEBUG 
	std::cerr << "CirclesDialog::getSelectedGroup() GroupId: " << groupId << std::endl;
#endif
	return groupId;
}

/************************** Request / Response *************************/
/*** Loading Main Index ***/

void CirclesDialog::insertWikiGroups()
{
	requestGroupList();
}


void CirclesDialog::requestGroupList()
{
	std::cerr << "CirclesDialog::requestGroupList()";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	mWikiQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_DATA, opts, WIKIDIALOG_LISTING_GROUPDATA);
}


void CirclesDialog::loadGroupData(const uint32_t &token)
{
	std::cerr << "CirclesDialog::loadGroupData()";
	std::cerr << std::endl;

	clearGroupTree();

	std::vector<RsWikiCollection> datavector;
	std::vector<RsWikiCollection>::iterator vit;

        if (!rsWiki->getCollections(token, datavector))
        {
                std::cerr << "CirclesDialog::loadGroupData() Error getting GroupData";
                std::cerr << std::endl;
                return;
        }

        for(vit = datavector.begin(); vit != datavector.end(); vit++)
        {
		RsWikiCollection &group = *vit;

		/* Add Widget, and request Pages */

		std::cerr << "CirclesDialog::addGroup() GroupId: " << group.mMeta.mGroupId;
		std::cerr << " Group: " << group.mMeta.mGroupName;
		std::cerr << std::endl;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem();
		groupItem->setText(WIKI_GROUP_COL_GROUPNAME, QString::fromStdString(group.mMeta.mGroupName));
		groupItem->setText(WIKI_GROUP_COL_GROUPID, QString::fromStdString(group.mMeta.mGroupId));
		ui.treeWidget_Pages->addTopLevelItem(groupItem);

		/* request pages */
		std::list<std::string> groupIds;	
		groupIds.push_back(group.mMeta.mGroupId);

		requestPages(groupIds);
		//requestOriginalPages(groupIds);
        }
}


void CirclesDialog::requestPages(const std::list<RsGxsGroupId> &groupIds)
{
	std::cerr << "CirclesDialog::requestPages()";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
        opts.mOptions = (RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD); // We want latest version of Thread Heads.
	uint32_t token;
	mWikiQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, WIKIDIALOG_LISTING_PAGES);
}


void CirclesDialog::loadPages(const uint32_t &token)
{
	std::cerr << "CirclesDialog::loadPages()";
	std::cerr << std::endl;

	/* find parent in GUI Tree - stick children */

	QTreeWidgetItem *groupItem = NULL;

	std::vector<RsWikiSnapshot> snapshots;
	std::vector<RsWikiSnapshot>::iterator vit;
        if (!rsWiki->getSnapshots(token, snapshots))
	{
		// ERROR
		return;
	}

	for(vit = snapshots.begin(); vit != snapshots.end(); vit++)
	{
                RsWikiSnapshot page = *vit;
		if (!groupItem)
		{
			/* find the entry */
        		int itemCount = ui.treeWidget_Pages->topLevelItemCount();
        		for (int nIndex = 0; nIndex < itemCount; nIndex++) 
        		{
				QTreeWidgetItem *tmpItem = ui.treeWidget_Pages->topLevelItem(nIndex);
                		std::string tmpid = tmpItem->data(WIKI_GROUP_COL_GROUPID, 
						Qt::DisplayRole).toString().toStdString();
				if (tmpid == page.mMeta.mGroupId)
				{
                			groupItem = tmpItem;
					break;
				}
			}

			if (!groupItem)
			{
				/* error */
				std::cerr << "CirclesDialog::loadPages() ERROR Unable to find group";
				std::cerr << std::endl;
				return;
			}
		}
						

		std::cerr << "CirclesDialog::loadPages() PageId: " << page.mMeta.mMsgId;
		std::cerr << " Page: " << page.mMeta.mMsgName;
		std::cerr << std::endl;

		QTreeWidgetItem *pageItem = new QTreeWidgetItem();
		pageItem->setText(WIKI_GROUP_COL_PAGENAME, QString::fromStdString(page.mMeta.mMsgName));
		pageItem->setText(WIKI_GROUP_COL_PAGEID, QString::fromStdString(page.mMeta.mMsgId));
		pageItem->setText(WIKI_GROUP_COL_ORIGPAGEID, QString::fromStdString(page.mMeta.mOrigMsgId));

		groupItem->addChild(pageItem);
	}
}

/***** Wiki *****/

void CirclesDialog::requestWikiPage(const RsGxsGrpMsgIdPair &msgId)
{
	std::cerr << "CirclesDialog::requestWikiPage(" << msgId.first << "," << msgId.second << ")";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect_msgIds = msgIds[msgId.first];
	vect_msgIds.push_back(msgId.second);

	mWikiQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, WIKIDIALOG_WIKI_PAGE);

	//mWikiQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, WIKIDIALOG_WIKI_PAGE);
}


void CirclesDialog::loadWikiPage(const uint32_t &token)
{
	std::cerr << "CirclesDialog::loadWikiPage()";
	std::cerr << std::endl;

	// Should only have one WikiPage....
	std::vector<RsWikiSnapshot> snapshots;
        if (!rsWiki->getSnapshots(token, snapshots))
	{
		std::cerr << "CirclesDialog::loadWikiPage() ERROR";
		std::cerr << std::endl;

		// ERROR
		return;
	}

	if (snapshots.size() != 1) 
	{
		std::cerr << "CirclesDialog::loadWikiPage() SIZE ERROR";
		std::cerr << std::endl;

		// ERROR
		return;
	}


	RsWikiSnapshot page = snapshots[0];
	
	std::cerr << "CirclesDialog::loadWikiPage() PageId: " << page.mMeta.mMsgId;
	std::cerr << " Page: " << page.mMeta.mMsgName;
	std::cerr << std::endl;

	updateWikiPage(page);
}

#endif


void CirclesDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{

#if 0
	std::cerr << "CirclesDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	if (queue == mWikiQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case WIKIDIALOG_LISTING_GROUPDATA:
				loadGroupData(req.mToken);
				break;

			case WIKIDIALOG_LISTING_PAGES:
				loadPages(req.mToken);
				break;

			case WIKIDIALOG_WIKI_PAGE:
				loadWikiPage(req.mToken);
				break;

#define GXSGROUP_NEWGROUPID             1
			case GXSGROUP_NEWGROUPID:
				insertWikiGroups();
				break;
			default:
				std::cerr << "CirclesDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}

#endif

}
	
	
	
	


