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

#include <QFile>
#include <QFileInfo>

#include "WikiDialog.h"
#include "gui/WikiPoos/WikiAddDialog.h"
#include "gui/WikiPoos/WikiEditDialog.h"

#include "gui/gxs/WikiGroupDialog.h"

#include <retroshare/rswiki.h>

#include <iostream>
#include <sstream>

#include <QTimer>

/******
 * #define WIKI_DEBUG 1
 *****/

#define WIKI_DEBUG 1

#define WIKIDIALOG_LISTING_GROUPLIST		1
#define WIKIDIALOG_LISTING_GROUPDATA		2
#define WIKIDIALOG_LISTING_ORIGINALPAGES	3
#define WIKIDIALOG_LISTING_LATESTPAGES		4
#define WIKIDIALOG_LISTING_PAGES		5
#define WIKIDIALOG_MOD_LIST			6
#define WIKIDIALOG_MOD_PAGES			7
#define WIKIDIALOG_WIKI_PAGE			8


/** Constructor */
WikiDialog::WikiDialog(QWidget *parent)
: MainPage(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	mAddPageDialog = NULL;
	mAddGroupDialog = NULL;
	mEditDialog = NULL;

	connect( ui.toolButton_NewGroup, SIGNAL(clicked()), this, SLOT(OpenOrShowAddGroupDialog()));
	connect( ui.toolButton_NewPage, SIGNAL(clicked()), this, SLOT(OpenOrShowAddPageDialog()));
	connect( ui.toolButton_Edit, SIGNAL(clicked()), this, SLOT(OpenOrShowEditDialog()));

	connect( ui.treeWidget_Pages, SIGNAL(itemSelectionChanged()), this, SLOT(groupTreeChanged()));
	connect( ui.treeWidget_Mods, SIGNAL(itemSelectionChanged()), this, SLOT(modTreeChanged()));

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	/* setup TokenQueue */
#if 0
	mWikiQueue = new TokenQueue(rsWiki, this);
#endif

}

void WikiDialog::checkUpdate()
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


void WikiDialog::OpenOrShowAddPageDialog()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No Group selected";
		std::cerr << std::endl;
		return;
	}

	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

	std::cerr << "WikiDialog::OpenOrShowAddPageDialog() GroupId: " << groupId;
	std::cerr << std::endl;

#if 0
	RsWikiCollection group;
	if (!rsWiki->getGroup(groupId, group))
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() Failed to Get Group";
		std::cerr << std::endl;
	}

	mEditDialog->setGroup(group);
#endif

	mEditDialog->setupData(groupId, "");
	mEditDialog->setNewPage();

	mEditDialog->show();
}


void WikiDialog::OpenOrShowAddGroupDialog()
{
#if 0
	if (mAddGroupDialog)
	{
		mAddGroupDialog->show();
	}
	else
	{
		mAddGroupDialog = new WikiAddDialog(NULL);
		mAddGroupDialog->show();
	}
#endif

	newGroup();
}

/*********************** **** **** **** ***********************/
/** New / Edit Groups          ********************************/
/*********************** **** **** **** ***********************/

void WikiDialog::newGroup()
{
        WikiGroupDialog cf(mWikiQueue, this);
        //cf.newGroup();

        cf.exec ();
}

void WikiDialog::showGroupDetails()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::showGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}


	//RsWikiCollection collection;
        //WikiGroupDialog cf (collection, this);
        //cf.exec ();
}

void WikiDialog::editGroupDetails()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::editGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}


        //WikiGroupDialog cf (this);
        //cf.existingGroup(groupId,  GXS_GROUP_DIALOG_EDIT_MODE);

        //cf.exec ();
}



void WikiDialog::OpenOrShowEditDialog()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No Group selected";
		std::cerr << std::endl;
		return;
	}

	std::string modId = getSelectedMod();
	std::string realPageId;

	std::string pageId;
	std::string origPageId;

	if (!getSelectedPage(pageId, origPageId))
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No PageId selected";
		std::cerr << std::endl;
		return;
	}

	if (modId == "")
	{
		realPageId = pageId;
	}
	else
	{
		realPageId = modId;
	}


	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

#if 0
	RsWikiCollection group;
	rsWiki->getGroup(groupId, group);
	mEditDialog->setGroup(group);

	RsWikiSnapshot page;
	rsWiki->getPage(realPageId, page);
	mEditDialog->setPreviousPage(page);
#endif
	mEditDialog->setupData(groupId, realPageId);

	mEditDialog->show();
}



void WikiDialog::groupTreeChanged()
{
	/* */
	std::string pageId;
	std::string origPageId;

	getSelectedPage(pageId, origPageId);
	if (pageId == mPageSelected)
	{
		return; /* nothing changed */
	}

	if (pageId == "")
	{
		/* clear Mods */
		clearGroupTree();
		clearModsTree();

		return;
	}


	clearModsTree();

	insertModsForPage(origPageId);
	requestWikiPage(pageId);
}

void WikiDialog::modTreeChanged()
{
	/* */
	std::string pageId = getSelectedMod();
	if (pageId == mModSelected)
	{
		return; /* nothing changed */
	}

	if (pageId == "")
	{
		clearWikiPage();
		return;
	}

	requestWikiPage(pageId);
}


void WikiDialog::updateWikiPage(const RsWikiSnapshot &page)
{
	ui.textBrowser->setPlainText(QString::fromStdString(page.mPage));
}


void WikiDialog::clearWikiPage()
{
	ui.textBrowser->setPlainText("");
}


void 	WikiDialog::clearGroupTree()
{
	ui.treeWidget_Pages->clear();
}

void 	WikiDialog::clearModsTree()
{
	ui.treeWidget_Mods->clear();
}



#define WIKI_GROUP_COL_GROUPNAME	0
#define WIKI_GROUP_COL_GROUPID		1

#define WIKI_GROUP_COL_PAGENAME		0
#define WIKI_GROUP_COL_PAGEID		1
#define WIKI_GROUP_COL_ORIGPAGEID	2


// THIS WAS ALREADY COMMENTED OUT!!!
#if 0
#########################################################
void WikiDialog::insertWikiGroups()
{

	std::cerr << "WikiDialog::insertWikiGroups()";
	std::cerr << std::endl;

	/* clear it all */
	clearGroupTree();

	std::list<std::string> groupIds;
	std::list<std::string>::iterator it;

	rsWiki->getGroupList(groupIds);

	for(it = groupIds.begin(); it != groupIds.end(); it++)
	{
		/* add a group Item */
		RsWikiCollection group;
		rsWiki->getGroup(*it, group);

		QTreeWidgetItem *groupItem = new QTreeWidgetItem(NULL);
		groupItem->setText(WIKI_GROUP_COL_GROUPNAME, QString::fromStdString(group.mName));
		groupItem->setText(WIKI_GROUP_COL_GROUPID, QString::fromStdString(group.mGroupId));
		ui.treeWidget_Pages->addTopLevelItem(groupItem);

		std::cerr << "Group: " << group.mName;
		std::cerr << std::endl;

		std::list<std::string> pageIds;
		std::list<std::string>::iterator pit;

		rsWiki->getOrigPageList(*it, pageIds);

		for(pit = pageIds.begin(); pit != pageIds.end(); pit++)
		{
			std::cerr << "\tOrigPageId: " << *pit;
			std::cerr << std::endl;

			/* get newest page */
			RsWikiSnapshot page;
			std::string latestPageId;
			if (!rsWiki->getLatestPage(*pit, latestPageId))
			{
				std::cerr << "\tgetLatestPage() Failed";
				std::cerr << std::endl;
			}

			if (!rsWiki->getPage(latestPageId, page))
			{
				std::cerr << "\tgetPage() Failed";
				std::cerr << std::endl;
			}

			std::cerr << "\tLatestPageId: " << latestPageId;
			std::cerr << std::endl;
			std::cerr << "\tExtracted OrigPageId: " << page.mOrigPageId;
			std::cerr << std::endl;
			std::cerr << "\tExtracted PageId: " << page.mPageId;
			std::cerr << std::endl;

			QTreeWidgetItem *pageItem = new QTreeWidgetItem(NULL);
			pageItem->setText(WIKI_GROUP_COL_PAGENAME, QString::fromStdString(page.mName));
			pageItem->setText(WIKI_GROUP_COL_PAGEID, QString::fromStdString(page.mPageId));
			pageItem->setText(WIKI_GROUP_COL_ORIGPAGEID, QString::fromStdString(page.mOrigPageId));

			groupItem->addChild(pageItem);

			std::cerr << "\tPage: " << page.mName;
			std::cerr << std::endl;
		}
	}
}
#########################################################
#endif


bool WikiDialog::getSelectedPage(std::string &pageId, std::string &origPageId)
{
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedPage()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedPage() Nothing selected" << std::endl;
#endif
		return false;
	}

	QTreeWidgetItem *parent = item->parent();

	if (!parent)
	{
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedPage() No Parent -> Group Selected" << std::endl;
#endif
		return false;
	}


	/* check if it has changed */
	pageId = item->text(WIKI_GROUP_COL_PAGEID).toStdString();
	origPageId = item->text(WIKI_GROUP_COL_ORIGPAGEID).toStdString();

#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedPage() PageId: " << pageId << std::endl;
#endif
	return true;
}


std::string WikiDialog::getSelectedGroup()
{
	std::string groupId;
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedGroup()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedGroup() Nothing selected" << std::endl;
#endif
		return groupId;
	}

	QTreeWidgetItem *parent = item->parent();

	if (parent)
	{
		groupId = parent->text(WIKI_GROUP_COL_GROUPID).toStdString();
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedGroup() Page -> GroupId: " << groupId << std::endl;
#endif
		return groupId;
	}

	/* otherwise, we are on the group already */
	groupId = item->text(WIKI_GROUP_COL_GROUPID).toStdString();
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedGroup() GroupId: " << groupId << std::endl;
#endif
	return groupId;
}

#define WIKI_MODS_COL_ORIGPAGEID	0
#define WIKI_MODS_COL_PAGEID		1

#if 0
#########################################################
void WikiDialog::insertModsForPage(std::string &origPageId)
{
	/* clear it all */
	//clearPhotos();
	//ui.photoLayout->clear();

	/* create a list of albums */

	std::list<std::string> pageIds;
	std::list<std::string>::const_iterator it;

	rsWiki->getPageVersions(origPageId, pageIds);


	for(it = pageIds.begin(); it != pageIds.end(); it++)
	{
		RsWikiSnapshot page;
		rsWiki->getPage(*it, page);

		QTreeWidgetItem *modItem = new QTreeWidgetItem(NULL);
		modItem->setText(WIKI_MODS_COL_ORIGPAGEID, QString::fromStdString(page.mOrigPageId));
		modItem->setText(WIKI_MODS_COL_PAGEID, QString::fromStdString(page.mPageId));
		ui.treeWidget_Mods->addTopLevelItem(modItem);
	}
}
#########################################################
#endif



std::string WikiDialog::getSelectedMod()
{
	std::string pageId;
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedMod()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Mods->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedMod() Nothing selected" << std::endl;
#endif
		return pageId;
	}

	pageId = item->text(WIKI_MODS_COL_PAGEID).toStdString();
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedMod() PageId: " << pageId << std::endl;
#endif
	return pageId;
}

/************************** Request / Response *************************/
/*** Loading Main Index ***/

void WikiDialog::insertWikiGroups()
{
	requestGroupList();
}


void WikiDialog::requestGroupList()
{
	std::cerr << "WikiDialog::requestGroupList()";
	std::cerr << std::endl;

	std::list<std::string> ids;
	RsTokReqOptions opts;
	uint32_t token;
	mWikiQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_LIST, opts, ids, WIKIDIALOG_LISTING_GROUPLIST);
}

void WikiDialog::loadGroupList(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadGroupList()";
	std::cerr << std::endl;
	
	std::list<std::string> groupIds;
	rsWiki->getGroupList(token, groupIds);

	if (groupIds.size() > 0)
	{	
		requestGroupData(groupIds);
	}
	else
	{
		std::cerr << "WikiDialog::loadGroupList() ERROR No Groups...";
		std::cerr << std::endl;
	}
}

void WikiDialog::requestGroupData(const std::list<std::string> &groupIds)
{
	RsTokReqOptions opts;
	uint32_t token;
#if 0
	mWikiQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, WIKIDIALOG_LISTING_GROUPDATA);
#endif
}

void WikiDialog::loadGroupData(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadGroupData()";
	std::cerr << std::endl;

	clearGroupTree();

        bool moreData = true;
        while(moreData)
        {
		RsWikiCollection group;
#if 0
                if (rsWiki->getGroupData(token, group))
#else
		if (0)
#endif
                {
			/* Add Widget, and request Pages */

                        std::cerr << "WikiDialog::addGroup() GroupId: " << group.mMeta.mGroupId;
			std::cerr << " Group: " << group.mMeta.mGroupName;
			std::cerr << std::endl;

			QTreeWidgetItem *groupItem = new QTreeWidgetItem();
			groupItem->setText(WIKI_GROUP_COL_GROUPNAME, QString::fromStdString(group.mMeta.mGroupName));
			groupItem->setText(WIKI_GROUP_COL_GROUPID, QString::fromStdString(group.mMeta.mGroupId));
			ui.treeWidget_Pages->addTopLevelItem(groupItem);

			/* request pages */
			std::list<std::string> groupIds;	
			groupIds.push_back(group.mMeta.mGroupId);

			requestOriginalPages(groupIds);
                }
                else
                {
                        moreData = false;
                }
        }
        //return true;
}

void WikiDialog::requestOriginalPages(const std::list<std::string> &groupIds)
{
	RsTokReqOptions opts;
	opts.mOptions = RS_TOKREQOPT_MSG_ORIGMSG;
	uint32_t token;
	mWikiQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, groupIds, WIKIDIALOG_LISTING_ORIGINALPAGES);
}

void WikiDialog::loadOriginalPages(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadOriginalPages()";
	std::cerr << std::endl;

	/* translate into latest pages */
	std::list<std::string> msgIds;
#if 0
        if (rsWiki->getMsgList(token, msgIds))
#else
	if (0)
#endif
	{
		std::cerr << "WikiDialog::loadOriginalPages() Loaded " << msgIds.size();
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "WikiDialog::loadOriginalPages() ERROR No Data";
		std::cerr << std::endl;
		return;
	}

	requestLatestPages(msgIds);

}

void WikiDialog::requestLatestPages(const std::list<std::string> &msgIds)
{
	RsTokReqOptions opts;
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
	uint32_t token;
#if 0
	mWikiQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, msgIds, WIKIDIALOG_LISTING_LATESTPAGES);
#endif

}

void convert_result_to_list(const GxsMsgIdResult &result, std::list<RsGxsMessageId> &list)
{
	GxsMsgIdResult::const_iterator it;
	for(it = result.begin(); it != result.end(); it++)
	{
		std::vector<RsGxsMessageId>::const_iterator vit;
		for (vit = it->second.begin(); vit != it->second.end(); vit++)
		{
			list.push_back(*vit);
		}
	}
}

void WikiDialog::loadLatestPages(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadLatestPages()";
	std::cerr << std::endl;

	//std::list<std::string> msgIds;
	GxsMsgIdResult msgIds;
        if (rsWiki->getMsgList(token, msgIds))
	{
		std::cerr << "WikiDialog::loadLatestPages() Loaded " << msgIds.size();
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "WikiDialog::loadLatestPages() ERROR No Data";
		std::cerr << std::endl;
		return;
	}

	std::list<RsGxsMessageId> list;
	convert_result_to_list(msgIds, list);

	/* request actual data */
	//requestPages(list);

}

#if 0
void WikiDialog::requestPages(std::list<RsGxsMessageId> &msgids)
{
	RsTokReqOptions opts;
	uint32_t token;
	mWikiQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, WIKIDIALOG_LISTING_PAGES);
}
#endif

void WikiDialog::loadPages(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadLatestPages()";
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
        		for (int nIndex = 0; nIndex < itemCount;) 
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
				std::cerr << "WikiDialog::loadPages() ERROR Unable to find group";
				std::cerr << std::endl;
				return;
			}
		}
						

		std::cerr << "WikiDialog::loadPages() PageId: " << page.mMeta.mMsgId;
		std::cerr << " Page: " << page.mMeta.mMsgName;
		std::cerr << std::endl;

		QTreeWidgetItem *pageItem = new QTreeWidgetItem();
		pageItem->setText(WIKI_GROUP_COL_PAGENAME, QString::fromStdString(page.mMeta.mMsgName));
		pageItem->setText(WIKI_GROUP_COL_PAGEID, QString::fromStdString(page.mMeta.mMsgId));
		pageItem->setText(WIKI_GROUP_COL_ORIGPAGEID, QString::fromStdString(page.mMeta.mOrigMsgId));

		groupItem->addChild(pageItem);
	}
        //return true;
}


/***** Mods *****/

void WikiDialog::insertModsForPage(const std::string &origPageId)
{
	requestModPageList(origPageId);
}

void WikiDialog::requestModPageList(const std::string &origMsgId)
{
	RsTokReqOptions opts;
	opts.mOptions = RS_TOKREQOPT_MSG_VERSIONS;

	std::list<std::string> msgIds;
	msgIds.push_back(origMsgId);

	uint32_t token;
#if 0
	mWikiQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, msgIds, WIKIDIALOG_MOD_LIST);
#endif
}


void WikiDialog::loadModPageList(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadModPages()";
	std::cerr << std::endl;

	/* translate into latest pages */
	//std::list<std::string> msgIds;
	GxsMsgIdResult msgIds;

        if (rsWiki->getMsgList(token, msgIds))
	{
		std::cerr << "WikiDialog::loadModPageList() Loaded " << msgIds.size();
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "WikiDialog::loadModPageList() ERROR No Data";
		std::cerr << std::endl;
		return;
	}

	std::list<RsGxsMessageId> list;
	convert_result_to_list(msgIds, list);
	requestModPages(list);

}

void WikiDialog::requestModPages(const std::list<RsGxsMessageId> &msgIds)
{
	RsTokReqOptions opts;

	uint32_t token;
#if 0
	mWikiQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, WIKIDIALOG_MOD_PAGES);
#endif
}


void WikiDialog::loadModPages(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadModPages()";
	std::cerr << std::endl;

        bool moreData = true;
        while(moreData)
        {
                RsWikiSnapshot page;
#if 0
                if (rsWiki->getMsgData(token, page))
#endif
		if (0)
                {
                        std::cerr << "WikiDialog::loadModPages() PageId: " << page.mMeta.mMsgId;
			std::cerr << " Page: " << page.mMeta.mMsgName;
			std::cerr << std::endl;

			QTreeWidgetItem *modItem = new QTreeWidgetItem();
			modItem->setText(WIKI_MODS_COL_ORIGPAGEID, QString::fromStdString(page.mMeta.mOrigMsgId));
			modItem->setText(WIKI_MODS_COL_PAGEID, QString::fromStdString(page.mMeta.mMsgId));
			ui.treeWidget_Mods->addTopLevelItem(modItem);

                }
                else
                {
                        moreData = false;
                }
        }
}



/***** Wiki *****/


void WikiDialog::requestWikiPage(const std::string &msgId)
{
	RsTokReqOptions opts;

	std::list<std::string> msgIds;
	msgIds.push_back(msgId);

	uint32_t token;
#if 0
	mWikiQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, WIKIDIALOG_WIKI_PAGE);
#endif
}


void WikiDialog::loadWikiPage(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadModPages()";
	std::cerr << std::endl;

	// Should only have one WikiPage....
	std::vector<RsWikiSnapshot> snapshots;
        if (!rsWiki->getSnapshots(token, snapshots))
	{
		// ERROR
		return;
	}

	if (snapshots.size() < 1) 
	{
		// ERROR
		return;
	}


	if (snapshots.size() > 1) 
	{
		// ERROR
		return;
	}

	RsWikiSnapshot page = snapshots[0];
	
	std::cerr << "WikiDialog::loadModPages() PageId: " << page.mMeta.mMsgId;
	std::cerr << " Page: " << page.mMeta.mMsgName;
	std::cerr << std::endl;

	updateWikiPage(page);
}



void WikiDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "WikiDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	if (queue == mWikiQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case WIKIDIALOG_LISTING_GROUPLIST:
				loadGroupList(req.mToken);
				break;

			case WIKIDIALOG_LISTING_GROUPDATA:
				loadGroupData(req.mToken);
				break;

			case WIKIDIALOG_LISTING_ORIGINALPAGES:
				loadOriginalPages(req.mToken);
				break;

			case WIKIDIALOG_LISTING_LATESTPAGES:
				loadLatestPages(req.mToken);
				break;

			case WIKIDIALOG_LISTING_PAGES:
				loadPages(req.mToken);
				break;

			case WIKIDIALOG_MOD_LIST:
				loadModPageList(req.mToken);
				break;

			case WIKIDIALOG_MOD_PAGES:
				loadModPages(req.mToken);
				break;

			case WIKIDIALOG_WIKI_PAGE:
				loadWikiPage(req.mToken);
				break;

			default:
				std::cerr << "PhotoDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}
	
	
	
	


