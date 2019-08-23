/*******************************************************************************
 * gui/common/NewFriendList.h                                                  *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#pragma once

#include <set>

#include <QWidget>
#include <QTreeView>

#include "FriendListModel.h"
#include "RsAutoUpdatePage.h"
#include "retroshare/rsstatus.h"

namespace Ui {
    class NewFriendList;
}

class RSTreeWidgetItemCompareRole;
class QTreeWidgetItem;
class QToolButton;
class FriendListSortFilterProxyModel;

class NewFriendList: public RsAutoUpdatePage
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorGroup          READ textColorGroup          WRITE setTextColorGroup)
	Q_PROPERTY(QColor textColorStatusOffline  READ textColorStatusOffline  WRITE setTextColorStatusOffline)
	Q_PROPERTY(QColor textColorStatusAway     READ textColorStatusAway     WRITE setTextColorStatusAway)
	Q_PROPERTY(QColor textColorStatusBusy     READ textColorStatusBusy     WRITE setTextColorStatusBusy)
	Q_PROPERTY(QColor textColorStatusOnline   READ textColorStatusOnline   WRITE setTextColorStatusOnline)
	Q_PROPERTY(QColor textColorStatusInactive READ textColorStatusInactive WRITE setTextColorStatusInactive)

public:
	explicit NewFriendList(QWidget *parent = 0);
	~NewFriendList();

	// Add a tool button to the tool area
	void addToolButton(QToolButton *toolButton);
	void processSettings(bool load);
	void addGroupToExpand(const RsNodeGroupId &groupId);
	bool getExpandedGroups(std::set<RsNodeGroupId> &groups) const;
	void addPeerToExpand(const RsPgpId &gpgId);
	bool getExpandedPeers(std::set<RsPgpId> &peers) const;
	void setColumnVisible(int col,bool visible);
	bool isColumnVisible(int col) const;

	std::string getSelectedGroupId() const;

    void updateDisplay() override;

	QColor textColorGroup()          const { return mModel->mTextColorGroup; }
	QColor textColorStatusOffline()  const { return mModel->mTextColorStatus[RS_STATUS_OFFLINE ]; }
	QColor textColorStatusAway()     const { return mModel->mTextColorStatus[RS_STATUS_AWAY    ]; }
	QColor textColorStatusBusy()     const { return mModel->mTextColorStatus[RS_STATUS_BUSY    ]; }
	QColor textColorStatusOnline()   const { return mModel->mTextColorStatus[RS_STATUS_ONLINE  ]; }
	QColor textColorStatusInactive() const { return mModel->mTextColorStatus[RS_STATUS_INACTIVE]; }

	void setTextColorGroup(QColor color)          { mModel->mTextColorGroup = color; }
	void setTextColorStatusOffline(QColor color)  { mModel->mTextColorStatus[RS_STATUS_OFFLINE ] = color; }
	void setTextColorStatusAway(QColor color)     { mModel->mTextColorStatus[RS_STATUS_AWAY    ] = color; }
	void setTextColorStatusBusy(QColor color)     { mModel->mTextColorStatus[RS_STATUS_BUSY    ] = color; }
	void setTextColorStatusOnline(QColor color)   { mModel->mTextColorStatus[RS_STATUS_ONLINE  ] = color; }
	void setTextColorStatusInactive(QColor color) { mModel->mTextColorStatus[RS_STATUS_INACTIVE] = color; }

public slots:
	void filterItems(const QString &text);
	void toggleSortByState(bool sort);
	void forceUpdateDisplay();

	void toggleColumnVisible();
	void setShowGroups(bool show);
	void setShowUnconnected(bool hidden);
	void setShowState(bool show);
    void headerContextMenuRequested(QPoint);

private slots:
	void sortColumn(int col,Qt::SortOrder so);

protected:
	void changeEvent(QEvent *e);
	void createDisplayMenu();

private:
	Ui::NewFriendList *ui;
	RsFriendListModel *mModel;
	QAction *mActionSortByState;

	void recursRestoreExpandedItems(const QModelIndex& index, const QString& parent_path, const std::set<QString>& exp, const std::set<QString> &sel);
	void recursSaveExpandedItems(const QModelIndex& index,const QString& parent_path,std::set<QString>& exp, std::set<QString>& sel);
	void saveExpandedPathsAndSelection(std::set<QString>& expanded_indexes, std::set<QString>& selected_indexes);
	void restoreExpandedPathsAndSelection(const std::set<QString>& expanded_indexes, const std::set<QString>& selected_indexes);

    void checkInternalData(bool force);

	QModelIndex getCurrentSourceIndex() const;

	bool getCurrentNode(RsFriendListModel::RsNodeDetails& prof) const;
	bool getCurrentGroup(RsGroupInfo& prof) const;
	bool getCurrentProfile(RsFriendListModel::RsProfileDetails& prof) const;

	// Settings for peer list display
	bool mShowState;

	std::set<RsNodeGroupId> openGroups;
	std::set<RsPgpId>   openPeers;

	bool getOrCreateGroup(const std::string& name, uint flag, RsNodeGroupId& id);
	bool getGroupIdByName(const std::string& name, RsNodeGroupId& id);

	bool importExportFriendlistFileDialog(QString &fileName, bool import);
	bool exportFriendlist(QString &fileName);
	bool importFriendlist(QString &fileName, bool &errorPeers, bool &errorGroups);

	FriendListSortFilterProxyModel *mProxyModel ;
private slots:
	void peerTreeWidgetCustomPopupMenu();
	void pastePerson();
	void connectNode();
	void configureNode();
	void configureProfile();
	void chatNode();
	void copyFullCertificate();
	void addFriend();
	void msgNode();
	void msgGroup();
	void msgProfile();
	void recommendNode();
	void removeNode();
	void removeProfile();
	void createNewGroup() ;

	void addToGroup();
	void moveToGroup();
	void removeFromGroup();

	void editGroup();
	void removeGroup();

	void exportFriendlistClicked();
	void importFriendlistClicked();
};
