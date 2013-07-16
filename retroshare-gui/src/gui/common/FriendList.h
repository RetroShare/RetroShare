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
class QMenu;

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

    void setBigName(bool bigName); // show customStateString in second line of the name cell
    void setShowGroups(bool show);
    void setHideUnconnected(bool hidden);
    void setHideState(bool hidden);
    void setShowStatusColumn(bool show);
    void setShowLastContactColumn(bool show);
    void setShowIPColumn(bool show);
    void setShowAvatarColumn(bool show);
    void setRootIsDecorated(bool show);
    void setSortByName();
    void setSortByState();
    void setSortByLastContact();
    void setSortByIP();
    void sortPeersAscendingOrder();
    void sortPeersDescendingOrder();

protected:
    void changeEvent(QEvent *e);

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

    /* Color definitions (for standard see qss.default) */
    QColor mTextColorGroup;
    QColor mTextColorStatus[RS_STATUS_COUNT];

    QTreeWidgetItem *getCurrentPeer() const;
    static bool filterItem(QTreeWidgetItem *item, const QString &text);
    void updateHeader();
    void initializeHeader(bool afterLoadSettings);
    void getSslIdsFromItem(QTreeWidgetItem *item, std::list<std::string> &sslIds);

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
    void copyFullCertificate();
//    void exportfriend();
    void addFriend();
    void msgfriend();
    void recommendfriend();
    void removefriend();
#ifdef UNFINISHED_FD
    void viewprofile();
#endif
	 void servicePermission() ;
	 void recommendFriends() ;
	 void createNewGroup() ;

    void addToGroup();
    void moveToGroup();
    void removeFromGroup();

    void editGroup();
    void removeGroup();

//	 void inviteToLobby();
//	 void createchatlobby();
//	 void unsubscribeToLobby();
//	 void showLobby();
};

#endif // FRIENDLIST_H
