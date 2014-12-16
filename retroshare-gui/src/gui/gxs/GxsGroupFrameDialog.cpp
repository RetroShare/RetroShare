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

#include <QMenu>
#include <QMessageBox>
#include <QToolButton>

#include "GxsGroupFrameDialog.h"
#include "ui_GxsGroupFrameDialog.h"
#include "GxsMessageFrameWidget.h"

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsGroupShareKey.h"
#include "gui/common/RSTreeWidget.h"
#include "gui/notifyqt.h"
#include "gui/common/UIStateHelper.h"
#include "GxsCommentDialog.h"

//#define DEBUG_GROUPFRAMEDIALOG

/* Images for TreeWidget */
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
//#define IMAGE_GROUPAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"
#define IMAGE_EDIT           ":/images/edit_16.png"
#define IMAGE_SHARE          ":/images/share-icon-16.png"
#define IMAGE_TABNEW         ":/images/tab-new.png"
#define IMAGE_COMMENT        ""

#define TOKEN_TYPE_GROUP_SUMMARY    1
//#define TOKEN_TYPE_SUBSCRIBE_CHANGE 2
//#define TOKEN_TYPE_CURRENTGROUP     3
#define TOKEN_TYPE_STATISTICS       4

#define MAX_COMMENT_TITLE 32

/*
 * Transformation Notes:
 *   there are still a couple of things that the new groups differ from Old version.
 *   these will need to be addressed in the future.
 *     -> Child TS (for sorting) is not handled by GXS, this will probably have to be done in the GUI.
 *     -> Need to handle IDs properly.
 *     -> Much more to do.
 */

/** Constructor */
GxsGroupFrameDialog::GxsGroupFrameDialog(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
: RsGxsUpdateBroadcastPage(ifaceImpl, parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::GxsGroupFrameDialog();
	ui->setupUi(this);

	mInitialized = false;
	mCountChildMsgs = false;
	mYourGroups = NULL;
	mSubscribedGroups = NULL;
	mPopularGroups = NULL;
	mOtherGroups = NULL;
	mMessageWidget = NULL;

	/* Setup Queue */
	mInterface = ifaceImpl;
	mTokenService = mInterface->getTokenService();
	mTokenQueue = new TokenQueue(mInterface->getTokenService(), this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	mStateHelper->addWidget(TOKEN_TYPE_GROUP_SUMMARY, ui->loadingLabel, UISTATE_LOADING_VISIBLE);

	connect(ui->groupTreeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(groupTreeCustomPopupMenu(QPoint)));
	connect(ui->groupTreeWidget, SIGNAL(treeItemActivated(QString)), this, SLOT(changedGroup(QString)));
	connect(ui->groupTreeWidget->treeWidget(), SIGNAL(signalMouseMiddleButtonClicked(QTreeWidgetItem*)), this, SLOT(groupTreeMiddleButtonClicked(QTreeWidgetItem*)));
	connect(ui->messageTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(messageTabCloseRequested(int)));
	connect(ui->messageTabWidget, SIGNAL(currentChanged(int)), this, SLOT(messageTabChanged(int)));

	connect(ui->todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));

	/* Set initial size the splitter */
	ui->splitter->setStretchFactor(0, 0);
	ui->splitter->setStretchFactor(1, 1);

	QList<int> sizes;
	sizes << 300 << width(); // Qt calculates the right sizes
	ui->splitter->setSizes(sizes);

#ifndef UNFINISHED
	ui->todoPushButton->hide();
#endif
}

GxsGroupFrameDialog::~GxsGroupFrameDialog()
{
	// save settings
	processSettings(false);

	delete(mTokenQueue);
	delete(ui);
}

void GxsGroupFrameDialog::initUi()
{
	registerHelpButton(ui->helpButton, getHelpString()) ;

	ui->titleBarPixmap->setPixmap(QPixmap(icon(ICON_NAME)));
	ui->titleBarLabel->setText(text(TEXT_NAME));

	/* Initialize group tree */
	QToolButton *newGroupButton = new QToolButton(this);
	newGroupButton->setIcon(QIcon(icon(ICON_NEW)));
	newGroupButton->setToolTip(text(TEXT_NEW));
	connect(newGroupButton, SIGNAL(clicked()), this, SLOT(newGroup()));
	ui->groupTreeWidget->addToolButton(newGroupButton);

	/* Create group tree */
	mYourGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_YOUR_GROUP), QIcon(icon(ICON_YOUR_GROUP)), true);
	mSubscribedGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_SUBSCRIBED_GROUP), QIcon(icon(ICON_SUBSCRIBED_GROUP)), true);
	mPopularGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_POPULAR_GROUP), QIcon(icon(ICON_POPULAR_GROUP)), false);
	mOtherGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_OTHER_GROUP), QIcon(icon(ICON_OTHER_GROUP)), false);

	if (text(TEXT_TODO).isEmpty()) {
		ui->todoPushButton->hide();
	}

	// load settings
	mSettingsName = settingsGroupName();
	processSettings(true);

	if (groupFrameSettingsType() != GroupFrameSettings::Nothing) {
		connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
		settingsChanged();
	}
}

void GxsGroupFrameDialog::showEvent(QShowEvent *event)
{
	if (!mInitialized) {
		/* Problem: virtual methods cannot be used in constructor */
		mInitialized = true;

		initUi();
	}

	RsGxsUpdateBroadcastPage::showEvent(event);
}

void GxsGroupFrameDialog::processSettings(bool load)
{
	if (mSettingsName.isEmpty()) {
		return;
	}

	Settings->beginGroup(mSettingsName);

	if (load) {
		// load settings

		// state of splitter
		ui->splitter->restoreState(Settings->value("Splitter").toByteArray());
	} else {
		// save settings

		// state of splitter
		Settings->setValue("Splitter", ui->splitter->saveState());
	}

	ui->groupTreeWidget->processSettings(Settings, load);

	Settings->endGroup();
}

void GxsGroupFrameDialog::settingsChanged()
{
	GroupFrameSettings groupFrameSettings;
	if (Settings->getGroupFrameSettings(groupFrameSettingsType(), groupFrameSettings)) {
		setSingleTab(!groupFrameSettings.mOpenAllInNewTab);
		setHideTabBarWithOneTab(groupFrameSettings.mHideTabBarWithOneTab);
	}
}

void GxsGroupFrameDialog::setSingleTab(bool singleTab)
{
	if (singleTab) {
		if (!mMessageWidget) {
			mMessageWidget = createMessageWidget(RsGxsGroupId());
			// remove close button of the the first tab
			ui->messageTabWidget->hideCloseButton(ui->messageTabWidget->indexOf(mMessageWidget));
		}
	} else {
		if (mMessageWidget) {
			delete(mMessageWidget);
			mMessageWidget = NULL;
		}
	}
}

void GxsGroupFrameDialog::setHideTabBarWithOneTab(bool hideTabBarWithOneTab)
{
	ui->messageTabWidget->setHideTabBarWithOneTab(hideTabBarWithOneTab);
}

void GxsGroupFrameDialog::updateDisplay(bool complete)
{
	if (complete || !getGrpIds().empty() || !getGrpIdsMeta().empty()) {
		/* Update group list */
		requestGroupSummary();
	} else {
		/* Update all groups of changed messages */
		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgIds;
		getAllMsgIds(msgIds);

		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator msgIt;
		for (msgIt = msgIds.begin(); msgIt != msgIds.end(); ++msgIt) {
			updateMessageSummaryList(msgIt->first);
		}
	}
}

void GxsGroupFrameDialog::todo()
{
	QMessageBox::information(this, "Todo", text(TEXT_TODO));
}

void GxsGroupFrameDialog::groupTreeCustomPopupMenu(QPoint point)
{
	QString id = ui->groupTreeWidget->itemIdAt(point);
	if (id.isEmpty()) return;

	mGroupId = RsGxsGroupId(id.toStdString());
	int subscribeFlags = ui->groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId.toStdString()));

	bool isAdmin = IS_GROUP_ADMIN(subscribeFlags);
	bool isPublisher = IS_GROUP_PUBLISHER(subscribeFlags);
	bool isSubscribed = IS_GROUP_SUBSCRIBED(subscribeFlags);

	QMenu contextMnu(this);

	QAction *action;

	if (isSubscribed) {
		action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe"), this, SLOT(unsubscribeGroup()));
		action->setEnabled (!mGroupId.isNull() && IS_GROUP_SUBSCRIBED(subscribeFlags));
	} else {
		action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe"), this, SLOT(subscribeGroup()));
		action->setDisabled (mGroupId.isNull() || IS_GROUP_SUBSCRIBED(subscribeFlags));
	}

	if (mMessageWidget) {
		action = contextMnu.addAction(QIcon(IMAGE_TABNEW), tr("Open in new tab"), this, SLOT(openInNewTab()));
		if (mGroupId.isNull() || messageWidget(mGroupId, true)) {
			action->setEnabled(false);
		}
	}

	contextMnu.addSeparator();

	contextMnu.addAction(QIcon(icon(ICON_NEW)), text(TEXT_NEW), this, SLOT(newGroup()));

	action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Details"), this, SLOT(showGroupDetails()));
	action->setEnabled (!mGroupId.isNull());

	action = contextMnu.addAction(QIcon(IMAGE_EDIT), tr("Edit Details"), this, SLOT(editGroupDetails()));
	action->setEnabled (!mGroupId.isNull() && isAdmin);

	if (shareKeyType()) {
		action = contextMnu.addAction(QIcon(IMAGE_SHARE), tr("Share"), this, SLOT(shareKey()));
        action->setEnabled(!mGroupId.isNull() && isPublisher);
	}

    //if (!mGroupId.isNull() && isPublisher && !isAdmin) {
    //	contextMnu.addAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights" ), this, SLOT(restoreGroupKeys()));
    //}

	if (getLinkType() != RetroShareLink::TYPE_UNKNOWN) {
		action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyGroupLink()));
		action->setEnabled(!mGroupId.isNull());
	}

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(":/images/message-mail-read.png"), tr("Mark all as read"), this, SLOT(markMsgAsRead()));
	action->setEnabled (!mGroupId.isNull() && isSubscribed);

	action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark all as unread"), this, SLOT(markMsgAsUnread()));
	action->setEnabled (!mGroupId.isNull() && isSubscribed);

	/* Add special actions */
	QList<QAction*> actions;
	groupTreeCustomActions(mGroupId, subscribeFlags, actions);
	if (!actions.isEmpty()) {
		contextMnu.addSeparator();
		contextMnu.addActions(actions);
	}

	contextMnu.exec(QCursor::pos());
}

void GxsGroupFrameDialog::restoreGroupKeys(void)
{
	QMessageBox::warning(this, "RetroShare", "ToDo");

#ifdef TOGXS
	mInterface->groupRestoreKeys(mGroupId);
#endif
}

void GxsGroupFrameDialog::newGroup()
{
	GxsGroupDialog *dialog = createNewGroupDialog(mTokenQueue);
	if (!dialog) {
		return;
	}

	dialog->exec();
	delete(dialog);
}

void GxsGroupFrameDialog::subscribeGroup()
{
	groupSubscribe(true);
}

void GxsGroupFrameDialog::unsubscribeGroup()
{
	groupSubscribe(false);
}

void GxsGroupFrameDialog::groupSubscribe(bool subscribe)
{
	if (mGroupId.isNull()) {
		return;
	}

	uint32_t token;
	mInterface->subscribeToGroup(token, mGroupId, subscribe);
// Replaced by meta data changed
//	mTokenQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

void GxsGroupFrameDialog::showGroupDetails()
{
	if (mGroupId.isNull()) {
		return;
	}

	GxsGroupDialog *dialog = createGroupDialog(mTokenQueue, mInterface->getTokenService(), GxsGroupDialog::MODE_SHOW, mGroupId);
	if (!dialog) {
		return;
	}

	dialog->exec();
	delete(dialog);
}

void GxsGroupFrameDialog::editGroupDetails()
{
	if (mGroupId.isNull()) {
		return;
	}

	GxsGroupDialog *dialog = createGroupDialog(mTokenQueue, mInterface->getTokenService(), GxsGroupDialog::MODE_EDIT, mGroupId);
	if (!dialog) {
		return;
	}

	dialog->exec();
	delete(dialog);
}

void GxsGroupFrameDialog::copyGroupLink()
{
	if (mGroupId.isNull()) {
		return;
	}

	RetroShareLink link;

	QString name;
	if(!getCurrentGroupName(name)) return;

	if (link.createGxsGroupLink(getLinkType(), mGroupId, name)) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

bool GxsGroupFrameDialog::getCurrentGroupName(QString& name)
{
	return ui->groupTreeWidget->getGroupName(QString::fromStdString(mGroupId.toStdString()), name);
}

void GxsGroupFrameDialog::markMsgAsRead()
{
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId, false);
	if (msgWidget) {
		msgWidget->setAllMessagesRead(true);
	}
}

void GxsGroupFrameDialog::markMsgAsUnread()
{
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId, false);
	if (msgWidget) {
		msgWidget->setAllMessagesRead(false);
	}
}

void GxsGroupFrameDialog::shareKey()
{
	if (mGroupId.isNull()) {
		return;
	}

//	QMessageBox::warning(this, "", "ToDo");

    GroupShareKey shareUi(this, mGroupId, shareKeyType());
    shareUi.exec();
}

void GxsGroupFrameDialog::loadComment(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId, const QString &title)
{
	RsGxsCommentService *commentService = getCommentService();
	if (!commentService) {
		/* No comment service available */
		return;
	}

	GxsCommentDialog *commentDialog = commentWidget(msgId);
	if (!commentDialog) {
		QString comments = title;
		if (title.length() > MAX_COMMENT_TITLE)
		{
			comments.truncate(MAX_COMMENT_TITLE - 3);
			comments += "...";
		}

		commentDialog = new GxsCommentDialog(this, mInterface->getTokenService(), commentService);

		QWidget *commentHeader = createCommentHeaderWidget(grpId, msgId);
		if (commentHeader) {
			commentDialog->setCommentHeader(commentHeader);
		}

		commentDialog->commentLoad(grpId, msgId);

		int index = ui->messageTabWidget->addTab(commentDialog, comments);
		ui->messageTabWidget->setTabIcon(index, QIcon(IMAGE_COMMENT));
	}

	ui->messageTabWidget->setCurrentWidget(commentDialog);
}

bool GxsGroupFrameDialog::navigate(const RsGxsGroupId &groupId, const RsGxsMessageId& msgId)
{
	if (groupId.isNull()) {
		return false;
	}

	if (mStateHelper->isLoading(TOKEN_TYPE_GROUP_SUMMARY)) {
		mNavigatePendingGroupId = groupId;
		mNavigatePendingMsgId = msgId;

		/* No information if group is available */
		return true;
	}

	QString groupIdString = QString::fromStdString(groupId.toStdString());
	if (ui->groupTreeWidget->activateId(groupIdString, msgId.isNull()) == NULL) {
		return false;
	}

	changedGroup(groupIdString);

	/* search exisiting tab */
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId, false);
	if (!msgWidget) {
		return false;
	}

	if (msgId.isNull()) {
		return true;
	}

	return msgWidget->navigate(msgId);
}

GxsMessageFrameWidget *GxsGroupFrameDialog::messageWidget(const RsGxsGroupId &groupId, bool ownTab)
{
	int tabCount = ui->messageTabWidget->count();
	for (int index = 0; index < tabCount; ++index) {
		GxsMessageFrameWidget *childWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
		if (ownTab && mMessageWidget && childWidget == mMessageWidget) {
			continue;
		}
		if (childWidget && childWidget->groupId() == groupId) {
			return childWidget;
		}
	}

	return NULL;
}

GxsMessageFrameWidget *GxsGroupFrameDialog::createMessageWidget(const RsGxsGroupId &groupId)
{
	GxsMessageFrameWidget *msgWidget = createMessageFrameWidget(groupId);
	if (!msgWidget) {
		return NULL;
	}

	int index = ui->messageTabWidget->addTab(msgWidget, msgWidget->groupName(true));
	ui->messageTabWidget->setTabIcon(index, msgWidget->groupIcon());

	connect(msgWidget, SIGNAL(groupChanged(QWidget*)), this, SLOT(messageTabInfoChanged(QWidget*)));
	connect(msgWidget, SIGNAL(loadComment(RsGxsGroupId,RsGxsMessageId,QString)), this, SLOT(loadComment(RsGxsGroupId,RsGxsMessageId,QString)));

	return msgWidget;
}

GxsCommentDialog *GxsGroupFrameDialog::commentWidget(const RsGxsMessageId &msgId)
{
	int tabCount = ui->messageTabWidget->count();
	for (int index = 0; index < tabCount; ++index) {
		GxsCommentDialog *childWidget = dynamic_cast<GxsCommentDialog*>(ui->messageTabWidget->widget(index));
		if (childWidget && childWidget->messageId() == msgId) {
			return childWidget;
		}
	}

	return NULL;
}

void GxsGroupFrameDialog::changedGroup(const QString &groupId)
{
	mGroupId = RsGxsGroupId(groupId.toStdString());
	if (mGroupId.isNull()) {
		return;
	}

	/* search exisiting tab */
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId, true);

	if (!msgWidget) {
		if (mMessageWidget) {
			/* not found, use standard tab */
			msgWidget = mMessageWidget;
			msgWidget->setGroupId(mGroupId);
		} else {
			/* create new tab */
			msgWidget = createMessageWidget(mGroupId);
		}
	}

	ui->messageTabWidget->setCurrentWidget(msgWidget);
}

void GxsGroupFrameDialog::groupTreeMiddleButtonClicked(QTreeWidgetItem *item)
{
	openGroupInNewTab(RsGxsGroupId(ui->groupTreeWidget->itemId(item).toStdString()));
}

void GxsGroupFrameDialog::openInNewTab()
{
	openGroupInNewTab(mGroupId);
}

void GxsGroupFrameDialog::openGroupInNewTab(const RsGxsGroupId &groupId)
{
	if (groupId.isNull()) {
		return;
	}

	/* search exisiting tab */
	GxsMessageFrameWidget *msgWidget = messageWidget(groupId, true);
	if (!msgWidget) {
		/* not found, create new tab */
		msgWidget = createMessageWidget(groupId);
	}

	ui->messageTabWidget->setCurrentWidget(msgWidget);
}

void GxsGroupFrameDialog::messageTabCloseRequested(int index)
{
	QWidget *widget = ui->messageTabWidget->widget(index);
	if (!widget) {
		return;
	}

	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(widget);
	if (msgWidget && msgWidget == mMessageWidget) {
		/* Don't close single tab */
		return;
	}

	delete(widget);
}

void GxsGroupFrameDialog::messageTabChanged(int index)
{
	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
	if (!msgWidget) {
		return;
	}

	ui->groupTreeWidget->activateId(QString::fromStdString(msgWidget->groupId().toStdString()), false);
}

void GxsGroupFrameDialog::messageTabInfoChanged(QWidget *widget)
{
	int index = ui->messageTabWidget->indexOf(widget);
	if (index < 0) {
		return;
	}

	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
	if (!msgWidget) {
		return;
	}

	ui->messageTabWidget->setTabText(index, msgWidget->groupName(true));
	ui->messageTabWidget->setTabIcon(index, msgWidget->groupIcon());
}

///***** INSERT GROUP LISTS *****/
void GxsGroupFrameDialog::groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo, const RsUserdata */*userdata*/)
{
	groupItemInfo.id = QString::fromStdString(groupInfo.mGroupId.toStdString());
	groupItemInfo.name = QString::fromUtf8(groupInfo.mGroupName.c_str());
	//groupItemInfo.description =
	groupItemInfo.popularity = groupInfo.mPop;
	groupItemInfo.lastpost = QDateTime::fromTime_t(groupInfo.mLastPost);
    groupItemInfo.subscribeFlags = groupInfo.mSubscribeFlags;
    groupItemInfo.privatekey = IS_GROUP_PUBLISHER(groupInfo.mSubscribeFlags) ;
    groupItemInfo.max_visible_posts = groupInfo.mVisibleMsgCount ;

#if TOGXS
	if (groupInfo.mGroupFlags & RS_DISTRIB_AUTHEN_REQ) {
		groupItemInfo.name += " (" + tr("AUTHD") + ")";
		groupItemInfo.icon = QIcon(IMAGE_GROUPAUTHD);
	}
	else
#endif
	{
		groupItemInfo.icon = QIcon(icon(ICON_DEFAULT));
	}
}

void GxsGroupFrameDialog::insertGroupsData(const std::list<RsGroupMetaData> &groupList, const RsUserdata *userdata)
{
	if (!mInitialized) {
		return;
	}

	std::list<RsGroupMetaData>::const_iterator it;

	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for (it = groupList.begin(); it != groupList.end(); ++it) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->mSubscribeFlags;

		GroupItemInfo groupItemInfo;
		groupInfoToGroupItemInfo(*it, groupItemInfo, userdata);

		if (IS_GROUP_SUBSCRIBED(flags))
		{
			if (IS_GROUP_ADMIN(flags))
			{
				adminList.push_back(groupItemInfo);
			}
			else
			{
				/* subscribed group */
				subList.push_back(groupItemInfo);
			}
		}
		else
		{
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
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); ++rit, ++i) ;
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
	ui->groupTreeWidget->fillGroupItems(mYourGroups, adminList);
	ui->groupTreeWidget->fillGroupItems(mSubscribedGroups, subList);
	ui->groupTreeWidget->fillGroupItems(mPopularGroups, popList);
	ui->groupTreeWidget->fillGroupItems(mOtherGroups, otherList);

	/* Re-fill group */
	if (!ui->groupTreeWidget->activateId(QString::fromStdString(mGroupId.toStdString()), true)) {
		mGroupId.clear();
	}

	updateMessageSummaryList(RsGxsGroupId());
}

void GxsGroupFrameDialog::updateMessageSummaryList(RsGxsGroupId groupId)
{
	if (!mInitialized) {
		return;
	}

	if (groupId.isNull()) {
		QTreeWidgetItem *items[2] = { mYourGroups, mSubscribedGroups };
		for (int item = 0; item < 2; ++item) {
			int child;
			int childCount = items[item]->childCount();
			for (child = 0; child < childCount; ++child) {
				QTreeWidgetItem *childItem = items[item]->child(child);
				QString childId = ui->groupTreeWidget->itemId(childItem);
				if (childId.isEmpty()) {
					continue;
				}

				requestGroupStatistics(RsGxsGroupId(childId.toLatin1().constData()));
			}
		}
	} else {
		requestGroupStatistics(groupId);
	}
}

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

void GxsGroupFrameDialog::requestGroupSummary()
{
	mStateHelper->setLoading(TOKEN_TYPE_GROUP_SUMMARY, true);

#ifdef DEBUG_GROUPFRAMEDIALOG
	std::cerr << "GxsGroupFrameDialog::requestGroupSummary()";
	std::cerr << std::endl;
#endif

	mTokenQueue->cancelActiveRequestTokens(TOKEN_TYPE_GROUP_SUMMARY);

	RsTokReqOptions opts;
	opts.mReqType = requestGroupSummaryType();

	uint32_t token;
	mTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_TYPE_GROUP_SUMMARY);
}

void GxsGroupFrameDialog::loadGroupSummaryToken(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo, RsUserdata *&/*userdata*/)
{
	/* Default implementation for request type GXS_REQUEST_TYPE_GROUP_META */
	mInterface->getGroupSummary(token, groupInfo);
}

void GxsGroupFrameDialog::loadGroupSummary(const uint32_t &token)
{
#ifdef DEBUG_GROUPFRAMEDIALOG
	std::cerr << "GxsGroupFrameDialog::loadGroupSummary()";
	std::cerr << std::endl;
#endif

	std::list<RsGroupMetaData> groupInfo;
	RsUserdata *userdata = NULL;
	loadGroupSummaryToken(token, groupInfo, userdata);

	insertGroupsData(groupInfo, userdata);

	mStateHelper->setLoading(TOKEN_TYPE_GROUP_SUMMARY, false);

	if (userdata) {
		delete(userdata);
	}

	if (!mNavigatePendingGroupId.isNull()) {
		/* Navigate pending */
		navigate(mNavigatePendingGroupId, mNavigatePendingMsgId);

		mNavigatePendingGroupId.clear();
		mNavigatePendingMsgId.clear();
	}
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

//void GxsGroupFrameDialog::acknowledgeSubscribeChange(const uint32_t &token)
//{
//#ifdef DEBUG_GROUPFRAMEDIALOG
//	std::cerr << "GxsGroupFrameDialog::acknowledgeSubscribeChange()";
//	std::cerr << std::endl;
//#endif

//	RsGxsGroupId groupId;
//	mInterface->acknowledgeGrp(token, groupId);

//	fillComplete();
//}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

//void GxsGroupFrameDialog::requestGroupSummary_CurrentGroup(const RsGxsGroupId &groupId)
//{
//	RsTokReqOptions opts;
//	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

//	std::list<std::string> grpIds;
//	grpIds.push_back(groupId);

//	std::cerr << "GxsGroupFrameDialog::requestGroupSummary_CurrentGroup(" << groupId << ")";
//	std::cerr << std::endl;

//	uint32_t token;
//	mInteface->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_CURRENTGROUP);
//}

//void GxsGroupFrameDialog::loadGroupSummary_CurrentGroup(const uint32_t &token)
//{
//	std::cerr << "GxsGroupFrameDialog::loadGroupSummary_CurrentGroup()";
//	std::cerr << std::endl;

//	std::list<RsGroupMetaData> groupInfo;
//	rsGxsForums->getGroupSummary(token, groupInfo);

//	if (groupInfo.size() == 1)
//	{
//		RsGroupMetaData fi = groupInfo.front();
//		mSubscribeFlags = fi.mSubscribeFlags;
//	}
//	else
//	{
//		resetData();
//		std::cerr << "GxsGroupFrameDialog::loadGroupSummary_CurrentGroup() ERROR Invalid Number of Groups...";
//		std::cerr << std::endl;
//	}

//	setValid(true);
//}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsGroupFrameDialog::requestGroupStatistics(const RsGxsGroupId &groupId)
{
	uint32_t token;
	mTokenService->requestGroupStatistic(token, groupId);
	mTokenQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_STATISTICS);
}

void GxsGroupFrameDialog::loadGroupStatistics(const uint32_t &token)
{
	GxsGroupStatistic stats;
	mInterface->getGroupStatistic(token, stats);

	QTreeWidgetItem *item = ui->groupTreeWidget->getItemFromId(QString::fromStdString(stats.mGrpId.toStdString()));
	if (!item) {
		return;
	}

    ui->groupTreeWidget->setUnreadCount(item, mCountChildMsgs ? (stats.mNumThreadMsgsUnread + stats.mNumChildMsgsUnread) : stats.mNumThreadMsgsUnread);
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsGroupFrameDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_GROUPFRAMEDIALOG
	std::cerr << "GxsGroupFrameDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	if (queue == mTokenQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
		case TOKEN_TYPE_GROUP_SUMMARY:
			loadGroupSummary(req.mToken);
			break;

//		case TOKEN_TYPE_SUBSCRIBE_CHANGE:
//			acknowledgeSubscribeChange(req.mToken);
//			break;

//		case TOKEN_TYPE_CURRENTGROUP:
//			loadGroupSummary_CurrentGroup(req.mToken);
//			break;

		case TOKEN_TYPE_STATISTICS:
			loadGroupStatistics(req.mToken);
			break;

		default:
			std::cerr << "GxsGroupFrameDialog::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;
		}
	}
}
