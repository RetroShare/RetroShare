/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2011 RetroShare Team
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

#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include <set>

#include <QWidget>

#include "retroshare-gui/RsAutoUpdatePage.h"
#include "retroshare/rsstatus.h"

namespace Ui {
    class FriendList;
}

class RSTreeWidgetItemCompareRole;
class QTreeWidgetItem;
class QToolButton;

class FriendList : public RsAutoUpdatePage
{
    Q_OBJECT

    Q_PROPERTY(QColor textColorGroup READ textColorGroup WRITE setTextColorGroup)
    Q_PROPERTY(QColor textColorStatusOffline READ textColorStatusOffline WRITE setTextColorStatusOffline)
    Q_PROPERTY(QColor textColorStatusAway READ textColorStatusAway WRITE setTextColorStatusAway)
    Q_PROPERTY(QColor textColorStatusBusy READ textColorStatusBusy WRITE setTextColorStatusBusy)
    Q_PROPERTY(QColor textColorStatusOnline READ textColorStatusOnline WRITE setTextColorStatusOnline)
    Q_PROPERTY(QColor textColorStatusInactive READ textColorStatusInactive WRITE setTextColorStatusInactive)

public:
    enum Column
    {
        COLUMN_NAME         = 0,
        COLUMN_LAST_CONTACT = 1,
        COLUMN_IP           = 2
    };

public:
    explicit FriendList(QWidget *parent = 0);
    ~FriendList();

    // Add a tool button to the tool area
    void addToolButton(QToolButton *toolButton);
    void processSettings(bool load);
    void addGroupToExpand(const RsNodeGroupId &groupId);
    bool getExpandedGroups(std::set<RsNodeGroupId> &groups) const;
    void addPeerToExpand(const RsPgpId &gpgId);
    bool getExpandedPeers(std::set<RsPgpId> &peers) const;

    std::string getSelectedGroupId() const;

    virtual void updateDisplay();
    void setColumnVisible(Column column, bool visible);

    void sortByColumn(Column column, Qt::SortOrder sortOrder);
    bool isSortByState();

    QColor textColorGroup() const { return mTextColorGroup; }
    QColor textColorStatusOffline() const { return mTextColorStatus[RS_STATUS_OFFLINE]; }
    QColor textColorStatusAway() const { return mTextColorStatus[RS_STATUS_AWAY]; }
    QColor textColorStatusBusy() const { return mTextColorStatus[RS_STATUS_BUSY]; }
    QColor textColorStatusOnline() const { return mTextColorStatus[RS_STATUS_ONLINE]; }
    QColor textColorStatusInactive() const { return mTextColorStatus[RS_STATUS_INACTIVE]; }

    void setTextColorGroup(QColor color) { mTextColorGroup = color; }
    void setTextColorStatusOffline(QColor color) { mTextColorStatus[RS_STATUS_OFFLINE] = color; }
    void setTextColorStatusAway(QColor color) { mTextColorStatus[RS_STATUS_AWAY] = color; }
    void setTextColorStatusBusy(QColor color) { mTextColorStatus[RS_STATUS_BUSY] = color; }
    void setTextColorStatusOnline(QColor color) { mTextColorStatus[RS_STATUS_ONLINE] = color; }
    void setTextColorStatusInactive(QColor color) { mTextColorStatus[RS_STATUS_INACTIVE] = color; }

public slots:
    void filterItems(const QString &text);
    void sortByState(bool sort);

    void setShowGroups(bool show);
    void setHideUnconnected(bool hidden);
    void setShowState(bool show);

private slots:
    void peerTreeColumnVisibleChanged(int column, bool visible);
    void peerTreeItemCollapsedExpanded(QTreeWidgetItem *item);
	void collapseItem(QTreeWidgetItem *item);
	void expandItem(QTreeWidgetItem *item);

protected:
    void changeEvent(QEvent *e);
    void createDisplayMenu();

private:
    Ui::FriendList *ui;
    RSTreeWidgetItemCompareRole *mCompareRole;
    QAction *mActionSortByState;

    // Settings for peer list display
    bool mShowGroups;
    bool mShowState;
    bool mHideUnconnected;

    QString mFilterText;

    bool groupsHasChanged;
    std::set<RsNodeGroupId> openGroups;
    std::set<RsPgpId>   openPeers;

    /* Color definitions (for standard see qss.default) */
    QColor mTextColorGroup;
    QColor mTextColorStatus[RS_STATUS_COUNT];

    QTreeWidgetItem *getCurrentPeer() const;
    void getSslIdsFromItem(QTreeWidgetItem *item, std::list<RsPeerId> &sslIds);

    bool getOrCreateGroup(const std::string &name, const uint &flag, RsNodeGroupId &id);
    bool getGroupIdByName(const std::string &name, RsNodeGroupId &id);

    bool importExportFriendlistFileDialog(QString &fileName, bool import);
    bool exportFriendlist(QString &fileName);
    bool importFriendlist(QString &fileName, bool &errorPeers, bool &errorGroups);

private slots:
    void groupsChanged();
    void insertPeers();
    void peerTreeWidgetCustomPopupMenu();
    void updateMenu();

    void pastePerson();

    void connectfriend();
    void configurefriend();
    void chatfriend(QTreeWidgetItem *item);
    void chatfriendproxy();
    //void copyLink();
    void copyFullCertificate();
//    void exportfriend();
    void addFriend();
    void msgfriend();
    void recommendfriend();
    void removefriend();
#ifdef UNFINISHED_FD
    void viewprofile();
#endif
	 void createNewGroup() ;

    void addToGroup();
    void moveToGroup();
    void removeFromGroup();

    void editGroup();
    void removeGroup();

    void exportFriendlistClicked();
    void importFriendlistClicked();

//	 void inviteToLobby();
//	 void createchatlobby();
//	 void unsubscribeToLobby();
//	 void showLobby();
};

#endif // FRIENDLIST_H
