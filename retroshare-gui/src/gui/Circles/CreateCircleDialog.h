/*
 * Retroshare Circles.
 *
 * Copyright 2012-2013 by Robert Fernie.
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

#ifndef MRK_CREATE_CIRCLE_DIALOG_H
#define MRK_CREATE_CIRCLE_DIALOG_H

#include "ui_CreateCircleDialog.h"

#include "util/TokenQueue.h"

#include <retroshare/rsgxscircles.h>
#include <QDialog>

class CreateCircleDialog : public QDialog, public TokenResponse
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

	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);
    
private slots:
	void addMember();
	void removeMember();

	void updateCircleType(bool b);
	void selectedId(QTreeWidgetItem*, QTreeWidgetItem*);
	void selectedMember(QTreeWidgetItem*, QTreeWidgetItem*);

	void createCircle();
	void filterChanged(const QString &text);
	void createNewGxsId();
	void idTypeChanged();
	
	/** Create the context popup menu and it's submenus */
	void IdListCustomPopupMenu( QPoint point );
	void MembershipListCustomPopupMenu( QPoint point);

private:

	void updateCircleGUI();

	void setupForPersonalCircle();
	void setupForExternalCircle();

	bool mIsExistingCircle;
	bool mIsExternalCircle;
    	bool mReadOnly;

	void loadCircle(uint32_t token);
	void loadIdentities(uint32_t token);

	void requestCircle(const RsGxsGroupId &groupId);
	void requestGxsIdentities();
	//void getPgpIdentities();
	
	void filterIds();

	TokenQueue *mCircleQueue;
	TokenQueue *mIdQueue;

	RsGxsCircleGroup mCircleGroup; // for editting existing Circles.
	bool mClearList;

	/** Qt Designer generated object */
	Ui::CreateCircleDialog ui;
};

#endif
