/*
 * Retroshare Identity.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef IDENTITYDIALOG_H
#define IDENTITYDIALOG_H

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"

#include <retroshare/rsidentity.h>

#include "util/TokenQueue.h"

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

class IdDialog : public RsGxsUpdateBroadcastPage, public TokenResponse
{
	Q_OBJECT

public:
	IdDialog(QWidget *parent = 0);
	~IdDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_IDDIALOG) ; } //MainPage
	virtual QString pageName() const { return tr("People") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	void loadRequest(const TokenQueue *queue, const TokenRequest &req);

    void navigate(const RsGxsId& gxs_id) ; // shows the info about this particular ID
protected:
	virtual void updateDisplay(bool complete);

	void loadCircleGroupMeta(const uint32_t &token);
	void loadCircleGroupData(const uint32_t &token);
	void updateCircleGroup(const uint32_t& token);

	void requestCircleGroupMeta();
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
	void sendMsg();
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

	void requestIdDetails();
	void insertIdDetails(uint32_t token);

	void requestIdList();
	void requestIdData(std::list<RsGxsGroupId> &ids);
	bool fillIdListItem(const RsGxsIdGroup& data, QTreeWidgetItem *&item, const RsPgpId &ownPgpId, int accept);
	void insertIdList(uint32_t token);
	void filterIds();

	void requestRepList();
	void insertRepList(uint32_t token);

	void requestIdEdit(std::string &id);
	void showIdEdit(uint32_t token);

private:
	TokenQueue *mIdQueue;
	TokenQueue *mCircleQueue;

	UIStateHelper *mStateHelper;

	QTreeWidgetItem *contactsItem;
	QTreeWidgetItem *allItem;
	QTreeWidgetItem *ownItem;
	QTreeWidgetItem *mExternalBelongingCircleItem;
	QTreeWidgetItem *mExternalOtherCircleItem;
	RsGxsUpdateBroadcastBase *mCirclesBroadcastBase ;

	std::map<uint32_t, CircleUpdateOrder> mCircleUpdates ;

	RsGxsGroupId mId;
	int filter;

	/* UI -  Designer */
	Ui::IdDialog *ui;
};

#endif
