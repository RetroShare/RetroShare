/*******************************************************************************
 * retroshare-gui/src/gui/Identity/IdDialog.h                                  *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#ifndef IDENTITYDIALOG_H
#define IDENTITYDIALOG_H

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"

#include "retroshare/rsidentity.h"
#include "IdentityListModel.h"

#include <QTimer>

#define IMAGE_IDDIALOG          ":/icons/png/people.png"

namespace Ui {
class IdDialog;
}

class UIStateHelper;
class QTreeWidgetItem;
class RsIdentityListModel;
class IdListSortFilterProxyModel;

class IdDialog : public MainPage
{
	Q_OBJECT

public:
	IdDialog(QWidget *parent = 0);
	~IdDialog();

	virtual QIcon iconPixmap() const override { return QIcon(IMAGE_IDDIALOG) ; } //MainPage
	virtual QString pageName() const override { return tr("People") ; } //MainPage
	virtual QString helpText() const override { return ""; } //MainPage

	void navigate(const RsGxsId& gxs_id) ; // shows the info about this particular ID
protected:
	virtual void updateDisplay(bool complete);

	void loadIdentities(const std::map<RsGxsGroupId, RsGxsIdGroup> &ids_set);

	void updateIdentity();
	void loadIdentity(RsGxsIdGroup id_data);

	void loadCircles(const std::list<RsGroupMetaData>& circle_metas);

	//void requestCircleGroupData(const RsGxsCircleId& circle_id);
	bool getItemCircleId(QTreeWidgetItem *item,RsGxsCircleId& id) ;

	virtual void showEvent(QShowEvent *) override;


private slots:
	void updateIdList();
	void updateCircles();
void trace_expanded(const QModelIndex&);
void trace_collapsed(const QModelIndex& i);
    void createExternalCircle();
	void showEditExistingCircle();
	void updateCirclesDisplay();
	void toggleAutoBanIdentities(bool b);

	void acceptCircleSubscription() ;
	void cancelCircleSubscription() ;
	void grantCircleMembership() ;
	void revokeCircleMembership() ;

	void filterChanged(const QString &text);
	void filterToggled(const bool &value);

	void addIdentity();
	void removeIdentity();
	void editIdentity();
	void chatIdentity();
    void chatIdentityItem(const QModelIndex &indx);
    void chatIdentity(const RsGxsId& toGxsId);
    void sendMsg();
	void copyRetroshareLink();
	void on_closeInfoFrameButton_Invite_clicked();

	void updateSelection();

	void modifyReputation();

	/** Create the context popup menu and it's submenus */
	void IdListCustomPopupMenu( QPoint point );
    void headerContextMenuRequested(QPoint);
    void toggleColumnVisible();

	void CircleListCustomPopupMenu(QPoint point) ;
#ifdef SUSPENDED
	void circle_selected() ;
#endif

	void  addtoContacts();
	void  removefromContacts();

	void negativePerson();
	void positivePerson();
	void neutralPerson();

	static QString inviteMessage();
	void sendInvite();

private:
	void processSettings(bool load);
	QString createUsageString(const RsIdentityUsage& u) const;

    void saveExpandedPathsAndSelection_idTreeView(std::set<QStringList> &expanded, std::set<QStringList> &selected);
    void restoreExpandedPathsAndSelection_idTreeView(const std::set<QStringList>& expanded, const std::set<QStringList>& selelected);
    void recursSaveExpandedItems_idTreeView(const QModelIndex& index, const QStringList& parent_path, std::set<QStringList>& expanded, std::set<QStringList>& selected);
    void recursRestoreExpandedItems_idTreeView(const QModelIndex& index,const QStringList& parent_path,const std::set<QStringList>& expanded,const std::set<QStringList>& selected);

    void requestIdData(std::list<RsGxsGroupId> &ids);
	bool fillIdListItem(const RsGxsIdGroup& data, QTreeWidgetItem *&item, const RsPgpId &ownPgpId, int accept);
	void insertIdList(uint32_t token);
	void filterIds();

	void requestRepList();
	void insertRepList(uint32_t token);
	void handleSerializedGroupData(uint32_t token);

	void requestIdEdit(std::string &id);
	void showIdEdit(uint32_t token);

	void clearPerson();

private:
	UIStateHelper *mStateHelper;

	QTreeWidgetItem *mExternalBelongingCircleItem;
	QTreeWidgetItem *mExternalOtherCircleItem;
	QTreeWidgetItem *mMyCircleItem;
	RsGxsUpdateBroadcastBase *mCirclesBroadcastBase ;

	void saveExpandedCircleItems(std::vector<bool> &expanded_root_items, std::set<RsGxsCircleId>& expanded_circle_items) const;
	void restoreExpandedCircleItems(const std::vector<bool>& expanded_root_items,const std::set<RsGxsCircleId>& expanded_circle_items);

    void applyWhileKeepingTree(std::function<void()> predicate);

    RsGxsId getSelectedIdentity() const;
    std::list<RsGxsId> getSelectedIdentities() const;

    void idListItemExpanded(const QModelIndex& index);
    void idListItemCollapsed(const QModelIndex& index);

    RsGxsGroupId mId;
	RsGxsGroupId mIdToNavigate;
	int filter;

    RsIdentityListModel *mIdListModel;
    IdListSortFilterProxyModel *mProxyModel;

	void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);
	RsEventsHandlerId_t mEventHandlerId_identity;
	RsEventsHandlerId_t mEventHandlerId_circles;

	QTimer updateIdTimer;
	bool needUpdateIdsOnNextShow;
	bool needUpdateCirclesOnNextShow;

	/* UI -  Designer */
	Ui::IdDialog *ui;
};

#endif
