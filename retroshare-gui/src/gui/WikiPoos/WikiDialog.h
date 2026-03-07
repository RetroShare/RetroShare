/*******************************************************************************
 * gui/WikiPoos/WikiDialog.h                                                   *
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

#ifndef MRK_WIKI_DIALOG_H
#define MRK_WIKI_DIALOG_H

#include <QMessageBox>

#include "retroshare-gui/mainpage.h"
#include "ui_WikiDialog.h"

#include <retroshare/rswiki.h>

#include <map>

#define IMAGE_WIKI              ":/icons/png/wiki.png"

class WikiAddDialog;
class WikiEditDialog;
class UserNotify;
class GxsCommentTreeWidget;
class QTreeWidgetItem;

class WikiDialog : public MainPage
{
  Q_OBJECT

public:
	WikiDialog(QWidget *parent = 0);
	~WikiDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_WIKI) ; } //MainPage
	virtual QString pageName() const { return tr("Wiki") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	virtual UserNotify *createUserNotify(QObject *parent) override;

protected:
	virtual void showEvent(QShowEvent *event) override;

private slots:
	void updateDisplay();



private slots:

	void OpenOrShowAddPageDialog();
	void OpenOrShowAddGroupDialog();
	void OpenOrShowEditDialog();
	void OpenOrShowRepublishDialog();

	void groupTreeChanged();
	void pagesTreeCustomPopupMenu(QPoint point);
	void markPageAsRead();
	void markPageAsUnread();

	void newGroup();
	void showGroupDetails();
	void editGroupDetails();

	// GroupTreeWidget stuff.
	void groupListCustomPopupMenu(QPoint point);
	void subscribeToGroup();
	void unsubscribeToGroup();
	void wikiGroupChanged(const QString &groupId);

	void insertWikiGroups();
	
	// Search filter
	void filterPages();
	
	// Comments (placeholder for future UI integration)
	void loadComments(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId);

private:

	void clearWikiPage();
	void clearGroupTree();

	void updateWikiPage(const RsWikiSnapshot &page);
	void setSelectedPageReadStatus(bool read);
	QTreeWidgetItem *findPageItem(const RsGxsMessageId &pageId) const;

	bool getSelectedPage(RsGxsGroupId &groupId, RsGxsMessageId &pageId, RsGxsMessageId &origPageId);
	std::string getSelectedPage();
	const RsGxsGroupId &getSelectedGroup();

	uint32_t mEventHandlerId;
    void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

	// Using GroupTreeWidget.
	void wikiSubscribe(bool subscribe);
	void GroupMetaDataToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo);
	void insertGroupsData(const std::list<RsGroupMetaData> &wikiList);

	void processSettings(bool load);

	// Async data loading methods
	void loadGroupMeta();
	void loadPages(const RsGxsGroupId &groupId);
	void loadWikiPage(const RsGxsGrpMsgIdPair &msgId);
	void updateModerationState(const RsGxsGroupId &groupId, int subscribeFlags);

	WikiAddDialog *mAddPageDialog;
	WikiAddDialog *mAddGroupDialog;
	WikiEditDialog *mEditDialog;

	std::string mGroupSelected;
	RsGxsMessageId mPageSelected;
	std::string mModSelected;

	GxsCommentTreeWidget *mCommentTreeWidget;
	RsGxsGroupId mCurrentGroupId;
	RsGxsMessageId mCurrentPageId;

	QTreeWidgetItem *mYourGroups;
	QTreeWidgetItem *mSubscribedGroups;
	QTreeWidgetItem *mPopularGroups;
	QTreeWidgetItem *mOtherGroups;
	RsGxsGroupId mGroupId; // From GroupTreeWidget
	bool mCanModerate = false;
	bool mInitialLoadDone = false;

	/* UI - from Designer */
	Ui::WikiDialog ui;

};

#endif
