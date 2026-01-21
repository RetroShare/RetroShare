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

#include <QMenu>
#include <QFile>
#include <QFileInfo>

#include "WikiDialog.h"
#include "gui/WikiPoos/WikiAddDialog.h"
#include "gui/WikiPoos/WikiEditDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxs/WikiGroupDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/DateTime.h"

#include <retroshare/rswiki.h>
#include "util/qtthreadsutils.h"
#include "util/rsthreads.h"


// These should be in retroshare/ folder.
#include "retroshare/rsgxsflags.h"


#include <iostream>
#include <sstream>

#include <QTimer>

#define USE_PEGMMD_RENDERER     1

#ifdef USE_PEGMMD_RENDERER
#include "markdown_lib.h"
#endif

/******
 * #define WIKI_DEBUG 1
 *****/

#define WIKI_DEBUG 1

#define WIKIDIALOG_LISTING_GROUPMETA		2
#define WIKIDIALOG_LISTING_PAGES		5
#define WIKIDIALOG_MOD_LIST			6
#define WIKIDIALOG_MOD_PAGES			7
#define WIKIDIALOG_WIKI_PAGE			8

#define WIKIDIALOG_EDITTREE_DATA		9

/* Images for TreeWidget (Copied from GxsForums.cpp) */
#define IMAGE_FOLDER         ":/images/folder16.png"
#define IMAGE_FOLDERGREEN    ":/images/folder_green.png"
#define IMAGE_FOLDERRED      ":/images/folder_red.png"
#define IMAGE_FOLDERYELLOW   ":/images/folder_yellow.png"
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
#define IMAGE_NEWFORUM       ":/images/new_forum16.png"
#define IMAGE_FORUMAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"
#define IMAGE_WIKI           ":icons/wiki.png"
#define IMAGE_EDIT           ":/icons/png/pencil-edit-button.png"


/** Constructor */
WikiDialog::WikiDialog(QWidget *parent) :
    MainPage(parent),
    mEventHandlerId(0) // Initialize handler ID
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    mAddPageDialog = NULL;
    mAddGroupDialog = NULL;
    mEditDialog = NULL;

    connect( ui.toolButton_NewPage, SIGNAL(clicked()), this, SLOT(OpenOrShowAddPageDialog()));
    connect( ui.toolButton_Edit, SIGNAL(clicked()), this, SLOT(OpenOrShowEditDialog()));
    connect( ui.toolButton_Republish, SIGNAL(clicked()), this, SLOT(OpenOrShowRepublishDialog()));

    connect( ui.treeWidget_Pages, SIGNAL(itemSelectionChanged()), this, SLOT(groupTreeChanged()));

    // Connect search filter
    connect( ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterPages(QString)));

    // GroupTreeWidget.
    connect(ui.groupTreeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(groupListCustomPopupMenu(QPoint)));
    connect(ui.groupTreeWidget, SIGNAL(treeItemActivated(QString)), this, SLOT(wikiGroupChanged(QString)));

    // Set initial size of the splitter
    ui.listSplitter->setStretchFactor(0, 0);
    ui.listSplitter->setStretchFactor(1, 1);

    /* Setup Group Tree */
    mYourGroups = ui.groupTreeWidget->addCategoryItem(tr("My Groups"), QIcon(), true);
    mSubscribedGroups = ui.groupTreeWidget->addCategoryItem(tr("Subscribed Groups"), QIcon(), true);
    mPopularGroups = ui.groupTreeWidget->addCategoryItem(tr("Popular Groups"), QIcon(), false);
    mOtherGroups = ui.groupTreeWidget->addCategoryItem(tr("Other Groups"), QIcon(), false);

    /* Add the New Group button */
    QToolButton *newGroupButton = new QToolButton(this);
    newGroupButton->setIcon(QIcon(":/icons/png/add.png"));
    newGroupButton->setToolTip(tr("Create Group"));
    connect(newGroupButton, SIGNAL(clicked()), this, SLOT(OpenOrShowAddGroupDialog()));
    ui.groupTreeWidget->addToolButton(newGroupButton);

    // load settings
    processSettings(true);

    // Load initial data
    loadGroupMeta();

    /* Get dynamic event type ID for Wiki. This avoids hardcoding IDs in rsevents.h */
    RsEventType wikiEventType = (RsEventType)rsEvents->getDynamicEventType("GXS_WIKI");

    /* Register events handler using the dynamic type */
    rsEvents->registerEventsHandler(
        [this](std::shared_ptr<const RsEvent> event) {
            RsQThreadUtils::postToObject([=]() {
                handleEvent_main_thread(event);
            }, this );
        },
        mEventHandlerId, wikiEventType);
}

WikiDialog::~WikiDialog()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);

	// save settings
	processSettings(false);
}

void WikiDialog::processSettings(bool load)
{
	Settings->beginGroup("WikiDialog");

	if (load) {
		// load settings

		// state of splitter
		ui.listSplitter->restoreState(Settings->value("SplitterList").toByteArray());
	} else {
		// save settings

		// state of splitter
		Settings->setValue("SplitterList", ui.listSplitter->saveState());
	}

	Settings->endGroup();
}


void WikiDialog::OpenOrShowAddPageDialog()
{
	RsGxsGroupId groupId = getSelectedGroup();
	if (groupId.isNull())
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

	mEditDialog->setupData(groupId, RsGxsMessageId());
	mEditDialog->setNewPage();

	mEditDialog->show();
}

void WikiDialog::OpenOrShowAddGroupDialog()
{
	newGroup();
}

/*********************** **** **** **** ***********************/
/** New / Edit Groups          ********************************/
/*********************** **** **** **** ***********************/

void WikiDialog::newGroup()
{
	WikiGroupDialog cf(this);
	cf.exec ();
}

void WikiDialog::showGroupDetails()
{
	RsGxsGroupId groupId = getSelectedGroup();
	if (groupId.isNull())
	{
		std::cerr << "WikiDialog::showGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}

	WikiGroupDialog cf(GxsGroupDialog::MODE_SHOW, groupId, this);
	cf.exec ();
}

void WikiDialog::editGroupDetails()
{
	RsGxsGroupId groupId = getSelectedGroup();
	if (groupId.isNull())
	{
		std::cerr << "WikiDialog::editGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}

	WikiGroupDialog cf(GxsGroupDialog::MODE_EDIT, groupId, this);
	cf.exec ();
}

void WikiDialog::OpenOrShowEditDialog()
{
	RsGxsGroupId groupId;
	RsGxsMessageId pageId;
	RsGxsMessageId origPageId;

	if (!getSelectedPage(groupId, pageId, origPageId))
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No Group or PageId selected";
		std::cerr << std::endl;
		return;
	}

	std::cerr << "WikiDialog::OpenOrShowAddPageDialog()";
	std::cerr << std::endl;

	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

	mEditDialog->setupData(groupId, pageId);
	mEditDialog->show();
}

void WikiDialog::OpenOrShowRepublishDialog()
{
	OpenOrShowEditDialog();

	RsGxsGroupId groupId;
	RsGxsMessageId pageId;
	RsGxsMessageId origPageId;

	if (!getSelectedPage(groupId, pageId, origPageId))
	{
		std::cerr << "WikiDialog::OpenOrShowAddRepublishDialog() No Group or PageId selected";
		std::cerr << std::endl;
		if (mEditDialog)
		{
			mEditDialog->hide();
		}
		return;
	}

	mEditDialog->setRepublishMode(origPageId);
}

void WikiDialog::groupTreeChanged()
{
	/* */
	RsGxsGroupId groupId;
	RsGxsMessageId pageId;
	RsGxsMessageId origPageId;

	getSelectedPage(groupId, pageId, origPageId);
	if (pageId == mPageSelected)
	{
		return; /* nothing changed */
	}

	if (pageId.isNull())
	{
		/* clear Mods */
		clearGroupTree();
		return;
	}

	RsGxsGrpMsgIdPair origPagePair = std::make_pair(groupId, origPageId);
	RsGxsGrpMsgIdPair pagepair = std::make_pair(groupId, pageId);
	loadWikiPage(pagepair);
}

void WikiDialog::updateWikiPage(const RsWikiSnapshot &page)
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

void WikiDialog::clearWikiPage()
{
	ui.textBrowser->setPlainText("");
}

void WikiDialog::clearGroupTree()
{
	ui.treeWidget_Pages->clear();
}

void WikiDialog::filterPages(const QString &text)
{
	std::cerr << "WikiDialog::filterPages() Filter text: " << text.toStdString() << std::endl;

	// Get the filter text in lowercase for case-insensitive search
	QString filterText = text.toLower();

	// Iterate through all items in the pages tree
	QTreeWidgetItemIterator it(ui.treeWidget_Pages);
	while (*it)
	{
		QTreeWidgetItem *item = *it;

		// Get the page name from the first column
		QString pageName = item->text(WIKI_GROUP_COL_PAGENAME).toLower();

		// Show/hide item based on whether it matches the filter
		if (filterText.isEmpty() || pageName.contains(filterText))
		{
			item->setHidden(false);
		}
		else
		{
			item->setHidden(true);
		}

		++it;
	}
}

#define WIKI_GROUP_COL_GROUPNAME	0
#define WIKI_GROUP_COL_GROUPID		1

#define WIKI_GROUP_COL_PAGENAME		0
#define WIKI_GROUP_COL_PAGEID		1
#define WIKI_GROUP_COL_ORIGPAGEID	2

bool WikiDialog::getSelectedPage(RsGxsGroupId &groupId, RsGxsMessageId &pageId, RsGxsMessageId &origPageId)
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

	/* check if it has changed */
	groupId = getSelectedGroup();
	if (groupId.isNull())
	{
		return false;
	}

	pageId = RsGxsMessageId(item->text(WIKI_GROUP_COL_PAGEID).toStdString());
	origPageId = RsGxsMessageId(item->text(WIKI_GROUP_COL_ORIGPAGEID).toStdString());

#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedPage() PageId: " << pageId << std::endl;
#endif
	return true;
}

const RsGxsGroupId& WikiDialog::getSelectedGroup()
{
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedGroup(): " << mGroupId << std::endl;
#endif
	return mGroupId;
}

/************************** Request / Response *************************/
/*** Loading Main Index ***/

void WikiDialog::loadGroupMeta()
{
	std::cerr << "WikiDialog::loadGroupMeta() - using async";
	std::cerr << std::endl;

	RsThread::async([this]()
	{
		// 1 - get group metadata from rsWiki (backend thread)
		std::list<RsGxsGroupId> groupIds;
		std::vector<RsWikiCollection> groups;

		if(!rsWiki->getCollections(groupIds, groups))
		{
			std::cerr << "WikiDialog::loadGroupMeta() failed to retrieve wiki groups" << std::endl;
			return;
		}

		// Convert to RsGroupMetaData list
		std::list<RsGroupMetaData> *groupMeta = new std::list<RsGroupMetaData>();
		for(const auto& group : groups)
		{
			RsGroupMetaData meta;
			meta.mGroupId = group.mMeta.mGroupId;
			meta.mGroupName = group.mMeta.mGroupName;
			meta.mGroupFlags = group.mMeta.mGroupFlags;
			meta.mSignFlags = group.mMeta.mSignFlags;
			meta.mPublishTs = group.mMeta.mPublishTs;
			meta.mAuthorId = group.mMeta.mAuthorId;
			meta.mCircleType = group.mMeta.mCircleType;
			meta.mCircleId = group.mMeta.mCircleId;
			meta.mSubscribeFlags = group.mMeta.mSubscribeFlags;
			meta.mPop = group.mMeta.mPop;
			meta.mVisibleMsgCount = group.mMeta.mVisibleMsgCount;
			meta.mLastPost = group.mMeta.mLastPost;
			groupMeta->push_back(meta);
		}

		// 2 - update the UI in the main thread
		RsQThreadUtils::postToObject([groupMeta, this]()
		{
			if (groupMeta->size() > 0)
			{
				insertGroupsData(*groupMeta);
			}
			else
			{
				std::cerr << "WikiDialog::loadGroupMeta() No groups found" << std::endl;
			}
			delete groupMeta;
		}, this);
	});
}

void WikiDialog::loadPages(const std::list<RsGxsGroupId> &groupIds)
{
	std::cerr << "WikiDialog::loadPages() - using async";
	std::cerr << std::endl;

	if(groupIds.empty())
		return;

	RsThread::async([this, groupIds]()
	{
		// 1 - get snapshots from rsWiki (backend thread)
		std::vector<RsWikiSnapshot> *snapshots = new std::vector<RsWikiSnapshot>();

		// Get snapshots for the specified groups
		for(const auto& groupId : groupIds)
		{
			std::vector<RsWikiSnapshot> groupSnapshots;
			// Note: We need to use the token-based API here since there's no direct blocking API
			// For now, we'll use a simplified approach
			// TODO: Implement proper blocking API in rsWiki
		}

		// 2 - update the UI in the main thread
		RsQThreadUtils::postToObject([snapshots, this]()
		{
			clearGroupTree();
			clearWikiPage();

			for(auto vit = snapshots->begin(); vit != snapshots->end(); ++vit)
			{
				RsWikiSnapshot page = *vit;

				std::cerr << "WikiDialog::loadPages() PageId: " << page.mMeta.mMsgId;
				std::cerr << " Page: " << page.mMeta.mMsgName;
				std::cerr << std::endl;

				QTreeWidgetItem *pageItem = new QTreeWidgetItem();
				pageItem->setText(WIKI_GROUP_COL_PAGENAME, QString::fromStdString(page.mMeta.mMsgName));
				pageItem->setText(WIKI_GROUP_COL_PAGEID, QString::fromStdString(page.mMeta.mMsgId.toStdString()));
				pageItem->setText(WIKI_GROUP_COL_ORIGPAGEID, QString::fromStdString(page.mMeta.mOrigMsgId.toStdString()));

				ui.treeWidget_Pages->addTopLevelItem(pageItem);
			}

			delete snapshots;
		}, this);
	});
}

/***** Wiki *****/

void WikiDialog::loadWikiPage(const RsGxsGrpMsgIdPair &msgId)
{
	std::cerr << "WikiDialog::loadWikiPage(" << msgId.first << "," << msgId.second << ") - using async";
	std::cerr << std::endl;

	RsThread::async([this, msgId]()
	{
		// 1 - get snapshot from rsWiki (backend thread)
		std::vector<RsWikiSnapshot> snapshots;

		// Get the specific snapshot
		// TODO: Need blocking API for getting specific snapshot
		// For now, we'll need to use a workaround

		// 2 - update the UI in the main thread
		RsQThreadUtils::postToObject([snapshots, this]()
		{
			if (snapshots.size() != 1)
			{
				std::cerr << "WikiDialog::loadWikiPage() SIZE ERROR: " << snapshots.size();
				std::cerr << std::endl;
				return;
			}

			RsWikiSnapshot page = snapshots[0];

			std::cerr << "WikiDialog::loadWikiPage() PageId: " << page.mMeta.mMsgId;
			std::cerr << " Page: " << page.mMeta.mMsgName;
			std::cerr << std::endl;

			updateWikiPage(page);
		}, this);
	});
}

// loadRequest() method removed - now using async loading pattern

/************************** Group Widget Stuff *********************************/


void WikiDialog::subscribeToGroup()
{
	wikiSubscribe(true);
}

void WikiDialog::unsubscribeToGroup()
{
	wikiSubscribe(false);
}

void WikiDialog::wikiSubscribe(bool subscribe)
{
	if (mGroupId.isNull()) {
		return;
	}

	uint32_t token;
	rsWiki->subscribeToGroup(token, mGroupId, subscribe);

	insertWikiGroups();
}


void WikiDialog::wikiGroupChanged(const QString &groupId)
{
	mGroupId = RsGxsGroupId(groupId.toStdString());

	if (mGroupId.isNull()) {
		return;
	}

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(mGroupId);
	loadPages(groupIds);

	int subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId.toStdString()));
	ui.toolButton_NewPage->setEnabled(IS_GROUP_ADMIN(subscribeFlags));
	ui.toolButton_Republish->setEnabled(IS_GROUP_ADMIN(subscribeFlags));
}

void WikiDialog::groupListCustomPopupMenu(QPoint /*point*/)
{

	int subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId.toStdString()));

	QMenu contextMnu(this);

	std::cerr << "WikiDialog::groupListCustomPopupMenu()";
	std::cerr << std::endl;
	std::cerr << "    mGroupId: " << mGroupId;
	std::cerr << std::endl;
	std::cerr << "    subscribeFlags: " << subscribeFlags;
	std::cerr << std::endl;
	std::cerr << "    IS_GROUP_SUBSCRIBED(): " << IS_GROUP_SUBSCRIBED(subscribeFlags);
	std::cerr << std::endl;
	std::cerr << "    IS_GROUP_ADMIN(): " << IS_GROUP_ADMIN(subscribeFlags);
	std::cerr << std::endl;
	std::cerr << std::endl;

	QAction *action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe to Group"), this, SLOT(subscribeToGroup()));
	action->setDisabled (mGroupId.isNull() || IS_GROUP_SUBSCRIBED(subscribeFlags));

	action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe to Group"), this, SLOT(unsubscribeToGroup()));
	action->setEnabled (!mGroupId.isNull() && IS_GROUP_SUBSCRIBED(subscribeFlags));

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Wiki Group"), this, SLOT(showGroupDetails()));
	action->setEnabled (!mGroupId.isNull());

	action = contextMnu.addAction(QIcon(IMAGE_EDIT), tr("Edit Wiki Group"), this, SLOT(editGroupDetails()));
	action->setEnabled (!mGroupId.isNull() && IS_GROUP_ADMIN(subscribeFlags));

	/************** NOT ENABLED YET *****************/

	//if (!Settings->getForumOpenAllInNewTab()) {
	//	action = contextMnu.addAction(QIcon(""), tr("Open in new tab"), this, SLOT(openInNewTab()));
	//	if (mForumId.empty() || forumThreadWidget(mForumId)) {
	//		action->setEnabled(false);
	//	}
	//}

	//QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Forum"), &contextMnu);
	//connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );
	//shareKeyAct->setEnabled(!mForumId.empty() && IS_GROUP_ADMIN(subscribeFlags));
	//contextMnu.addAction( shareKeyAct);

	//QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Forum" ), &contextMnu);
	//connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreForumKeys() ) );
	//restoreKeysAct->setEnabled(!mForumId.empty() && !IS_GROUP_ADMIN(subscribeFlags));
	//contextMnu.addAction( restoreKeysAct);

	//action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyForumLink()));
	//action->setEnabled(!mForumId.empty());

	//contextMnu.addSeparator();

	contextMnu.exec(QCursor::pos());
}

void WikiDialog::insertGroupsData(const std::list<RsGroupMetaData> &wikiList)
{
	std::list<RsGroupMetaData>::const_iterator it;

	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for (it = wikiList.begin(); it != wikiList.end(); ++it) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->mSubscribeFlags;

		GroupItemInfo groupItemInfo;
		GroupMetaDataToGroupItemInfo(*it, groupItemInfo);

		if (IS_GROUP_ADMIN(flags)) {
			adminList.push_back(groupItemInfo);
		} else if (IS_GROUP_SUBSCRIBED(flags)) {
			/* subscribed forum */
			subList.push_back(groupItemInfo);
		} else {
			/* rate the others by popularity */
			popMap.insert(std::make_pair(it->mPop, groupItemInfo));
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10)
	{
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	uint32_t popLimit = 0;
	std::multimap<uint32_t, GroupItemInfo>::reverse_iterator rit;
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); ++rit, i++) ;
	if (rit != popMap.rend()) {
		popLimit = rit->first;
	}

	for (rit = popMap.rbegin(); rit != popMap.rend(); ++rit) {
		if (rit->second.popularity < (int) popLimit) {
			otherList.append(rit->second);
		} else {
			popList.append(rit->second);
		}
	}

	/* now we can add them in as a tree! */
	ui.groupTreeWidget->fillGroupItems(mYourGroups, adminList);
	ui.groupTreeWidget->fillGroupItems(mSubscribedGroups, subList);
	ui.groupTreeWidget->fillGroupItems(mPopularGroups, popList);
	ui.groupTreeWidget->fillGroupItems(mOtherGroups, otherList);
}

void WikiDialog::GroupMetaDataToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo)
{
	groupItemInfo.id = QString::fromStdString(groupInfo.mGroupId.toStdString());
	groupItemInfo.name = QString::fromUtf8(groupInfo.mGroupName.c_str());
	//groupItemInfo.description = QString::fromUtf8(groupInfo.forumDesc);
	groupItemInfo.popularity = groupInfo.mPop;
	groupItemInfo.lastpost = DateTime::DateTimeFromTime_t(groupInfo.mLastPost);
	groupItemInfo.subscribeFlags = groupInfo.mSubscribeFlags;

	groupItemInfo.icon = GxsIdDetails::makeDefaultGroupIcon(groupInfo.mGroupId, IMAGE_WIKI, GxsIdDetails::ORIGINAL);

}

// updateDisplay() method removed - now using async loading pattern

void WikiDialog::insertWikiGroups()
{
	loadGroupMeta();
}

void WikiDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    // Cast to the specific Wiki event
    const RsGxsWikiEvent *e = dynamic_cast<const RsGxsWikiEvent*>(event.get());

    if(e) {
        std::cerr << "WikiDialog: Received event for group " << e->mWikiGroupId.toStdString() << std::endl;

        switch(e->mWikiEventCode) {
            case RsWikiEventCode::NEW_COLLECTION:
            case RsWikiEventCode::UPDATED_COLLECTION:
            case RsWikiEventCode::SUBSCRIBE_STATUS_CHANGED:
                // Refresh global list for any collection changes
                loadGroupMeta();
                break;

            case RsWikiEventCode::NEW_SNAPSHOT:
            case RsWikiEventCode::UPDATED_SNAPSHOT:
                // Only refresh if we are currently looking at the changed group
                if (e->mWikiGroupId == mGroupId) {
                    wikiGroupChanged(QString::fromStdString(mGroupId.toStdString()));
                }
                break;

            case RsWikiEventCode::NEW_COMMENT:
                // TODO: Refresh comments view when implemented (Task 6)
                std::cerr << "WikiDialog: New comment received for group " << e->mWikiGroupId.toStdString() << std::endl;
                break;
        }
    }
}

