/*******************************************************************************
 * gui/common/FriendList.cpp                                                   *
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

#include <algorithm>

#include <QShortcut>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QWidgetAction>
#include <QDateTime>
#include <QPainter>
#include <QtXml>

#include "rsserver/rsaccounts.h"
#include "retroshare/rspeers.h"

#include "GroupDefs.h"
#include "gui/chat/ChatDialog.h"
//#include "gui/chat/CreateLobbyDialog.h"
#include "gui/common/AvatarDefs.h"

#include "gui/connect/ConfCertDialog.h"
#include "gui/connect/PGPKeyDialog.h"
#include "gui/connect/ConnectFriendWizard.h"
#include "gui/groups/CreateGroup.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#ifdef UNFINISHED_FD
#include "gui/unfinished/profile/ProfileView.h"
#endif
#include "RSTreeWidgetItem.h"
#include "StatusDefs.h"
#include "util/misc.h"
#include "vmessagebox.h"
#include "util/QtVersion.h"
#include "gui/chat/ChatUserNotify.h"
#include "gui/connect/ConnectProgressDialog.h"
#include "gui/common/ElidedLabel.h"

#include "FriendList.h"
#include "ui_FriendList.h"

/* Images for context menu icons */
#define IMAGE_DENYFRIEND         ":/images/denied16.png"
#define IMAGE_REMOVEFRIEND       ":/images/remove_user24.png"
#define IMAGE_EXPORTFRIEND       ":/images/user/friend_suggestion16.png"
#define IMAGE_ADDFRIEND          ":/images/user/add_user16.png"
#define IMAGE_FRIENDINFO         ":/images/info16.png"
#define IMAGE_CHAT               ":/icons/png/chats.png"
#define IMAGE_MSG                ":/icons/mail/write-mail.png"
#define IMAGE_CONNECT            ":/images/connect_friend.png"
#define IMAGE_COPYLINK           ":/images/copyrslink.png"
#define IMAGE_GROUP16            ":/images/user/group16.png"
#define IMAGE_EDIT               ":/images/edit_16.png"
#define IMAGE_REMOVE             ":/images/delete.png"
#define IMAGE_EXPAND             ":/images/edit_add24.png"
#define IMAGE_COLLAPSE           ":/images/edit_remove24.png"
/* Images for Status icons */
#define IMAGE_AVAILABLE          ":/images/user/identityavaiblecyan24.png"
#define IMAGE_PASTELINK          ":/images/pasterslink.png"
#define IMAGE_GROUP24            ":/images/user/group24.png"

#define COLUMN_DATA     0 // column for storing the userdata id

#define ROLE_ID                  Qt::UserRole
#define ROLE_STANDARD            Qt::UserRole + 1
#define ROLE_SORT_GROUP          Qt::UserRole + 2
#define ROLE_SORT_STANDARD_GROUP Qt::UserRole + 3
#define ROLE_SORT_NAME           Qt::UserRole + 4
#define ROLE_SORT_STATE          Qt::UserRole + 5
#define ROLE_FILTER              Qt::UserRole + 6

#define TYPE_GPG   0
#define TYPE_SSL   1
#define TYPE_GROUP 2

// states for sorting (equal values are possible)
// used in BuildSortString - state + name
#define PEER_STATE_ONLINE       1
#define PEER_STATE_BUSY         2
#define PEER_STATE_AWAY         3
#define PEER_STATE_AVAILABLE    4
#define PEER_STATE_INACTIVE     5
#define PEER_STATE_OFFLINE      6

/******
 * #define FRIENDS_DEBUG 1
 *****/

Q_DECLARE_METATYPE(ElidedLabel*)

FriendList::FriendList(QWidget *parent) :
    RsAutoUpdatePage(1500, parent),
    ui(new Ui::FriendList),
    mCompareRole(new RSTreeWidgetItemCompareRole),
    mShowGroups(true),
    mShowState(false),
    mHideUnconnected(false),
    groupsHasChanged(false)
{
    ui->setupUi(this);

    connect(ui->peerTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(peerTreeWidgetCustomPopupMenu()));
    connect(ui->peerTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(expandItem(QTreeWidgetItem *)));
    connect(ui->peerTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(collapseItem(QTreeWidgetItem *)));

#ifdef RS_DIRECT_CHAT
	connect(ui->peerTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(chatfriend(QTreeWidgetItem *)));
#else
	connect( ui->peerTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(expandItem(QTreeWidgetItem *)) );
#endif

    connect(NotifyQt::getInstance(), SIGNAL(groupsChanged(int)), this, SLOT(groupsChanged()));
    connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(insertPeers()));

    connect(ui->actionHideOfflineFriends, SIGNAL(triggered(bool)), this, SLOT(setHideUnconnected(bool)));
    connect(ui->actionShowState, SIGNAL(triggered(bool)), this, SLOT(setShowState(bool)));
    connect(ui->actionShowGroups, SIGNAL(triggered(bool)), this, SLOT(setShowGroups(bool)));
    connect(ui->actionExportFriendlist, SIGNAL(triggered()), this, SLOT(exportFriendlistClicked()));
    connect(ui->actionImportFriendlist, SIGNAL(triggered()), this, SLOT(importFriendlistClicked()));

    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));

    ui->filterLineEdit->setPlaceholderText(tr("Search")) ;
    ui->filterLineEdit->showFilterIcon();

    /* Add filter actions */
    QTreeWidgetItem *headerItem = ui->peerTreeWidget->headerItem();
    QString headerText = headerItem->text(COLUMN_NAME);
    ui->filterLineEdit->addFilter(QIcon(), headerText, COLUMN_NAME, QString("%1 %2").arg(tr("Search"), headerText));
    ui->filterLineEdit->addFilter(QIcon(), tr("ID"), COLUMN_ID, tr("Search ID"));

    mActionSortByState = new QAction(tr("Sort by state"), this);
    mActionSortByState->setCheckable(true);
    connect(mActionSortByState, SIGNAL(toggled(bool)), this, SLOT(sortByState(bool)));
    ui->peerTreeWidget->addContextMenuAction(mActionSortByState);

    /* Set sort */
    sortByColumn(COLUMN_NAME, Qt::AscendingOrder);
    sortByState(false);

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), ui->peerTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT(removefriend()));

    /* Initialize tree */
    ui->peerTreeWidget->enableColumnCustomize(true);
    ui->peerTreeWidget->setColumnCustomizable(COLUMN_NAME, false);
    connect(ui->peerTreeWidget, SIGNAL(columnVisibleChanged(int,bool)), this, SLOT(peerTreeColumnVisibleChanged(int,bool)));
    connect(ui->peerTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(peerTreeItemCollapsedExpanded(QTreeWidgetItem*)));
    connect(ui->peerTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(peerTreeItemCollapsedExpanded(QTreeWidgetItem*)));

    QFontMetricsF fontMetrics(ui->peerTreeWidget->font());

    /* Set initial column width */
    int fontWidth = fontMetrics.width("W");
    ui->peerTreeWidget->setColumnWidth(COLUMN_NAME, 22 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(COLUMN_LAST_CONTACT, 12 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(COLUMN_IP, 15 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(COLUMN_ID, 32 * fontWidth);

    int avatarHeight = fontMetrics.height() * 3;
    ui->peerTreeWidget->setIconSize(QSize(avatarHeight, avatarHeight));

    /* Initialize display menu */
    createDisplayMenu();
}

FriendList::~FriendList()
{
    delete ui;
    delete(mCompareRole);
}

void FriendList::addToolButton(QToolButton *toolButton)
{
    if (!toolButton) {
        return;
    }

    /* Initialize button */
    toolButton->setAutoRaise(true);
    float S = QFontMetricsF(ui->filterLineEdit->font()).height() ;
    toolButton->setIconSize(QSize(S*1.5,S*1.5));
    toolButton->setFocusPolicy(Qt::NoFocus);

    ui->titleBarFrame->layout()->addWidget(toolButton);
}

void FriendList::processSettings(bool load)
{
    // state of peer tree
    ui->peerTreeWidget->setSettingsVersion(2);
    ui->peerTreeWidget->processSettings(load);

    if (load) {
        // load settings

//        ui->peerTreeWidget->header()->doItemsLayout(); // is needed because I added a third column
                                                       // restoreState would corrupt the internal sectionCount

        // states
        setHideUnconnected(Settings->value("hideUnconnected", mHideUnconnected).toBool());
        setShowState(Settings->value("showState", mShowState).toBool());
        setShowGroups(Settings->value("showGroups", mShowGroups).toBool());

        // sort
        sortByState(Settings->value("sortByState", isSortByState()).toBool());

        // open groups
        int arrayIndex = Settings->beginReadArray("Groups");
        for (int index = 0; index < arrayIndex; ++index) {
            Settings->setArrayIndex(index);

            std::string gids = Settings->value("open").toString().toStdString();

            RsGroupInfo ginfo ;

            if(rsPeers->getGroupInfoByName(gids,ginfo)) // backward compatibility
                addGroupToExpand(ginfo.id) ;
            else if(rsPeers->getGroupInfo(RsNodeGroupId(gids),ginfo)) // backward compatibility
                addGroupToExpand(ginfo.id) ;
            else
                std::cerr << "(EE) Cannot find group info for openned group \"" << gids << "\"" << std::endl;
        }
        Settings->endArray();
    } else {
        // save settings

        // states
        Settings->setValue("hideUnconnected", mHideUnconnected);
        Settings->setValue("showState", mShowState);
        Settings->setValue("showGroups", mShowGroups);

        // sort
        Settings->setValue("sortByState", isSortByState());

        // open groups
        Settings->beginWriteArray("Groups");
        int arrayIndex = 0;
        std::set<RsNodeGroupId> expandedPeers;
        getExpandedGroups(expandedPeers);
        foreach (RsNodeGroupId groupId, expandedPeers) {
            Settings->setArrayIndex(arrayIndex++);
            Settings->setValue("open", QString::fromStdString(groupId.toStdString()));
        }
        Settings->endArray();
    }
}

void FriendList::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::StyleChange:
        insertPeers();
        break;
    default:
        // remove compiler warnings
        break;
    }
}

/* Utility Fns */
inline std::string getRsId(QTreeWidgetItem *item)
{
    return item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
}

/**
 * Creates the context popup menu and its submenus,
 * then shows it at the current cursor position.
 */
void FriendList::peerTreeWidgetCustomPopupMenu()
{
    QTreeWidgetItem *c = getCurrentPeer();

    QMenu *contextMenu = new QMenu(this);

    QWidget *widget = new QWidget(contextMenu);
    widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

    // create menu header
    QHBoxLayout *hbox = new QHBoxLayout(widget);
    hbox->setMargin(0);
    hbox->setSpacing(6);

    QLabel *iconLabel = new QLabel(widget);
    QPixmap pix = QPixmap(":/images/user/friends24.png").scaledToHeight(QFontMetricsF(iconLabel->font()).height()*1.5);
    iconLabel->setPixmap(pix);
    iconLabel->setMaximumSize(iconLabel->frameSize().height() + pix.height(), pix.width());
    hbox->addWidget(iconLabel);

    QLabel *textLabel = new QLabel("<strong>RetroShare</strong>", widget);
    hbox->addWidget(textLabel);

    QSpacerItem *spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(spacerItem);

    widget->setLayout(hbox);

    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(widget);
    contextMenu->addAction(widgetAction);

    // create menu entries
    if (c)
     { // if a peer is selected
         int type = c->type();

         // define header
         switch (type) {
             case TYPE_GROUP:
                 //this is a GPG key
                 textLabel->setText("<strong>" + tr("Group") + "</strong>");
                 break;
             case TYPE_GPG:
                 //this is a GPG key
                 textLabel->setText("<strong>" + tr("Friend") + "</strong>");
                 break;
             case TYPE_SSL:
                 //this is a SSL key
                 textLabel->setText("<strong>" + tr("Node") + "</strong>");
                 break;
         }

//         QMenu *lobbyMenu = NULL;

         switch (type)
		 {
         case TYPE_GROUP:
             {
                 bool standard = c->data(COLUMN_DATA, ROLE_STANDARD).toBool();
#ifdef RS_DIRECT_CHAT
                 contextMenu->addAction(QIcon(IMAGE_MSG), tr("Send message to whole group"), this, SLOT(msgfriend()));
                 contextMenu->addSeparator();
#endif // RS_DIRECT_CHAT
                 contextMenu->addAction(QIcon(IMAGE_EDIT), tr("Edit Group"), this, SLOT(editGroup()));

                 QAction *action = contextMenu->addAction(QIcon(IMAGE_REMOVE), tr("Remove Group"), this, SLOT(removeGroup()));
                 action->setDisabled(standard);
             }
             break;
         case TYPE_GPG:
        {
#ifdef RS_DIRECT_CHAT
             contextMenu->addAction(QIcon(IMAGE_CHAT), tr("Chat"), this, SLOT(chatfriendproxy()));
             contextMenu->addAction(QIcon(IMAGE_MSG), tr("Send message"), this, SLOT(msgfriend()));
             contextMenu->addSeparator();
#endif // RS_DIRECT_CHAT

             contextMenu->addAction(QIcon(IMAGE_FRIENDINFO), tr("Profile details"), this, SLOT(configurefriend()));
             contextMenu->addAction(QIcon(IMAGE_DENYFRIEND), tr("Deny connections"), this, SLOT(removefriend()));

             if(mShowGroups)
             {
                 QMenu* addToGroupMenu = NULL;
                 QMenu* moveToGroupMenu = NULL;

                 std::list<RsGroupInfo> groupInfoList;
                 rsPeers->getGroupInfoList(groupInfoList);

                 GroupDefs::sortByName(groupInfoList);

                 RsPgpId gpgId ( getRsId(c));

                 QTreeWidgetItem *parent = c->parent();

                 bool foundGroup = false;
                 // add action for all groups, except the own group
                 for (std::list<RsGroupInfo>::iterator groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
                     if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpgId) == groupIt->peerIds.end()) {
                         if (parent) {
                             if (addToGroupMenu == NULL) {
                                 addToGroupMenu = new QMenu(tr("Add to group"), contextMenu);
                             }
                             QAction* addToGroupAction = new QAction(GroupDefs::name(*groupIt), addToGroupMenu);
                             addToGroupAction->setData(QString::fromStdString(groupIt->id.toStdString()));
                             connect(addToGroupAction, SIGNAL(triggered()), this, SLOT(addToGroup()));
                             addToGroupMenu->addAction(addToGroupAction);
                         }

                         if (moveToGroupMenu == NULL) {
                             moveToGroupMenu = new QMenu(tr("Move to group"), contextMenu);
                         }
                         QAction* moveToGroupAction = new QAction(GroupDefs::name(*groupIt), moveToGroupMenu);
                         moveToGroupAction->setData(QString::fromStdString(groupIt->id.toStdString()));
                         connect(moveToGroupAction, SIGNAL(triggered()), this, SLOT(moveToGroup()));
                         moveToGroupMenu->addAction(moveToGroupAction);
                     } else {
                         foundGroup = true;
                     }
                 }

                 QMenu *groupsMenu = contextMenu->addMenu(QIcon(IMAGE_GROUP16), tr("Groups"));
                 groupsMenu->addAction(QIcon(IMAGE_EXPAND), tr("Create new group"), this, SLOT(createNewGroup()));

                 if (addToGroupMenu || moveToGroupMenu || foundGroup) {
                     if (addToGroupMenu) {
                         groupsMenu->addMenu(addToGroupMenu);
                     }

                     if (moveToGroupMenu) {
                         groupsMenu->addMenu(moveToGroupMenu);
                     }

                     if (foundGroup) {
                         // add remove from group
                         if (parent && parent->type() == TYPE_GROUP) {
                             QAction *removeFromGroup = groupsMenu->addAction(tr("Remove from group"));
                             removeFromGroup->setData(parent->data(COLUMN_DATA, ROLE_ID));
                             connect(removeFromGroup, SIGNAL(triggered()), this, SLOT(removeFromGroup()));
                         }

                         QAction *removeFromAllGroups = groupsMenu->addAction(tr("Remove from all groups"));
                         removeFromAllGroups->setData("");
                         connect(removeFromAllGroups, SIGNAL(triggered()), this, SLOT(removeFromGroup()));
                     }
                 }
             }

        }
         break ;

         case TYPE_SSL:
             {
#ifdef RS_DIRECT_CHAT
                 contextMenu->addAction(QIcon(IMAGE_CHAT), tr("Chat"), this, SLOT(chatfriendproxy()));
                 contextMenu->addAction(QIcon(IMAGE_MSG), tr("Send message to this node"), this, SLOT(msgfriend()));
                 contextMenu->addSeparator();
#endif // RS_DIRECT_CHAT

                 contextMenu->addAction(QIcon(IMAGE_FRIENDINFO), tr("Node details"), this, SLOT(configurefriend()));

                 if (type == TYPE_GPG || type == TYPE_SSL) {
                     contextMenu->addAction(QIcon(IMAGE_EXPORTFRIEND), tr("Recommend this node to..."), this, SLOT(recommendfriend()));
                 }

				 if(!rsPeers->isHiddenNode(rsPeers->getOwnId()) || rsPeers->isHiddenNode( RsPeerId(getRsId(c)) ))
					 contextMenu->addAction(QIcon(IMAGE_CONNECT), tr("Attempt to connect"), this, SLOT(connectfriend()));

                 contextMenu->addAction(QIcon(IMAGE_COPYLINK), tr("Copy certificate link"), this, SLOT(copyFullCertificate()));

                 //this is a SSL key
                 contextMenu->addAction(QIcon(IMAGE_REMOVEFRIEND), tr("Remove Friend Node"), this, SLOT(removefriend()));

             }
         }

     }

    contextMenu->addSeparator();

    QAction *action = contextMenu->addAction(QIcon(IMAGE_PASTELINK), tr("Paste certificate link"), this, SLOT(pastePerson()));
    if (RSLinkClipboard::empty(RetroShareLink::TYPE_CERTIFICATE))
        action->setDisabled(true);

    contextMenu->addAction(QIcon(IMAGE_EXPAND), tr("Expand all"), ui->peerTreeWidget, SLOT(expandAll()));
    contextMenu->addAction(QIcon(IMAGE_COLLAPSE), tr("Collapse all"), ui->peerTreeWidget, SLOT(collapseAll()));

    contextMenu = ui->peerTreeWidget->createStandardContextMenu(contextMenu);

    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

void FriendList::createNewGroup()
{
    CreateGroup createGrpDialog (RsNodeGroupId(), this);
    createGrpDialog.exec();
}

void FriendList::updateDisplay()
{
    insertPeers();
}

void FriendList::groupsChanged()
{
    groupsHasChanged = true;
    insertPeers();
}

static QIcon createAvatar(const QPixmap &avatar, const QPixmap &overlay)
{
	int avatarWidth = avatar.width();
	int avatarHeight = avatar.height();

	QPixmap pixmap(avatar);

	int overlaySize = (avatarWidth > avatarHeight) ? (avatarWidth/2.5) :  (avatarHeight/2.5);
	int overlayX = avatarWidth - overlaySize;
	int overlayY = avatarHeight - overlaySize;

	QPainter painter(&pixmap);
	painter.drawPixmap(overlayX, overlayY, overlaySize, overlaySize, overlay);

	QIcon icon;
	icon.addPixmap(pixmap);
	return icon;
}

static void getNameWidget(QTreeWidget *treeWidget, QTreeWidgetItem *item, ElidedLabel *&nameLabel, ElidedLabel *&textLabel)
{
    QWidget *widget = treeWidget->itemWidget(item, FriendList::COLUMN_NAME);

    if (!widget) {
        widget = new QWidget;
        widget->setAttribute(Qt::WA_TranslucentBackground);
        nameLabel = new ElidedLabel(widget);
        textLabel = new ElidedLabel(widget);

        widget->setProperty("nameLabel", qVariantFromValue(nameLabel));
        widget->setProperty("textLabel", qVariantFromValue(textLabel));

        QVBoxLayout *layout = new QVBoxLayout;
        layout->setSpacing(0);
        layout->setContentsMargins(3, 0, 0, 0);

        nameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        textLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        layout->addWidget(nameLabel);
        layout->addWidget(textLabel);

        widget->setLayout(layout);

        treeWidget->setItemWidget(item, FriendList::COLUMN_NAME, widget);
    } else {
        nameLabel = widget->property("nameLabel").value<ElidedLabel*>();
        textLabel = widget->property("textLabel").value<ElidedLabel*>();
    }
}

/**
 * Get the list of peers from the RsIface.
 * Adds all friend gpg ids, with their nodes as children to the peerTreeWidget.
 * If enabled, peers are sorted in their associated groups.
 * Groups are only updated, when groupsHasChanged is true.
 */
void FriendList::insertPeers()
{
    if (RsAutoUpdatePage::eventsLocked())
        return;

#ifdef FRIENDS_DEBUG
    std::cerr << "FriendList::insertPeers() called." << std::endl;
#endif

    int columnCount = ui->peerTreeWidget->columnCount();

    std::list<StatusInfo> statusInfo;
    rsStatus->getStatusList(statusInfo);

    if (!rsPeers) {
        /* not ready yet! */
        std::cerr << "FriendList::insertPeers() not ready yet : rsPeers unintialized."  << std::endl;
        return;
    }

    // get peers with waiting incoming chats
    std::vector<RsPeerId> privateChatIds;
    ChatUserNotify::getPeersWithWaitingChat(privateChatIds);

    // get existing groups
    std::list<RsGroupInfo> groupInfoList;
    std::list<RsGroupInfo>::iterator groupIt;
    rsPeers->getGroupInfoList(groupInfoList);

    std::list<RsPgpId> gpgFriends;
    std::list<RsPgpId>::iterator gpgIt;
    rsPeers->getGPGAcceptedList(gpgFriends);

    //add own gpg id, if we have more than on node (ssl client)
    std::list<RsPeerId> ownSslContacts;
    RsPgpId ownId = rsPeers->getGPGOwnId();
    rsPeers->getAssociatedSSLIds(ownId, ownSslContacts);
    if (ownSslContacts.size() > 0) {
        gpgFriends.push_back(ownId);
    }

    /* get a link to the table */
    QTreeWidget *peerTreeWidget = ui->peerTreeWidget;

    // remove items don't exist anymore
    QTreeWidgetItemIterator itemIterator(peerTreeWidget);
    QTreeWidgetItem *item;
    while ((item = *itemIterator) != NULL) {
        ++itemIterator;
        switch (item->type()) {
        case TYPE_GPG:
            {
                QTreeWidgetItem *parent = item->parent();
                RsPgpId gpg_widget_id ( getRsId(item));

                // remove items that are not friends anymore
                if (std::find(gpgFriends.begin(), gpgFriends.end(), gpg_widget_id) == gpgFriends.end()) {
                    if (parent) {
                        delete(parent->takeChild(parent->indexOfChild(item)));
                    } else {
                        delete(peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(item)));
                    }
                    break;
                }

                if (mShowGroups && groupsHasChanged) {
                    if (parent) {
                        if (parent->type() == TYPE_GROUP) {
                            RsNodeGroupId groupId(getRsId(parent));

                            // the parent is a group, check if the gpg id is assigned to the group
                            for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
                                if (groupIt->id == groupId) {
                                    if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpg_widget_id) == groupIt->peerIds.end()) {
                                        delete(parent->takeChild(parent->indexOfChild(item)));
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        // gpg item without group, check if the gpg id is assigned to a group
                        for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
                            if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpg_widget_id) != groupIt->peerIds.end()) {
                                delete(peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(item)));
                                break;
                            }
                        }
                    }
                }
            }
            break;
        case TYPE_GROUP:
            {
                if (!mShowGroups) {
                    if (item->parent()) {
                        delete(item->parent()->takeChild(item->parent()->indexOfChild(item)));
                    } else {
                        delete(peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(item)));
                    }
                } else if (groupsHasChanged) {
                    // remove deleted groups
                    RsNodeGroupId groupId ( getRsId(item));
                    for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
                        if (groupIt->id == groupId) {
                            break;
                        }
                    }
                    if (groupIt == groupInfoList.end() || groupIt->peerIds.empty()) {
                        if (item->parent()) {
                            delete(item->parent()->takeChild(item->parent()->indexOfChild(item)));
                        } else {
                            delete(peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(item)));
                        }
                    }
                }
            }
            break;
        }
    }

    std::list<RsPgpId> fillGpgIds;

    // start with groups
    groupIt = groupInfoList.begin();
    while (true) {
        QTreeWidgetItem *groupItem = NULL;
        RsGroupInfo *groupInfo = NULL;
        int onlineCount = 0;
        int availableCount = 0;
        if (mShowGroups && groupIt != groupInfoList.end()) {
            groupInfo = &(*groupIt);

            if ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) && groupInfo->peerIds.empty()) {
                // don't show empty standard groups
                ++groupIt;
                continue;
            }

            // search existing group item
            int itemCount = peerTreeWidget->topLevelItemCount();
            for (int index = 0; index < itemCount; ++index) {
                QTreeWidgetItem *groupItemLoop = peerTreeWidget->topLevelItem(index);
                if (groupItemLoop->type() == TYPE_GROUP && RsNodeGroupId(getRsId(groupItemLoop)) == groupInfo->id) {
                    groupItem = groupItemLoop;
                    break;
                }
            }

            if (groupItem == NULL) {
                // add group item
                groupItem = new RSTreeWidgetItem(mCompareRole, TYPE_GROUP);

                /* Add item to the list. Add here, because for setHidden the item must be added */
                peerTreeWidget->addTopLevelItem(groupItem);

                groupItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
                groupItem->setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter);
                groupItem->setIcon(COLUMN_NAME, QIcon(IMAGE_GROUP24));
                groupItem->setData(COLUMN_NAME, Qt::ForegroundRole, textColorGroup());

                /* used to find back the item */
                QString strID = QString::fromStdString(groupInfo->id.toStdString());
                groupItem->setData(COLUMN_DATA, ROLE_ID, strID);
                //No needs for group ???
                //groupItem->setText(COLUMN_ID, strID);
                //groupItem->setData(COLUMN_ID, ROLE_SORT_NAME, strID);
                //groupItem->setData(COLUMN_ID, ROLE_FILTER, strID);
                groupItem->setData(COLUMN_DATA, ROLE_STANDARD, (groupInfo->flag & RS_GROUP_FLAG_STANDARD) ? true : false);

                /* Sort data */
                for (int i = 0; i < columnCount; ++i) {
                    groupItem->setData(i, ROLE_SORT_GROUP, 1);
                    groupItem->setData(i, ROLE_SORT_STANDARD_GROUP, (groupInfo->flag & RS_GROUP_FLAG_STANDARD) ? 0 : 1);
                    groupItem->setData(i, ROLE_SORT_STATE, 0);
                }
            } else {
                // remove all gpg items that are not more assigned
                int childCount = groupItem->childCount();
                int childIndex = 0;
                while (childIndex < childCount) {
                    QTreeWidgetItem *gpgItemLoop = groupItem->child(childIndex);
                    if (gpgItemLoop->type() == TYPE_GPG) {
                        if (std::find(groupInfo->peerIds.begin(), groupInfo->peerIds.end(), RsPgpId(getRsId(gpgItemLoop))) == groupInfo->peerIds.end()) {
                            delete(groupItem->takeChild(groupItem->indexOfChild(gpgItemLoop)));
                            childCount = groupItem->childCount();
                            continue;
                        }
                    }
                    ++childIndex;
                }
            }

            if (openGroups.find(groupInfo->id) != openGroups.end()) {
                groupItem->setExpanded(true);
            }

            // name is set after calculation of online/offline items
        }

        // iterate through gpg friends
        for (gpgIt = gpgFriends.begin(); gpgIt != gpgFriends.end(); ++gpgIt)
        {
            RsPgpId gpgId = *gpgIt;

            if (mShowGroups) {
                if (groupInfo) {
                    // we fill a group, check if gpg id is assigned
                    if (std::find(groupInfo->peerIds.begin(), groupInfo->peerIds.end(), gpgId) == groupInfo->peerIds.end()) {
                        continue;
                    }
                } else {
                    // we fill the not assigned gpg ids
                    if (std::find(fillGpgIds.begin(), fillGpgIds.end(), gpgId) != fillGpgIds.end()) {
                        continue;
                    }
                }

                // add equal too, its no problem
                fillGpgIds.push_back(gpgId);
            }

            //add the gpg friends
#ifdef FRIENDS_DEBUG
            std::cerr << "FriendList::insertPeers() inserting gpg_id : " << gpgId << std::endl;
#endif

            /* make a widget per friend */
            QTreeWidgetItem *gpgItem = NULL;
            QTreeWidgetItem *gpgItemLoop = NULL;

            // search existing gpg item
            int itemCount = groupItem ? groupItem->childCount() : peerTreeWidget->topLevelItemCount();
            for (int index = 0; index < itemCount; ++index) {
                gpgItemLoop = groupItem ? groupItem->child(index) : peerTreeWidget->topLevelItem(index);
                if (gpgItemLoop->type() == TYPE_GPG && getRsId(gpgItemLoop) == gpgId.toStdString()) {
                    gpgItem = gpgItemLoop;
                    break;
                }
            }

            RsPeerDetails detail;
            if ((!rsPeers->getGPGDetails(gpgId, detail) || !detail.accept_connection) && detail.gpg_id != ownId) {
                // don't accept anymore connection, remove from the view
                if (gpgItem) {
                    if (groupItem) {
                        delete(groupItem->takeChild(groupItem->indexOfChild(gpgItem)));
                    } else {
                        delete (peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(gpgItem)));
                    }
                }
                continue;
            }

            if (gpgItem == NULL) {
                // create gpg item and add it to tree
                gpgItem = new RSTreeWidgetItem(mCompareRole, TYPE_GPG); //set type to 0 for custom popup menu

                /* Add gpg item to the list. Add here, because for setHidden the item must be added */
                if (groupItem) {
                    groupItem->addChild(gpgItem);
                } else {
                    peerTreeWidget->addTopLevelItem(gpgItem);
                }

                gpgItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
                gpgItem->setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter);

                /* not displayed, used to find back the item */
                QString strID = QString::fromStdString(detail.gpg_id.toStdString());
                gpgItem->setData(COLUMN_DATA, ROLE_ID, strID);
                gpgItem->setText(COLUMN_ID, strID);
                gpgItem->setData(COLUMN_ID, ROLE_SORT_NAME, strID);
                gpgItem->setData(COLUMN_ID, ROLE_FILTER, strID);

                /* Sort data */
                for (int i = 0; i < columnCount; ++i) {
                    gpgItem->setData(i, ROLE_SORT_GROUP, 2);
                    gpgItem->setData(i, ROLE_SORT_STANDARD_GROUP, 1);
                }
            }

            ++availableCount;

            // remove items that are not friends anymore
            int childCount = gpgItem->childCount();
            int childIndex = 0;
            while (childIndex < childCount) {
                std::string ssl_id = getRsId(gpgItem->child(childIndex));
                if (!rsPeers->isFriend(RsPeerId(ssl_id))) {
                    delete (gpgItem->takeChild(childIndex));
                    // count again
                    childCount = gpgItem->childCount();
                } else {
                    ++childIndex;
                }
            }

            // update the childs (ssl certs)
            bool gpg_connected = false;
            bool gpg_online = false;
            bool gpg_hasPrivateChat = false;
            int bestPeerState = 0;        // for gpg item
            unsigned int bestRSState = 0; // for gpg item
            QString bestCustomStateString;// for gpg item
            std::list<RsPeerId> sslContacts;
            QDateTime bestLastContact;
            QString bestIP;
            QPixmap bestAvatar;

            rsPeers->getAssociatedSSLIds(detail.gpg_id, sslContacts);
            for (std::list<RsPeerId>::iterator sslIt = sslContacts.begin(); sslIt != sslContacts.end(); ++sslIt) {
                QTreeWidgetItem *sslItem = NULL;
                RsPeerId sslId = *sslIt;

                // find the corresponding sslItem child item of the gpg item
                bool newChild = true;
                childCount = gpgItem->childCount();
                for (int childIndex = 0; childIndex < childCount; ++childIndex) {
                    // we assume, that only ssl items are child of the gpg item, so we don't need to test the type
                    if (getRsId(gpgItem->child(childIndex)) == sslId.toStdString()) {
                        sslItem = gpgItem->child(childIndex);
                        newChild = false;
                        break;
                    }
                }

                RsPeerDetails sslDetail;
                if (!rsPeers->getPeerDetails(sslId, sslDetail) || !rsPeers->isFriend(sslId)) {
#ifdef FRIENDS_DEBUG
                    std::cerr << "Removing widget from the view : id : " << sslId << std::endl;
#endif
                    // child has disappeared, remove it from the gpg_item
                    if (sslItem) {
                        gpgItem->removeChild(sslItem);
                        delete(sslItem);
                    }
                    continue;
                }

                if (newChild) {
                    sslItem = new RSTreeWidgetItem(mCompareRole, TYPE_SSL); //set type to 1 for custom popup menu

#ifdef FRIENDS_DEBUG
                    std::cerr << "FriendList::insertPeers() inserting sslItem." << std::endl;
#endif

                    /* Add ssl child to the list. Add here, because for setHidden the item must be added */
                    gpgItem->addChild(sslItem);

                    /* Sort data */
                    for (int i = 0; i < columnCount; ++i) {
                        sslItem->setData(i, ROLE_SORT_GROUP, 2);
                        sslItem->setData(i, ROLE_SORT_STANDARD_GROUP, 1);
                    }
                }

                /* not displayed, used to find back the item */
                QString strID = QString::fromStdString(sslDetail.id.toStdString());
                sslItem->setData(COLUMN_DATA, ROLE_ID, strID);
                sslItem->setText(COLUMN_ID, strID);
                sslItem->setData(COLUMN_ID, ROLE_SORT_NAME, strID);
                sslItem->setData(COLUMN_ID, ROLE_FILTER, strID);

                /* Custom state string */
                QString customStateString;
                if (sslDetail.state & RS_PEER_STATE_CONNECTED) {
                    customStateString = QString::fromUtf8(rsMsgs->getCustomStateString(sslDetail.id).c_str());
                }

                QPixmap sslAvatar;
                AvatarDefs::getAvatarFromSslId(RsPeerId(sslDetail.id.toStdString()), sslAvatar);

                /* last contact */
                QDateTime sslLastContact = QDateTime::fromTime_t(sslDetail.lastConnect);
                sslItem->setData(COLUMN_LAST_CONTACT, Qt::DisplayRole, QVariant(sslLastContact));
                sslItem->setData(COLUMN_LAST_CONTACT, ROLE_SORT_NAME, QVariant(sslLastContact));
                if (sslLastContact > bestLastContact) {
                    bestLastContact = sslLastContact;
                }

                /* IP */
                QString sslIP = (sslDetail.state & RS_PEER_STATE_CONNECTED) ? StatusDefs::connectStateIpString(sslDetail) : QString("---");
                sslItem->setText(COLUMN_IP, sslIP);
                sslItem->setData(COLUMN_IP, ROLE_SORT_NAME, sslIP);

                /* change color and icon */
                QPixmap sslOverlayIcon;
                QFont sslFont;
                QColor sslColor;
                int peerState = 0;
                QString connectStateString;
                if (sslDetail.state & RS_PEER_STATE_CONNECTED) {
                    // get the status info for this ssl id
                    int rsState = 0;
                    std::list<StatusInfo>::iterator it;
                    for (it = statusInfo.begin(); it != statusInfo.end(); ++it) {
                        if (it->id == sslId) {
                            rsState = it->status;
                            switch (rsState) {
                            case RS_STATUS_INACTIVE:
                                peerState = PEER_STATE_INACTIVE;
                                break;

                            case RS_STATUS_ONLINE:
                                peerState = PEER_STATE_ONLINE;
                                break;

                            case RS_STATUS_AWAY:
                                peerState = PEER_STATE_AWAY;
                                break;

                            case RS_STATUS_BUSY:
                                peerState = PEER_STATE_BUSY;
                                break;
                            }

                            /* find the best ssl contact for the gpg item */
                            if (bestPeerState == 0 || peerState < bestPeerState) {
                                /* first ssl contact or higher state */
                                bestPeerState = peerState;
                                bestRSState = rsState;
                                bestCustomStateString = customStateString;
                                bestIP = sslIP;
                                if (!sslAvatar.isNull()) {
                                    bestAvatar = sslAvatar;
                                }
                            } else if (peerState == bestPeerState) {
                                /* equal state */
                                if (bestCustomStateString.isEmpty() && !customStateString.isEmpty()) {
                                    /* when customStateString is shown in name item, use sslId with customStateString.
                                       second with a custom state string ... use second */
                                    bestPeerState = peerState;
                                    bestRSState = rsState;
                                    bestCustomStateString = customStateString;
                                }
                                if (bestAvatar.isNull() && !sslAvatar.isNull()) {
                                    /* Use available avatar */
                                    bestAvatar = sslAvatar;
                                }
                            }
                            break;
                        }
                    }

                    sslItem->setHidden(false);
                    gpg_connected = true;

                    sslOverlayIcon = QPixmap(StatusDefs::imageStatus(bestRSState));

                    connectStateString = StatusDefs::name(rsState);

                    if (rsState == 0) {
                        sslFont.setBold(true);
                        sslColor = mTextColorStatus[RS_STATUS_ONLINE];

                    } else {
                        sslFont = StatusDefs::font(rsState);
                        sslColor = mTextColorStatus[rsState];
                    }
                } else if (sslDetail.state & RS_PEER_STATE_ONLINE) {
                    sslItem->setHidden(mHideUnconnected);
                    gpg_online = true;
                    peerState = PEER_STATE_AVAILABLE;

                    if (sslDetail.connectState) {
                        sslOverlayIcon = QPixmap(":/images/connect_creating.png");
                    } else {
                        sslOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_ONLINE));
                    }

                    connectStateString = StatusDefs::name(RS_STATUS_ONLINE);

                    sslFont.setBold(true);
                    sslColor = mTextColorStatus[RS_STATUS_ONLINE];
                } else {
                    peerState = PEER_STATE_OFFLINE;
                    sslItem->setHidden(mHideUnconnected);
                    if (sslDetail.connectState) {
                        sslOverlayIcon = QPixmap(":/images/connect_creating.png");
                    } else {
                        sslOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_OFFLINE));
                    }

                    connectStateString = StatusDefs::connectStateWithoutTransportTypeString(sslDetail);

                    sslFont.setBold(false);
                    sslColor = mTextColorStatus[RS_STATUS_OFFLINE];
                }

                /* Location */
                QString sslName = QString::fromUtf8(sslDetail.location.c_str());
                QString sslText;

                if (mShowState) {
                    if (!connectStateString.isEmpty()) {
                        sslText = connectStateString;
                        if (!customStateString.isEmpty()) {
                            sslText += " [" + customStateString + "]";
                        }
                    } else {
                        if (!customStateString.isEmpty()) {
                            sslText = customStateString;
                        }
                    }

                    sslItem->setToolTip(COLUMN_NAME, "");
                } else {
                    if (!customStateString.isEmpty()) {
                        sslText = customStateString;
                    }

                    /* Show the state as tooltip */
                    sslItem->setToolTip(COLUMN_NAME, connectStateString);
                }

                /* Create or get ssl label */
                ElidedLabel *sslNameLabel = NULL;
                ElidedLabel *sslTextLabel = NULL;

                getNameWidget(ui->peerTreeWidget, sslItem, sslNameLabel, sslTextLabel);

                if (sslNameLabel) {
                    sslNameLabel->setText(sslName);
                    sslNameLabel->setFont(sslFont);

                    QPalette palette = sslNameLabel->palette();
                    palette.setColor(sslNameLabel->foregroundRole(), sslColor);

                    sslNameLabel->setPalette(palette);
                }
                if (sslTextLabel) {
                    sslTextLabel->setText(sslText);
                    sslTextLabel->setVisible(!sslText.isEmpty());
                }

                // Filter
                sslItem->setData(COLUMN_NAME, ROLE_FILTER, sslName);

                if (std::find(privateChatIds.begin(), privateChatIds.end(), sslDetail.id) != privateChatIds.end()) {
                    // private chat is available
                    sslOverlayIcon = QPixmap(":/images/chat.png");
                    gpg_hasPrivateChat = true;
                }
                sslItem->setIcon(COLUMN_NAME, createAvatar(sslAvatar, sslOverlayIcon));

                /* Sort data */
                sslItem->setData(COLUMN_NAME, ROLE_SORT_NAME, sslName);

                for (int i = 0; i < columnCount; ++i) {
                    sslItem->setData(i, ROLE_SORT_STATE, peerState);

                    sslItem->setData(i, Qt::ForegroundRole, sslColor);
                    sslItem->setData(i, Qt::FontRole, sslFont);
                }
            }

            QString gpgName = QString::fromUtf8(detail.name.c_str());
            QString gpgText;
            QFont gpgFont;
            QColor gpgColor;

            bool showInfoAtGpgItem = !gpgItem->isExpanded();

            QPixmap gpgOverlayIcon;
            if (gpg_connected) {
                gpgItem->setHidden(false);

                ++onlineCount;

                if (bestPeerState == 0) {
                    // show as online
                    bestPeerState = PEER_STATE_ONLINE;
                    bestRSState = RS_STATUS_ONLINE;
                }

                gpgColor = mTextColorStatus[bestRSState];
                gpgFont = StatusDefs::font(bestRSState);

                if (showInfoAtGpgItem) {
                    gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(bestRSState));

                    if (mShowState) {
                        gpgText = StatusDefs::name(bestRSState);
                        if (!bestCustomStateString.isEmpty()) {
                            gpgText += " [" + bestCustomStateString + "]";
                        }
                    } else {
                        if (!bestCustomStateString.isEmpty()) {
                            gpgText = bestCustomStateString;
                        }
                    }
                }
            } else if (gpg_online) {
                gpgItem->setHidden(mHideUnconnected);
                ++onlineCount;
                bestPeerState = PEER_STATE_AVAILABLE;

                gpgFont.setBold(true);
                gpgColor = mTextColorStatus[RS_STATUS_ONLINE];

                if (showInfoAtGpgItem) {
                    if (mShowState) {
                        gpgText += tr("Available");
                    }

                    gpgOverlayIcon = QPixmap(IMAGE_AVAILABLE);
                }
            } else {
                bestPeerState = PEER_STATE_OFFLINE;
                gpgItem->setHidden(mHideUnconnected);

                gpgFont = StatusDefs::font(RS_STATUS_OFFLINE);
                gpgColor = mTextColorStatus[RS_STATUS_OFFLINE];

                if (showInfoAtGpgItem) {
                    if (mShowState) {
                        gpgText += StatusDefs::name(RS_STATUS_OFFLINE);
                    }

                    gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_OFFLINE));
                }
            }

            if (gpg_hasPrivateChat) {
                gpgOverlayIcon = QPixmap(":/images/chat.png");
            }

            gpgItem->setIcon(COLUMN_NAME, createAvatar(bestAvatar.isNull() ? QPixmap(AVATAR_DEFAULT_IMAGE) : bestAvatar, gpgOverlayIcon));

            /* Create or get gpg label */
            ElidedLabel *gpgNameLabel = NULL;
            ElidedLabel *gpgTextLabel = NULL;

            getNameWidget(ui->peerTreeWidget, gpgItem, gpgNameLabel, gpgTextLabel);

            if (gpgNameLabel) {
                gpgNameLabel->setText(gpgName);
                gpgNameLabel->setFont(gpgFont);

                QPalette palette = gpgNameLabel->palette();
                palette.setColor(gpgNameLabel->foregroundRole(), gpgColor);

                gpgNameLabel->setPalette(palette);
            }
            if (gpgTextLabel) {
                gpgTextLabel->setText(gpgText);
                gpgTextLabel->setVisible(!gpgText.isEmpty());
            }

            // Filter
            gpgItem->setData(COLUMN_NAME, ROLE_FILTER, gpgName);

            gpgItem->setData(COLUMN_LAST_CONTACT, Qt::DisplayRole, showInfoAtGpgItem ? QVariant(bestLastContact) : "");
            gpgItem->setData(COLUMN_LAST_CONTACT, ROLE_SORT_NAME, QVariant(bestLastContact));
            gpgItem->setText(COLUMN_IP, showInfoAtGpgItem ? bestIP : "");
            gpgItem->setData(COLUMN_IP, ROLE_SORT_NAME, bestIP);

            /* Sort data */
            gpgItem->setData(COLUMN_NAME, ROLE_SORT_NAME, gpgName);

            for (int i = 0; i < columnCount; ++i) {
                gpgItem->setData(i, ROLE_SORT_STATE, bestPeerState);

                gpgItem->setData(i, Qt::ForegroundRole, gpgColor);
                gpgItem->setData(i, Qt::FontRole, gpgFont);
            }

            if (openPeers.find(gpgId) != openPeers.end()) {
                gpgItem->setExpanded(true);
            }
        }

        if (groupInfo && groupItem) {
            if ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) && groupItem->childCount() == 0) {
                // there are some dead id's assigned
                groupItem->setHidden(true);
            } else {
                groupItem->setHidden(false);
                QString groupName = GroupDefs::name(*groupInfo);
                groupItem->setText(COLUMN_NAME, QString("%1 (%2/%3)").arg(groupName).arg(onlineCount).arg(availableCount));

                // Filter
                groupItem->setData(COLUMN_NAME, ROLE_FILTER, groupName);

                /* Sort data */
                groupItem->setData(COLUMN_NAME, ROLE_SORT_NAME, groupName);
            }
        }

        if (mShowGroups && groupIt != groupInfoList.end()) {
            ++groupIt;
        } else {
            // all done
            break;
        }
    }

    if (mFilterText.isEmpty() == false) {
        filterItems(mFilterText);
    }

    groupsHasChanged = false;

    ui->peerTreeWidget->resort();
}

/**
 * Returns a list with all groupIds that are expanded
 */
bool FriendList::getExpandedGroups(std::set<RsNodeGroupId> &groups) const
{
    int itemCount = ui->peerTreeWidget->topLevelItemCount();
    for (int index = 0; index < itemCount; ++index) {
        QTreeWidgetItem *item = ui->peerTreeWidget->topLevelItem(index);
        if (item->type() == TYPE_GROUP && item->isExpanded()) {
            groups.insert(RsNodeGroupId(item->data(COLUMN_DATA, ROLE_ID).toString().toStdString()));
        }
    }
    return true;
}

/**
 * Returns a list with all gpg ids that are expanded
 */
bool FriendList::getExpandedPeers(std::set<RsPgpId> &peers) const
{
    peers.clear();
    QTreeWidgetItemIterator it(ui->peerTreeWidget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (item->type() == TYPE_GPG && item->isExpanded()) {
            peers.insert(peers.end(), RsPgpId(getRsId(item)));
        }
        ++it;
    }
    return true;
}

void FriendList::collapseItem(QTreeWidgetItem *item)
{
	switch (item->type())
	{
	case TYPE_GROUP:
		openGroups.erase(RsNodeGroupId(getRsId(item))) ;
		break;
	case TYPE_GPG:
		openPeers.erase(RsPgpId(getRsId(item))) ;
	default:
		break;
	}
}
void FriendList::expandItem(QTreeWidgetItem *item)
{
	switch (item->type())
	{
	case TYPE_GROUP:
		openGroups.insert(RsNodeGroupId(getRsId(item))) ;
		break;
	case TYPE_GPG:
		openPeers.insert(RsPgpId(getRsId(item))) ;
	default:
		break;
	}
}

void FriendList::chatfriendproxy()
{
   chatfriend(getCurrentPeer());
}

/**
 * Start a chat with a friend
 *
 * @param pPeer the gpg or ssl QTreeWidgetItem to chat with
 */
void FriendList::chatfriend(QTreeWidgetItem *item)
{
    if (item == NULL) {
        return;
    }

    switch (item->type()) {
    case TYPE_GROUP:
        break;
    case TYPE_GPG:
        ChatDialog::chatFriend(RsPgpId(getRsId(item)));
        break;
    case TYPE_SSL:
        ChatDialog::chatFriend(ChatId(RsPeerId(getRsId(item))));
        break;
    }
}

void FriendList::addFriend()
{
    std::string groupId = getSelectedGroupId();

    ConnectFriendWizard connwiz (this);

    if (groupId.empty() == false) {
        connwiz.setGroup(groupId);
    }

    connwiz.exec ();
}

void FriendList::msgfriend()
{
    QTreeWidgetItem *item = getCurrentPeer();

    if (!item)
        return;

    switch (item->type()) {
    case TYPE_GROUP:
        break;
    case TYPE_GPG:
        MessageComposer::msgFriend(RsPgpId(getRsId(item)));
        break;
    case TYPE_SSL:
        MessageComposer::msgFriend(RsPeerId(getRsId(item)));
        break;
    }
}

void FriendList::recommendfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::string peerId = getRsId(peer);
    std::list<RsPeerId> ids;

    switch (peer->type())
    {
    case TYPE_SSL:
        ids.push_back(RsPeerId(peerId));
        break;
    case TYPE_GPG:
        rsPeers->getAssociatedSSLIds(RsPgpId(peerId), ids);
        break;
    default:
        return;
    }
    std::set<RsPeerId> sids ;
    for(std::list<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
        sids.insert(*it) ;

    MessageComposer::recommendFriend(sids);
}

void FriendList::pastePerson()
{
    //RSLinkClipboard::process(RetroShareLink::TYPE_PERSON);
    RSLinkClipboard::process(RetroShareLink::TYPE_CERTIFICATE);
}

void FriendList::copyFullCertificate()
{
	QTreeWidgetItem *c = getCurrentPeer();
	QList<RetroShareLink> urls;
	RetroShareLink link = RetroShareLink::createCertificate(RsPeerId(getRsId(c)));
	urls.push_back(link);

	std::cerr << "link: " << std::endl;
	std::cerr<< link.toString().toStdString() << std::endl;

	RSLinkClipboard::copyLinks(urls);
}

// void FriendList::copyLink()
// {
//     QTreeWidgetItem *c = getCurrentPeer();
// 
//     if (c == NULL) {
//         return;
//     }
// 
//     QList<RetroShareLink> urls;
//     RetroShareLink link;
//     if (link.createPerson(getRsId(c))) {
//         urls.push_back(link);
//     }
// 
//     RSLinkClipboard::copyLinks(urls);
// }

/**
 * Find out which group is selected
 *
 * @return If a group item is selected, its groupId otherwise an empty string
 */
std::string FriendList::getSelectedGroupId() const
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->type() == TYPE_GROUP) {
        return getRsId(c);
    }

    return std::string();
}

QTreeWidgetItem *FriendList::getCurrentPeer() const
{
    /* get the current, and extract the Id */
    QTreeWidgetItem *item = ui->peerTreeWidget->currentItem();
#ifdef FRIENDS_DEBUG
    if (!item)
    {
        std::cerr << "Invalid Current Item" << std::endl;
        return NULL;
    }

    /* Display the columns of this item. */
    QString out = "CurrentPeerItem: \n";

    for(int i = 1; i < COLUMN_COUNT; ++i)
    {
        QString txt = item -> text(i);
        out += QString("\t%1:%2\n").arg(i).arg(txt);
    }
    std::cerr << out.toStdString();
#endif
    return item;
}

#ifdef UNFINISHED_FD
/* GUI stuff -> don't do anything directly with Control */
void FriendsDialog::viewprofile()
{
    /* display Dialog */

    QTreeWidgetItem *c = getCurrentPeer();


    //	static ProfileView *profileview = new ProfileView();


    if (!c)
        return;

    /* set the Id */
    std::string id = getRsId(c);

    profileview -> setPeerId(id);
    profileview -> show();
}
#endif

/* So from the Peers Dialog we can call the following control Functions:
 * (1) Remove Current.              FriendRemove(id)
 * (2) Allow/DisAllow.              FriendStatus(id, accept)
 * (2) Connect.                     FriendConnectAttempt(id, accept)
 * (3) Set Address.                 FriendSetAddress(id, str, port)
 * (4) Set Trust.                   FriendTrustSignature(id, bool)
 * (5) Configure (GUI Only) -> 3/4
 *
 * All of these rely on the finding of the current Id.
 */

void FriendList::removefriend()
{
    QTreeWidgetItem *c = getCurrentPeer();
#ifdef FRIENDS_DEBUG
    std::cerr << "FriendList::removefriend()" << std::endl;
#endif
    if (!c)
    {
#ifdef FRIENDS_DEBUG
        std::cerr << "FriendList::removefriend() None Selected -- sorry" << std::endl;
#endif
        return;
    }

    if (rsPeers)
    {
        switch (c->type()) {
        case TYPE_GPG:
            if(!RsPgpId(getRsId(c)).isNull()) {
                if ((QMessageBox::question(this, "RetroShare", tr("Do you want to remove this Friend?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
                {
                    rsPeers->removeFriend(RsPgpId(getRsId(c)));
                }
            }
            break;
        case TYPE_SSL:
            if (!RsPeerId(getRsId(c)).isNull()) {
                if ((QMessageBox::question(this, "RetroShare", tr("Do you want to remove this node?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
                {
                    rsPeers->removeFriendLocation(RsPeerId(getRsId(c)));
                }
            }
            break;
        }
    }
}

void FriendList::connectfriend()
{
    QTreeWidgetItem *c = getCurrentPeer();
#ifdef FRIENDS_DEBUG
    std::cerr << "FriendList::connectfriend()" << std::endl;
#endif
    if (!c)
    {
#ifdef FRIENDS_DEBUG
        std::cerr << "FriendList::connectfriend() None Selected -- sorry" << std::endl;
#endif
        return;
    }

    if (rsPeers)
    {
        if (c->type() == TYPE_GPG) {
            int childCount = c->childCount();
            for (int childIndex = 0; childIndex < childCount; ++childIndex) {
                QTreeWidgetItem *item = c->child(childIndex);
                if (item->type() == TYPE_SSL) {
                    rsPeers->connectAttempt(RsPeerId(getRsId(item)));

	    	    // Launch ProgressDialog, only if single SSL child.
		    if (childCount == 1)
		    {
                ConnectProgressDialog::showProgress(RsPeerId(getRsId(item)));
		    }
                }
            }
        } else {
            //this is a SSL key
            rsPeers->connectAttempt(RsPeerId(getRsId(c)));
	    // Launch ProgressDialog.
        ConnectProgressDialog::showProgress(RsPeerId(getRsId(c)));
        }
    }
}

/* GUI stuff -> don't do anything directly with Control */
void FriendList::configurefriend()
{
    if(!RsPeerId(getRsId(getCurrentPeer())).isNull())
        ConfCertDialog::showIt(RsPeerId(getRsId(getCurrentPeer())), ConfCertDialog::PageDetails);
    else if(!RsPgpId(getRsId(getCurrentPeer())).isNull())
        PGPKeyDialog::showIt(RsPgpId(getRsId(getCurrentPeer())), PGPKeyDialog::PageDetails);
    else
        std::cerr << "FriendList::configurefriend: id is not an SSL nor a PGP id." << std::endl;
}

// void FriendList::showLobby()
// {
//     std::string lobby_id = qobject_cast<QAction*>(sender())->data().toString().toStdString();
// 
//     if (lobby_id.empty())
//         return;
// 
//     std::string vpeer_id;
// 
//     if (rsMsgs->getVirtualPeerId(ChatLobbyId(QString::fromStdString(lobby_id).toULongLong()), vpeer_id))
//         ChatDialog::chatFriend(vpeer_id);
// }
// 
// void FriendList::unsubscribeToLobby()
// {
//     std::string lobby_id = qobject_cast<QAction*>(sender())->data().toString().toStdString();
// 
//     if (lobby_id.empty())
//         return;
// 
//     std::string vpeer_id ;
//     rsMsgs->getVirtualPeerId (ChatLobbyId(QString::fromStdString(lobby_id).toULongLong()), vpeer_id);
// 
//     if (QMessageBox::Ok == QMessageBox::question(this,tr("Unsubscribe to lobby"),tr("You are about to unsubscribe a chat lobby<br>You can only re-enter if your friends invite you again."),QMessageBox::Ok | QMessageBox::Cancel))
//         rsMsgs->unsubscribeChatLobby(ChatLobbyId(QString::fromStdString(lobby_id).toULongLong())) ;
// 
//     // we should also close existing windows.
//     ChatDialog::closeChat(vpeer_id);
// }

void FriendList::getSslIdsFromItem(QTreeWidgetItem *item, std::list<RsPeerId> &sslIds)
{
    if (item == NULL) {
        return;
    }

    std::string peerId = getRsId(item);

    switch (item->type()) {
    case TYPE_SSL:
        sslIds.push_back(RsPeerId(peerId));
        break;
    case TYPE_GPG:
        rsPeers->getAssociatedSSLIds(RsPgpId(peerId), sslIds);
        break;
    case TYPE_GROUP:
        {
            RsGroupInfo groupInfo;
            if (rsPeers->getGroupInfo(RsNodeGroupId(peerId), groupInfo)) {
                std::set<RsPgpId>::iterator gpgIt;
                for (gpgIt = groupInfo.peerIds.begin(); gpgIt != groupInfo.peerIds.end(); ++gpgIt) {
                    rsPeers->getAssociatedSSLIds(*gpgIt, sslIds);
                }
            }
        }
        break;
    }
}

void FriendList::addToGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId ( getRsId(c));

    if (gpgId.isNull() || groupId.isNull()) {
        return;
    }

    // automatically expand the group, the peer is added to
    addGroupToExpand(groupId);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
}

void FriendList::moveToGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId ( getRsId(c));

    if (gpgId.isNull() || groupId.isNull()) {
        return;
    }

    // remove from all groups
    rsPeers->assignPeerToGroup(RsNodeGroupId(), gpgId, false);

    // automatically expand the group, the peer is added to
    addGroupToExpand(groupId);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
}

void FriendList::removeFromGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId ( getRsId(c));

    if (gpgId.isNull()) {
        return;
    }

    // remove from (all) group(s)
    rsPeers->assignPeerToGroup(groupId, gpgId, false);
}

void FriendList::editGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GROUP) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( getRsId(c));

    if (!groupId.isNull())
    {
        CreateGroup editGrpDialog(groupId, this);
        editGrpDialog.exec();
    }
}

void FriendList::removeGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GROUP) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( getRsId(c));

    if (!groupId.isNull())
        rsPeers->removeGroup(groupId);
}

void FriendList::exportFriendlistClicked()
{
    QString fileName;
    if(!importExportFriendlistFileDialog(fileName, false))
        // error was already shown - just return
        return;

    if(!exportFriendlist(fileName))
        // error was already shown - just return
        return;

    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Information);
    mbox.setText(tr("Done!"));
    mbox.setInformativeText(tr("Your friendlist is stored at:\n") + fileName +
                            tr("\n(keep in mind that the file is unencrypted!)"));
    mbox.setStandardButtons(QMessageBox::Ok);
    mbox.exec();
}

void FriendList::importFriendlistClicked()
{
    QString fileName;
    if(!importExportFriendlistFileDialog(fileName, true))
        // error was already shown - just return
        return;

    bool errorPeers, errorGroups;
    if(importFriendlist(fileName, errorPeers, errorGroups)) {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Information);
        mbox.setText(tr("Done!"));
        mbox.setInformativeText(tr("Your friendlist was imported from:\n") + fileName);
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
    } else {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setText(tr("Done - but errors happened!"));
        mbox.setInformativeText(tr("Your friendlist was imported from:\n") + fileName +
                                (errorPeers  ? tr("\nat least one peer was not added") : "") +
                                (errorGroups ? tr("\nat least one peer was not added to a group") : "")
                                );
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
    }
}

/**
 * @brief opens a file dialog to select a file containing a friendlist
 * @param fileName file containing a friendlist
 * @param import show dialog for importing (true) or exporting (false) friendlist
 * @return success or failure
 *
 * This function also shows an error message when no valid file was selected
 */
bool FriendList::importExportFriendlistFileDialog(QString &fileName, bool import)
{
	bool res = true;
	if (import) {
		res = misc::getOpenFileName(this, RshareSettings::LASTDIR_CERT
		                            , tr("Select file for importing your friendlist from")
		                            , tr("XML File (*.xml);;All Files (*)")
		                            , fileName
		                            , QFileDialog::DontConfirmOverwrite
		                            );
		} else {
		res = misc::getSaveFileName(this, RshareSettings::LASTDIR_CERT
		                            , tr("Select a file for exporting your friendlist to")
		                            , tr("XML File (*.xml);;All Files (*)")
		                            , fileName, NULL
		                            , (QFileDialog::Options)0
		                            );
	}
	if ( res && !fileName.endsWith(".xml",Qt::CaseInsensitive) )
		fileName = fileName.append(".xml");

	return res;
}

/**
 * @brief exports friendlist to a given file
 * @param fileName file for storing friendlist
 * @return success or failure
 *
 * This function also shows an error message when the selected file is invalid/not writable
 */
bool FriendList::exportFriendlist(QString &fileName)
{
    QDomDocument doc("FriendListWithGroups");
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        // show error to user
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setText(tr("Error"));
        mbox.setInformativeText(tr("File is not writeable!\n") + fileName);
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
        return false;
    }

    std::list<RsPgpId> gpg_ids;
    rsPeers->getGPGAcceptedList(gpg_ids);

    std::list<RsGroupInfo> group_info_list;
    rsPeers->getGroupInfoList(group_info_list);

    QDomElement pgpIDs = doc.createElement("pgpIDs");
    RsPeerDetails detailPGP;
    for(std::list<RsPgpId>::iterator list_iter = gpg_ids.begin(); list_iter !=  gpg_ids.end(); list_iter++)	{
        rsPeers->getGPGDetails(*list_iter, detailPGP);
        QDomElement pgpID = doc.createElement("pgpID");
        // these values aren't used and just stored for better human readability
        pgpID.setAttribute("id", QString::fromStdString(detailPGP.gpg_id.toStdString()));
        pgpID.setAttribute("name", QString::fromUtf8(detailPGP.name.c_str()));

        std::list<RsPeerId> ssl_ids;
        rsPeers->getAssociatedSSLIds(*list_iter, ssl_ids);
        for(std::list<RsPeerId>::iterator list_iter = ssl_ids.begin(); list_iter !=  ssl_ids.end(); list_iter++) {
            RsPeerDetails detailSSL;
            if (!rsPeers->getPeerDetails(*list_iter, detailSSL))
                continue;

            std::string certificate = rsPeers->GetRetroshareInvite(detailSSL.id, true,true);
            // remove \n from certificate
            certificate.erase(std::remove(certificate.begin(), certificate.end(), '\n'), certificate.end());

            QDomElement sslID = doc.createElement("sslID");
            // these values aren't used and just stored for better human readability
            sslID.setAttribute("sslID", QString::fromStdString(detailSSL.id.toStdString()));
            if(!detailSSL.location.empty())
                sslID.setAttribute("location", QString::fromUtf8(detailSSL.location.c_str()));

            // required values
            sslID.setAttribute("certificate", QString::fromStdString(certificate));
            sslID.setAttribute("service_perm_flags", detailSSL.service_perm_flags.toUInt32());

            pgpID.appendChild(sslID);
        }
        pgpIDs.appendChild(pgpID);
    }
    root.appendChild(pgpIDs);

    QDomElement groups = doc.createElement("groups");
    for(std::list<RsGroupInfo>::iterator list_iter = group_info_list.begin(); list_iter !=  group_info_list.end(); list_iter++)	{
        RsGroupInfo group_info = *list_iter;

        //skip groups without peers
        if(group_info.peerIds.empty())
            continue;

        QDomElement group = doc.createElement("group");
        // id is not needed since it may differ between locatiosn / pgp ids (groups are identified by name)
        group.setAttribute("name", QString::fromUtf8(group_info.name.c_str()));
        group.setAttribute("flag", group_info.flag);

        for(std::set<RsPgpId>::iterator i = group_info.peerIds.begin(); i !=  group_info.peerIds.end(); i++) {
            QDomElement pgpID = doc.createElement("pgpID");
            std::string pid = i->toStdString();
            pgpID.setAttribute("id", QString::fromStdString(pid));
            group.appendChild(pgpID);
        }
        groups.appendChild(group);
    }
    root.appendChild(groups);

    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ts << doc.toString();
    file.close();

    return true;
}

/**
 * @brief helper function to show a message box
 */
static void showXMLParsingError()
{
    // show error to user
    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Warning);
    mbox.setText(QObject::tr("Error"));
    mbox.setInformativeText(QObject::tr("unable to parse XML file!"));
    mbox.setStandardButtons(QMessageBox::Ok);
    mbox.exec();
}

/**
 * @brief Imports friends from a given file
 * @param fileName file to load friends from
 * @param errorPeers an error occured while adding a peer
 * @param errorGroups an error occured while adding a peer to a group
 * @return success or failure (an error can also happen when adding a peer and/or adding a peer to a group fails at least once)
 */
bool FriendList::importFriendlist(QString &fileName, bool &errorPeers, bool &errorGroups)
{
    QDomDocument doc;
    // load from file
    {
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly)) {
            // show error to user
            QMessageBox mbox;
            mbox.setIcon(QMessageBox::Warning);
            mbox.setText(tr("Error"));
            mbox.setInformativeText(tr("File is not readable!\n") + fileName);
            mbox.setStandardButtons(QMessageBox::Ok);
            mbox.exec();
            return false;
        }

        bool ok = doc.setContent(&file);
        file.close();

        if(!ok) {
            showXMLParsingError();
            return false;
        }
    }

    QDomElement root = doc.documentElement();
    if(root.tagName() != "root") {
        showXMLParsingError();
        return false;
    }

    errorPeers = false;
    errorGroups = false;

    std::string error_string;
    RsPeerDetails rsPeerDetails;
    RsPeerId rsPeerID;
    RsPgpId rsPgpID;

    // lock all events for faster processing
    RsAutoUpdatePage::lockAllEvents();

    // pgp and ssl IDs
    QDomElement pgpIDs;
    {
        QDomNodeList nodes = root.elementsByTagName("pgpIDs");
        if(nodes.isEmpty() || nodes.size() != 1){
            showXMLParsingError();
            return false;
        }

        pgpIDs = nodes.item(0).toElement();
        if(pgpIDs.isNull()){
            showXMLParsingError();
            return false;
        }
    }
    QDomNode pgpIDElem = pgpIDs.firstChildElement("pgpID");
    while (!pgpIDElem.isNull()) {
        QDomElement sslIDElem = pgpIDElem.firstChildElement("sslID");
        while (!sslIDElem.isNull()) {
            rsPeerID.clear();
            rsPgpID.clear();

            // load everything needed from the pubkey string
            std::string pubkey = sslIDElem.attribute("certificate").toStdString();
			ServicePermissionFlags service_perm_flags(sslIDElem.attribute("service_perm_flags").toInt());
			if (!rsPeers->acceptInvite(pubkey, service_perm_flags)) {
                errorPeers = true;
				std::cerr << "FriendList::importFriendlist(): failed to get peer detaisl from public key (SSL id: " << sslIDElem.attribute("sslID", "invalid").toStdString() << ")" << std::endl;
            }
            sslIDElem = sslIDElem.nextSiblingElement("sslID");
        }
        pgpIDElem = pgpIDElem.nextSiblingElement("pgpID");
    }

    // groups
    QDomElement groups;
    {
        QDomNodeList nodes = root.elementsByTagName("groups");
        if(nodes.isEmpty() || nodes.size() != 1){
            showXMLParsingError();
            return false;
        }

        groups = nodes.item(0).toElement();
        if(groups.isNull()){
            showXMLParsingError();
            return false;
        }
    }
    QDomElement group = groups.firstChildElement("group");
    while (!group.isNull()) {
        // get name and flags and try to get the group ID
        std::string groupName = group.attribute("name").toStdString();
        uint32_t flag = group.attribute("flag").toInt();
        RsNodeGroupId groupId;
        if(getOrCreateGroup(groupName, flag, groupId)) {
            // group id found!
            QDomElement pgpID = group.firstChildElement("pgpID");
            while (!pgpID.isNull()) {
                // add pgp id to group
                RsPgpId rsPgpId(pgpID.attribute("id").toStdString());
                if(rsPgpID.isNull() || !rsPeers->assignPeerToGroup(groupId, rsPgpId, true)) {
                    errorGroups = true;
                    std::cerr << "FriendList::importFriendlist(): failed to add '" << rsPeers->getGPGName(rsPgpId) << "'' to group '" << groupName << "'" << std::endl;
                }

                pgpID = pgpID.nextSiblingElement("pgpID");
            }
            pgpID = pgpID.nextSiblingElement("pgpID");
        } else {
            errorGroups = true;
            std::cerr << "FriendList::importFriendlist(): failed to find/create group '" << groupName << "'" << std::endl;
        }
        group = group.nextSiblingElement("group");
    }

    // unlock events
    RsAutoUpdatePage::unlockAllEvents();

    return !(errorPeers || errorGroups);
}

/**
 * @brief Gets the groups ID for a given group name
 * @param name group name to search for
 * @param id groupd id for the given name
 * @return success or fail
 */
bool FriendList::getGroupIdByName(const std::string &name, RsNodeGroupId &id)
{
    std::list<RsGroupInfo> grpList;
    if(!rsPeers->getGroupInfoList(grpList))
        return false;

    foreach (const RsGroupInfo &grp, grpList) {
        if(grp.name == name) {
            id = grp.id;
            return true;
        }
    }

    return false;
}

/**
 * @brief Gets the groups ID for a given group name. If no groupd was it will create one
 * @param name group name to search for
 * @param flag flag to use when creating the group
 * @param id groupd id
 * @return success or failure
 */
bool FriendList::getOrCreateGroup(const std::string &name, const uint &flag, RsNodeGroupId &id)
{
    if(getGroupIdByName(name, id))
        return true;

    // -> create one
    RsGroupInfo grp;
    grp.id.clear(); // RS will generate an ID
    grp.name = name;
    grp.flag = flag;

    if(!rsPeers->addGroup(grp))
        return false;

    // try again
    return getGroupIdByName(name, id);
}


void FriendList::setHideUnconnected(bool hidden)
{
    if (mHideUnconnected != hidden) {
        mHideUnconnected = hidden;
        insertPeers();
    }
}

void FriendList::setColumnVisible(Column column, bool visible)
{
    ui->peerTreeWidget->setColumnHidden(column, !visible);
    peerTreeColumnVisibleChanged(column, visible);
}

void FriendList::sortByColumn(Column column, Qt::SortOrder sortOrder)
{
    ui->peerTreeWidget->sortByColumn(column, sortOrder);
}

void FriendList::peerTreeColumnVisibleChanged(int /*column*/, bool visible)
{
    if (visible) {
        insertPeers();
    }
}

void FriendList::peerTreeItemCollapsedExpanded(QTreeWidgetItem *item)
{
    if (!item) {
        return;
    }

    if (item->type() == TYPE_GPG) {
        insertPeers();
    }
}

void FriendList::setShowState(bool show)
{
    if (mShowState != show) {
        mShowState = show;
        insertPeers();
    }
}

void FriendList::sortByState(bool sort)
{
    int columnCount = ui->peerTreeWidget->columnCount();
    for (int i = 0; i < columnCount; ++i) {
        mCompareRole->setRole(i, ROLE_SORT_GROUP);
        mCompareRole->addRole(i, ROLE_SORT_STANDARD_GROUP);

        if (sort) {
            mCompareRole->addRole(i, ROLE_SORT_STATE);
            mCompareRole->addRole(i, ROLE_SORT_NAME);
        } else {
            mCompareRole->addRole(i, ROLE_SORT_NAME);
            mCompareRole->addRole(i, ROLE_SORT_STATE);
        }
    }

    mActionSortByState->setChecked(sort);

    ui->peerTreeWidget->resort();
}

bool FriendList::isSortByState()
{
    return mActionSortByState->isChecked();
}

void FriendList::setShowGroups(bool show)
{
    if (mShowGroups != show) {
        mShowGroups = show;
        if (mShowGroups) {
            // remove all not assigned gpg ids
            int childCount = ui->peerTreeWidget->topLevelItemCount();
            int childIndex = 0;
            while (childIndex < childCount) {
                QTreeWidgetItem *item = ui->peerTreeWidget->topLevelItem(childIndex);
                if (item->type() == TYPE_GPG) {
                    delete(ui->peerTreeWidget->takeTopLevelItem(childIndex));
                    childCount = ui->peerTreeWidget->topLevelItemCount();
                    continue;
                }
                ++childIndex;
            }
        }
        insertPeers();
    }
}

/**
 * Hides all items that don't contain text in the name column.
 */
void FriendList::filterItems(const QString &text)
{
	int filterColumn = ui->filterLineEdit->currentFilter();
	mFilterText = text;
	ui->peerTreeWidget->filterItems(filterColumn, mFilterText, ROLE_FILTER);
}

/**
 * Add a groupId to the openGroups list. These groups
 * will be expanded, when they're added to the QTreeWidget
 */
void FriendList::addGroupToExpand(const RsNodeGroupId &groupId)
{
    openGroups.insert(groupId);
}

/**
 * Add a gpgId to the openPeers list. These peers
 * will be expanded, when they're added to the QTreeWidget
 */
void FriendList::addPeerToExpand(const RsPgpId& gpgId)
{
    openPeers.insert(gpgId);
}

void FriendList::createDisplayMenu()
{
    QMenu *displayMenu = new QMenu(tr("Show Items"), this);
    connect(displayMenu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));

    displayMenu->addAction(ui->actionHideOfflineFriends);
    displayMenu->addAction(ui->actionShowState);
    displayMenu->addAction(ui->actionShowGroups);

    ui->peerTreeWidget->addContextMenuMenu(displayMenu);
    ui->peerTreeWidget->addContextMenuAction(ui->actionExportFriendlist);
    ui->peerTreeWidget->addContextMenuAction(ui->actionImportFriendlist);
}

void FriendList::updateMenu()
{
    ui->actionHideOfflineFriends->setChecked(mHideUnconnected);
    ui->actionShowState->setChecked(mShowState);
    ui->actionShowGroups->setChecked(mShowGroups);
}
