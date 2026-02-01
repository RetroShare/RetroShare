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
#include <QShowEvent>

#include "WikiDialog.h"
#include "WikiUserNotify.h"
#include "gui/WikiPoos/WikiAddDialog.h"
#include "gui/WikiPoos/WikiEditDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxs/WikiGroupDialog.h"
#include "gui/gxs/GxsCommentTreeWidget.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"

#include <retroshare/rsgxscommon.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rswiki.h>
#include "util/rstime.h"


// These should be in retroshare/ folder.
#include "retroshare/rsgxsflags.h"


#include <iostream>
#include <sstream>
#include <ctime>

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

#define WIKI_GROUP_COL_GROUPNAME	0
#define WIKI_GROUP_COL_GROUPID		1

#define WIKI_GROUP_COL_PAGENAME		0
#define WIKI_GROUP_COL_PAGEID		1
#define WIKI_GROUP_COL_ORIGPAGEID	2
#define WIKI_PAGE_ROLE_STATUS		(Qt::UserRole + 1)

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

    // Setup comment widget
    mCommentTreeWidget = new GxsCommentTreeWidget(this);
    if (RsGxsCommentService *commentService = dynamic_cast<RsGxsCommentService *>(rsWiki))
    {
        mCommentTreeWidget->setup(commentService);
        ui.commentsContainerLayout->addWidget(mCommentTreeWidget);
    }
    else
    {
        delete mCommentTreeWidget;
        mCommentTreeWidget = nullptr;
    }

    connect( ui.toolButton_NewPage, SIGNAL(clicked()), this, SLOT(OpenOrShowAddPageDialog()));
    connect( ui.toolButton_Edit, SIGNAL(clicked()), this, SLOT(OpenOrShowEditDialog()));
    connect( ui.toolButton_Republish, SIGNAL(clicked()), this, SLOT(OpenOrShowRepublishDialog()));

    connect( ui.treeWidget_Pages, SIGNAL(itemSelectionChanged()), this, SLOT(groupTreeChanged()));
    ui.treeWidget_Pages->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.treeWidget_Pages, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(pagesTreeCustomPopupMenu(QPoint)));

    // GroupTreeWidget.
    connect(ui.groupTreeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(groupListCustomPopupMenu(QPoint)));
    connect(ui.groupTreeWidget, SIGNAL(treeItemActivated(QString)), this, SLOT(wikiGroupChanged(QString)));

    // Search filter
    connect(ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterPages()));

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

UserNotify *WikiDialog::createUserNotify(QObject *parent)
{
	return new WikiUserNotify(rsWiki, parent);
}

void WikiDialog::showEvent(QShowEvent *event)
{
	MainPage::showEvent(event);
	
	// Load data when dialog is first shown
	if(!event->spontaneous())
		updateDisplay();
}

void WikiDialog::processSettings(bool load)
{
	Settings->beginGroup("WikiDialog");

	if (load) {
		// load settings

		// state of splitter
		ui.listSplitter->restoreState(Settings->value("SplitterList").toByteArray());

		const bool showPageId = Settings->value("ShowPageId", false).toBool();
		const bool showOrigId = Settings->value("ShowOrigId", false).toBool();
		ui.treeWidget_Pages->setColumnHidden(WIKI_GROUP_COL_PAGEID, !showPageId);
		ui.treeWidget_Pages->setColumnHidden(WIKI_GROUP_COL_ORIGPAGEID, !showOrigId);
	} else {
		// save settings

		// state of splitter
		Settings->setValue("SplitterList", ui.listSplitter->saveState());
		Settings->setValue("ShowPageId", !ui.treeWidget_Pages->isColumnHidden(WIKI_GROUP_COL_PAGEID));
		Settings->setValue("ShowOrigId", !ui.treeWidget_Pages->isColumnHidden(WIKI_GROUP_COL_ORIGPAGEID));
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

void WikiDialog::pagesTreeCustomPopupMenu(QPoint point)
{
	QTreeWidgetItem *item = ui.treeWidget_Pages->itemAt(point);
	if (item)
	{
		ui.treeWidget_Pages->setCurrentItem(item);
	}

	RsGxsGroupId groupId;
	RsGxsMessageId pageId;
	RsGxsMessageId origPageId;
	const bool hasSelection = getSelectedPage(groupId, pageId, origPageId);

	QMenu contextMenu(this);

	QAction *editAction = contextMenu.addAction(QIcon(IMAGE_EDIT), tr("Edit"), this, SLOT(OpenOrShowEditDialog()));
	editAction->setEnabled(hasSelection);

	if (mCanModerate)
	{
		QAction *republishAction = contextMenu.addAction(QIcon(IMAGE_WIKI), tr("Republish"),
			this, SLOT(OpenOrShowRepublishDialog()));
		republishAction->setEnabled(hasSelection);
	}

	contextMenu.addSeparator();

	QAction *markReadAction = contextMenu.addAction(tr("Mark as read"), this, SLOT(markPageAsRead()));
	QAction *markUnreadAction = contextMenu.addAction(tr("Mark as unread"), this, SLOT(markPageAsUnread()));
	markReadAction->setEnabled(hasSelection);
	markUnreadAction->setEnabled(hasSelection);

	if (hasSelection && item)
	{
		const QVariant statusData = item->data(WIKI_GROUP_COL_PAGENAME, WIKI_PAGE_ROLE_STATUS);
		if (statusData.isValid())
		{
			const uint32_t status = statusData.toUInt();
			const bool isUnread = IS_MSG_UNREAD(status) || IS_MSG_NEW(status);
			markReadAction->setEnabled(isUnread);
			markUnreadAction->setEnabled(!isUnread);
		}
	}

	contextMenu.addSeparator();

	QAction *showPageIdAction = contextMenu.addAction(tr("Show Page Id"));
	showPageIdAction->setCheckable(true);
	showPageIdAction->setChecked(!ui.treeWidget_Pages->isColumnHidden(WIKI_GROUP_COL_PAGEID));
	connect(showPageIdAction, &QAction::toggled, this, [this](bool checked)
	{
		ui.treeWidget_Pages->setColumnHidden(WIKI_GROUP_COL_PAGEID, !checked);
	});

	QAction *showOrigIdAction = contextMenu.addAction(tr("Show Orig Id"));
	showOrigIdAction->setCheckable(true);
	showOrigIdAction->setChecked(!ui.treeWidget_Pages->isColumnHidden(WIKI_GROUP_COL_ORIGPAGEID));
	connect(showOrigIdAction, &QAction::toggled, this, [this](bool checked)
	{
		ui.treeWidget_Pages->setColumnHidden(WIKI_GROUP_COL_ORIGPAGEID, !checked);
	});

	contextMenu.exec(QCursor::pos());
}

void WikiDialog::markPageAsRead()
{
	setSelectedPageReadStatus(true);
}

void WikiDialog::markPageAsUnread()
{
	setSelectedPageReadStatus(false);
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

	// Load comments for this page
	loadComments(page.mMeta.mGroupId, page.mMeta.mMsgId);
}

void WikiDialog::clearWikiPage()
{
	ui.textBrowser->setPlainText("");
}

void WikiDialog::clearGroupTree()
{
	ui.treeWidget_Pages->clear();
}

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
	if (origPageId.isNull())
	{
		origPageId = pageId;
	}

#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedPage() PageId: " << pageId << std::endl;
#endif
	return true;
}

void WikiDialog::setSelectedPageReadStatus(bool read)
{
	RsGxsGroupId groupId;
	RsGxsMessageId pageId;
	RsGxsMessageId origPageId;
	if (!getSelectedPage(groupId, pageId, origPageId))
	{
		return;
	}

	if (!rsWiki)
	{
		return;
	}

	uint32_t token = 0;
	const RsGxsGrpMsgIdPair msgId(groupId, pageId);
	rsWiki->setMessageReadStatus(token, msgId, read);

	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();
	if (!item)
	{
		return;
	}

	const uint32_t mask = GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	uint32_t status = item->data(WIKI_GROUP_COL_PAGENAME, WIKI_PAGE_ROLE_STATUS).toUInt();
	status &= ~mask;
	if (!read)
	{
		status |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	}

	item->setData(WIKI_GROUP_COL_PAGENAME, WIKI_PAGE_ROLE_STATUS, status);

	QFont pageFont = item->font(WIKI_GROUP_COL_PAGENAME);
	pageFont.setBold(!read);
	item->setFont(WIKI_GROUP_COL_PAGENAME, pageFont);
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
#ifdef WIKI_DEBUG
	std::cerr << "WikiDialog::loadGroupMeta()" << std::endl;
#endif

	RsThread::async([this]()
	{
		// Fetch group metadata from backend
		std::vector<RsWikiCollection> collections;
		std::list<RsGxsGroupId> groupIds; // empty list = get all groups
		
		if (!rsWiki->getCollections(groupIds, collections))
		{
			std::cerr << "WikiDialog::loadGroupMeta() Error getting collections" << std::endl;
			return;
		}

		// Convert to RsGroupMetaData for display
		std::list<RsGroupMetaData> groupMeta;
		for (auto& collection : collections)
		{
			groupMeta.push_back(collection.mMeta);
		}

		// Update UI in main thread
		RsQThreadUtils::postToObject([this, groupMeta]()
		{
			if (groupMeta.size() > 0)
			{
				insertGroupsData(groupMeta);
			}
			else
			{
#ifdef WIKI_DEBUG
				std::cerr << "WikiDialog::loadGroupMeta() No groups found" << std::endl;
#endif
			}
		}, this);
	});
}

void WikiDialog::loadPages(const RsGxsGroupId &groupId)
{
#ifdef WIKI_DEBUG
	std::cerr << "WikiDialog::loadPages() for group: " << groupId << std::endl;
#endif

	if (groupId.isNull())
		return;

	RsThread::async([this, groupId]()
	{
		// Fetch snapshots (pages) for the group using token-based API
		uint32_t token;
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
		opts.mOptions = (RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD); // Latest thread heads.

		std::list<RsGxsGroupId> groupIds;
		groupIds.push_back(groupId);

		rsWiki->getTokenService()->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds);
		
		// Poll for completion
		RsTokenService::GxsRequestStatus status = RsTokenService::PENDING;
		while (status == RsTokenService::PENDING)
		{
			rstime::rs_usleep(50 * 1000); // 50ms
			status = rsWiki->getTokenService()->requestStatus(token);
		}
		
		if (status != RsTokenService::COMPLETE)
		{
			std::cerr << "WikiDialog::loadPages() Token request failed" << std::endl;
			return;
		}

		std::vector<RsWikiSnapshot> snapshots;
		if (!rsWiki->getSnapshots(token, snapshots))
		{
			std::cerr << "WikiDialog::loadPages() Error getting snapshots" << std::endl;
			return;
		}

		// Update UI in main thread
		RsQThreadUtils::postToObject([this, snapshots = std::move(snapshots)]()
		{
			clearGroupTree();
			clearWikiPage();

			for (auto& page : snapshots)
			{
#ifdef WIKI_DEBUG
				if (!page.mMeta.mParentId.isNull())
				{
					std::cerr << "WikiDialog::loadPages() Skipping child edit: "
						<< page.mMeta.mMsgId << std::endl;
				}
#endif
				if (!page.mMeta.mParentId.isNull())
				{
					continue;
				}
#ifdef WIKI_DEBUG
				std::cerr << "WikiDialog::loadPages() PageId: " << page.mMeta.mMsgId
					<< " Page: " << page.mMeta.mMsgName << std::endl;
#endif

				QTreeWidgetItem *pageItem = new QTreeWidgetItem();
				pageItem->setText(WIKI_GROUP_COL_PAGENAME, QString::fromStdString(page.mMeta.mMsgName));
				const RsGxsMessageId origId = page.mMeta.mOrigMsgId.isNull()
					? page.mMeta.mMsgId
					: page.mMeta.mOrigMsgId;
				pageItem->setText(WIKI_GROUP_COL_PAGEID, QString::fromStdString(page.mMeta.mMsgId.toStdString()));
				pageItem->setText(WIKI_GROUP_COL_ORIGPAGEID, QString::fromStdString(origId.toStdString()));
				pageItem->setData(WIKI_GROUP_COL_PAGENAME, WIKI_PAGE_ROLE_STATUS,
					static_cast<uint32_t>(page.mMeta.mMsgStatus));

				QFont pageFont = pageItem->font(WIKI_GROUP_COL_PAGENAME);
				const bool isUnread = IS_MSG_UNREAD(page.mMeta.mMsgStatus)
					|| IS_MSG_NEW(page.mMeta.mMsgStatus);
				pageFont.setBold(isUnread);
				pageItem->setFont(WIKI_GROUP_COL_PAGENAME, pageFont);

				ui.treeWidget_Pages->addTopLevelItem(pageItem);
			}
		}, this);
	});
}

/***** Wiki *****/

void WikiDialog::loadWikiPage(const RsGxsGrpMsgIdPair &msgId)
{
#ifdef WIKI_DEBUG
	std::cerr << "WikiDialog::loadWikiPage(" << msgId.first << "," << msgId.second << ")" << std::endl;
#endif

	RsThread::async([this, msgId]()
	{
		// Fetch specific wiki page
		std::vector<RsWikiSnapshot> snapshots;
		
		// Use token system to get snapshot data
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
		
		uint32_t token;
		GxsMsgReq msgIds;
		std::set<RsGxsMessageId> &set_msgIds = msgIds[msgId.first];
		set_msgIds.insert(msgId.second);
		
		rsWiki->getTokenService()->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds);
		
		// Poll for completion
		RsTokenService::GxsRequestStatus status = RsTokenService::PENDING;
		while (status == RsTokenService::PENDING)
		{
			rstime::rs_usleep(50 * 1000); // 50ms
			status = rsWiki->getTokenService()->requestStatus(token);
		}
		
		if (status != RsTokenService::COMPLETE)
		{
			std::cerr << "WikiDialog::loadWikiPage() Token request failed" << std::endl;
			return;
		}
		
		if (!rsWiki->getSnapshots(token, snapshots) || snapshots.empty())
		{
			std::cerr << "WikiDialog::loadWikiPage() Error loading snapshot" << std::endl;
			return;
		}

		RsWikiSnapshot page = snapshots[0];
		
		// Update UI in main thread
		RsQThreadUtils::postToObject([this, page, msgId]()
		{
#ifdef WIKI_DEBUG
			std::cerr << "WikiDialog::loadWikiPage() PageId: " << page.mMeta.mMsgId
				<< " Page: " << page.mMeta.mMsgName << std::endl;
#endif
			updateWikiPage(page);
			
			// TODO: Wiki service does not expose read-status updates.
		}, this);
	});
}

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

	loadPages(mGroupId);

	int subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId.toStdString()));
	updateModerationState(mGroupId, subscribeFlags);
}

void WikiDialog::updateModerationState(const RsGxsGroupId &groupId, int subscribeFlags)
{
	const bool isAdmin = IS_GROUP_ADMIN(subscribeFlags);
	mCanModerate = isAdmin;
	ui.toolButton_NewPage->setEnabled(mCanModerate);
	ui.toolButton_Republish->setEnabled(mCanModerate);

	if (groupId.isNull() || isAdmin)
	{
		return;
	}

	RsThread::async([this, groupId]()
	{
		std::list<RsGxsId> ownIds;
		bool canModerate = false;
		if (rsIdentity->getOwnIds(ownIds))
		{
			for (const auto &ownId : ownIds)
			{
				if (rsWiki->isActiveModerator(groupId, ownId, time(nullptr)))
				{
					canModerate = true;
					break;
				}
			}
		}

		RsQThreadUtils::postToObject([this, groupId, canModerate]()
		{
			if (groupId != mGroupId)
			{
				return;
			}
			mCanModerate = canModerate;
			ui.toolButton_NewPage->setEnabled(mCanModerate);
			ui.toolButton_Republish->setEnabled(mCanModerate);
		}, this);
	});
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

void WikiDialog::updateDisplay()
{
	// Load all group metadata
	loadGroupMeta();
}

void WikiDialog::insertWikiGroups()
{
	updateDisplay();
}

void WikiDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    // Cast to the specific Wiki event
    const RsGxsWikiEvent *e = dynamic_cast<const RsGxsWikiEvent*>(event.get());

    if(e) {
#ifdef WIKI_DEBUG
        std::cerr << "WikiDialog: Received event for group " << e->mWikiGroupId.toStdString() << std::endl;
#endif
        
        switch(e->mWikiEventCode) {
            case RsWikiEventCode::UPDATED_COLLECTION:
            case RsWikiEventCode::NEW_COLLECTION:
                updateDisplay(); // Refresh global list
				if (e->mWikiGroupId == mGroupId)
				{
					int subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId.toStdString()));
					updateModerationState(mGroupId, subscribeFlags);
				}
                break;
            case RsWikiEventCode::UPDATED_SNAPSHOT:
            case RsWikiEventCode::NEW_SNAPSHOT:
                // Only refresh if we are currently looking at the changed group
                if (e->mWikiGroupId == mGroupId) {
                    wikiGroupChanged(QString::fromStdString(mGroupId.toStdString()));
                }
                break;
            case RsWikiEventCode::SUBSCRIBE_STATUS_CHANGED:
                // Refresh group list to update subscription status
                updateDisplay();
				if (e->mWikiGroupId == mGroupId)
				{
					int subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId.toStdString()));
					updateModerationState(mGroupId, subscribeFlags);
				}
                break;
            case RsWikiEventCode::NEW_COMMENT:
                // Refresh comments if the event is for the currently displayed page
                if (e->mWikiGroupId == mCurrentGroupId && !mCurrentPageId.isNull()) {
                    std::set<RsGxsMessageId> msgVersions;
                    msgVersions.insert(mCurrentPageId);
                    if (mCommentTreeWidget) {
                        mCommentTreeWidget->requestComments(mCurrentGroupId, msgVersions, mCurrentPageId);
                    }
                }
                break;
        }
    }
}

void WikiDialog::filterPages()
{
	QString filterText = ui.lineEdit->text().trimmed();
	
	// Filter page tree
	for (int i = 0; i < ui.treeWidget_Pages->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = ui.treeWidget_Pages->topLevelItem(i);
		if (filterText.isEmpty())
		{
			item->setHidden(false);
		}
		else
		{
			QString pageName = item->text(WIKI_GROUP_COL_PAGENAME);
			bool matches = pageName.contains(filterText, Qt::CaseInsensitive);
			item->setHidden(!matches);
		}
	}
}

void WikiDialog::loadComments(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
#ifdef WIKI_DEBUG
	std::cerr << "WikiDialog::loadComments() for page " << msgId << " in group " << groupId << std::endl;
#endif
	
	// Store current page info
	mCurrentGroupId = groupId;
	mCurrentPageId = msgId;
	
	// Load comments using the comment widget
	if (mCommentTreeWidget)
	{
		std::set<RsGxsMessageId> msgVersions;
		msgVersions.insert(msgId);
		mCommentTreeWidget->requestComments(groupId, msgVersions, msgId);
	}
}
