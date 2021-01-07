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

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"
#include "ui_WikiDialog.h"

#include <retroshare/rswiki.h>

#include "util/TokenQueue.h"

#include <map>

#define IMAGE_WIKI              ":/icons/png/wiki.png"

class WikiAddDialog;
class WikiEditDialog;

class WikiDialog : public RsGxsUpdateBroadcastPage, public TokenResponse
{
  Q_OBJECT

public:
	WikiDialog(QWidget *parent = 0);
	~WikiDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_WIKI) ; } //MainPage
	virtual QString pageName() const { return tr("Wiki Pages") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	void loadRequest(const TokenQueue *queue, const TokenRequest &req);

public:
	virtual void updateDisplay(bool complete);

private slots:

	void OpenOrShowAddPageDialog();
	void OpenOrShowAddGroupDialog();
	void OpenOrShowEditDialog();
	void OpenOrShowRepublishDialog();

	void groupTreeChanged();

	void newGroup();
	void showGroupDetails();
	void editGroupDetails();

	// GroupTreeWidget stuff.
	void groupListCustomPopupMenu(QPoint point);
	void subscribeToGroup();
	void unsubscribeToGroup();
	void wikiGroupChanged(const QString &groupId);

	void todo();
	void insertWikiGroups();

private:

	void clearWikiPage();
	void clearGroupTree();

	void updateWikiPage(const RsWikiSnapshot &page);

	bool getSelectedPage(RsGxsGroupId &groupId, RsGxsMessageId &pageId, RsGxsMessageId &origPageId);
	std::string getSelectedPage();
	const RsGxsGroupId &getSelectedGroup();

	// Using GroupTreeWidget.
	void wikiSubscribe(bool subscribe);
	void GroupMetaDataToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo);
	void insertGroupsData(const std::list<RsGroupMetaData> &wikiList);

	void processSettings(bool load);

	void requestGroupMeta();
	void loadGroupMeta(const uint32_t &token);

	void requestPages(const std::list<RsGxsGroupId> &groupIds);
	void loadPages(const uint32_t &token);

	void requestWikiPage(const  RsGxsGrpMsgIdPair &msgId);
	void loadWikiPage(const uint32_t &token);

	TokenQueue *mWikiQueue;

	WikiAddDialog *mAddPageDialog;
	WikiAddDialog *mAddGroupDialog;
	WikiEditDialog *mEditDialog;

	std::string mGroupSelected;
	RsGxsMessageId mPageSelected;
	std::string mModSelected;


	QTreeWidgetItem *mYourGroups;
	QTreeWidgetItem *mSubscribedGroups;
	QTreeWidgetItem *mPopularGroups;
	QTreeWidgetItem *mOtherGroups;
	RsGxsGroupId mGroupId; // From GroupTreeWidget

	/* UI - from Designer */
	Ui::WikiDialog ui;

};

#endif

