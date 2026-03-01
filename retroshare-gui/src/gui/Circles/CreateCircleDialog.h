/*******************************************************************************
 * retroshare-gui/src/gui/Circles/CreateCirclesDialog.h                        *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2012, Robert Fernie <retroshare.project@gmail.com>            *
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

#ifndef MRK_CREATE_CIRCLE_DIALOG_H
#define MRK_CREATE_CIRCLE_DIALOG_H

#include "ui_CreateCircleDialog.h"

#include <retroshare/rsgxscircles.h>
#include <QDialog>

class CreateCircleDialog : public QDialog
{
	Q_OBJECT

public:
	CreateCircleDialog();
	~CreateCircleDialog();

	void editNewId(bool isExternal);
	void editExistingId(const RsGxsGroupId &circleId, const bool &clearList = true, bool readonly=true);
	void addMember(const QString &keyId, const QString &idtype, const QString &nickname, const QIcon &icon);
	void addMember(const QString &keyId, const QString &idtype, const QString &nickname);
	void addMember(const RsGxsIdGroup &idGroup);
	void addCircle(const RsGxsCircleDetails &cirDetails);
	void addMember(const QString &keyId, const QString &idtype, const QString &nickname, 
                const QIcon &icon, bool invited, bool subscrb, bool is_own_id);

private slots:

    void addMember();
	void removeMember();

    void acceptInvite();
    void rejectInvite();

    void grantCircleMembership();
    void revokeCircleMembership();

	void updateCircleType(bool b);
	void selectedId(QTreeWidgetItem*, QTreeWidgetItem*);
	void selectedMember(QTreeWidgetItem*, QTreeWidgetItem*);

	void createCircle();
	void filterChanged(const QString &text);
	void createNewGxsId();
	void idTypeChanged();
	void updateMembership();
	
	/** Create the context popup menu and it's submenus */
	void IdListCustomPopupMenu( QPoint point );
	void MembershipListCustomPopupMenu( QPoint point);

protected:
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void accept() override;
    virtual void reject() override;

    bool tryClose();
private:

	void updateCircleGUI();

	void setupForPersonalCircle();
	void setupForExternalCircle();

	bool mIsExistingCircle;
	bool mIsExternalCircle;
    bool mReadOnly;
    bool mIdentitiesLoading;
    bool mCircleLoading;
    bool mCloseRequested;

	void loadCircle(const RsGxsGroupId& groupId);
	void loadIdentities();
	
	void filterIds();
	void fillIdentitiesList(const std::vector<RsGxsIdGroup>& id_groups);

	RsGxsCircleGroup mCircleGroup; // for editting existing Circles.
	bool mClearList;

    bool am_I_invited; // Tracks if the current user has an invitation to this circle
    bool am_I_pending; // Tracks if the current user's subscription is pending
    bool am_I_circle_admin; // Track admin status

    void updateMemberStatus(QTreeWidgetItem* item, bool invited, bool subscrb, bool is_own_id);

	/** Qt Designer generated object */
	Ui::CreateCircleDialog ui;
};

#endif
