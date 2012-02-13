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

#include "gui/RsAutoUpdatePage.h"

namespace Ui {
    class FriendList;
}

class RSTreeWidgetItemCompareRole;
class QTreeWidgetItem;
class QMenu;

class FriendList : public RsAutoUpdatePage
{
    Q_OBJECT

public:
    explicit FriendList(QWidget *parent = 0);
    ~FriendList();

    QMenu *createDisplayMenu();
    void processSettings(bool bLoad);
    void addGroupToExpand(const std::string &groupId);
    bool getExpandedGroups(std::set<std::string> &groups) const;
    void addPeerToExpand(const std::string &gpgId);
    bool getExpandedPeers(std::set<std::string> &peers) const;

    std::string getSelectedGroupId() const;

    virtual void updateDisplay();

public slots:
    void filterItems(const QString &sPattern);

    void setBigName(bool bigName); // show customStateString in second line of the name cell
    void setShowGroups(bool show);
    void setHideUnconnected(bool hidden);
    void setHideState(bool hidden);
    void setShowStatusColumn(bool show);
    void setShowLastContactColumn(bool show);
    void setShowAvatarColumn(bool show);
    void setRootIsDecorated(bool show);
    void setSortByName();
    void setSortByState();
    void setSortByLastContact();
    void sortPeersAscendingOrder();
    void sortPeersDescendingOrder();

private:
    Ui::FriendList *ui;
    RSTreeWidgetItemCompareRole *m_compareRole;

    // Settings for peer list display
    bool mBigName;
    bool mShowGroups;
    bool mHideState;
    bool mHideUnconnected;

    QString filterText;

    bool groupsHasChanged;
    std::set<std::string> *openGroups;
    std::set<std::string> *openPeers;

    QTreeWidgetItem *getCurrentPeer() const;
    static bool filterItem(QTreeWidgetItem *pItem, const QString &sPattern);
    void updateHeader();
    void initializeHeader(bool afterLoadSettings);

private slots:
    void groupsChanged();
    void insertPeers();
    void peerTreeWidgetCostumPopupMenu();
    void updateAvatar(const QString &);
    void updateMenu();

    void pastePerson();

    void connectfriend();
    void configurefriend();
    void chatfriend(QTreeWidgetItem *);
    void chatfriendproxy();
    void copyLink();
//    void exportfriend();
    void addFriend();
    void msgfriend();
    void recommendfriend();
    void removefriend();
#ifdef UNFINISHED_FD
    void viewprofile();
#endif

    void addToGroup();
    void moveToGroup();
    void removeFromGroup();

    void editGroup();
    void removeGroup();

	 void inviteToLobby();
	 void createchatlobby();
	 void unsubscribeToLobby();
	 void showLobby();
};

#endif // FRIENDLIST_H
