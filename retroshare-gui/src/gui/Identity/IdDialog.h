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

#include <retroshare/rsidentity.h>

#define IMAGE_IDDIALOG          ":/icons/png/people.png"

namespace Ui {
class IdDialog;
}

class UIStateHelper;
class QTreeWidgetItem;

struct CircleUpdateOrder
{
    enum { UNKNOWN_ACTION=0x00, GRANT_MEMBERSHIP=0x01, REVOKE_MEMBERSHIP=0x02 };
         
    uint32_t token ;
    RsGxsId  gxs_id ;
    uint32_t action ;
};

class IdDialog : public MainPage
{
	Q_OBJECT

public:
	IdDialog(QWidget *parent = 0);
	~IdDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_IDDIALOG) ; } //MainPage
	virtual QString pageName() const { return tr("People") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

    void navigate(const RsGxsId& gxs_id) ; // shows the info about this particular ID
protected:
	virtual void updateDisplay(bool complete);

	void updateIdList();
	void loadIdentities(const std::map<RsGxsGroupId, RsGxsIdGroup> &ids_set);

	void updateIdentity();
	void loadIdentity(RsGxsIdGroup id_data);

	void updateCircles();
	void loadCircles(const std::list<RsGroupMetaData>& circle_metas);

	//void requestCircleGroupData(const RsGxsCircleId& circle_id);
	bool getItemCircleId(QTreeWidgetItem *item,RsGxsCircleId& id) ;

private slots:
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
	void chatIdentityItem(QTreeWidgetItem* item);
	void sendMsg();
	void copyRetroshareLink();
  void on_closeInfoFrameButton_clicked();

	void updateSelection();

	void modifyReputation();

	/** Create the context popup menu and it's submenus */
	void IdListCustomPopupMenu( QPoint point );

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

	QTreeWidgetItem *contactsItem;
	QTreeWidgetItem *allItem;
	QTreeWidgetItem *ownItem;
	QTreeWidgetItem *mExternalBelongingCircleItem;
	QTreeWidgetItem *mExternalOtherCircleItem;
	QTreeWidgetItem *mMyCircleItem;
	RsGxsUpdateBroadcastBase *mCirclesBroadcastBase ;

	std::map<uint32_t, CircleUpdateOrder> mCircleUpdates ;

	RsGxsGroupId mId;
	RsGxsGroupId mIdToNavigate;
	int filter;

    void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);
    RsEventsHandlerId_t mEventHandlerId_identity;
    RsEventsHandlerId_t mEventHandlerId_circles;

	/* UI -  Designer */
	Ui::IdDialog *ui;
};

#endif
