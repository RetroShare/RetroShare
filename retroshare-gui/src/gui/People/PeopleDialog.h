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

#pragma once

#include <map>

#include <retroshare/rsidentity.h>

#include "gui/People/CircleWidget.h"
#include "gui/People/IdentityWidget.h"
#include "gui/gxs/RsGxsUpdateBroadcastPage.h"
#include "util/TokenQueue.h"

#include "ui_PeopleDialog.h"

#define IMAGE_IDENTITY          ":/images/identity/identities_32.png"

class UIStateHelper;
//class IdentityItem ;
//class CircleItem ;

class PeopleDialog : public RsGxsUpdateBroadcastPage, public Ui::PeopleDialog, public TokenResponse
{
	Q_OBJECT

	public:
	static const uint32_t PD_IDLIST    ;
	static const uint32_t PD_IDDETAILS ;
	static const uint32_t PD_REFRESH   ;
	static const uint32_t PD_CIRCLES   ;

		PeopleDialog(QWidget *parent = 0);
		~PeopleDialog();

		virtual QIcon iconPixmap() const { return QIcon(IMAGE_IDENTITY) ; } //MainPage
		virtual QString pageName() const { return tr("People") ; } //MainPage
		virtual QString helpText() const { return ""; } //MainPage

	// Derives from RsGxsUpdateBroadcastPage
//	virtual void updateDisplay(bool) ;
	void loadRequest(const TokenQueue * /*queue*/, const TokenRequest &req) ;

	void requestIdList() ;
	void requestCirclesList() ;

	void insertIdList(uint32_t token) ;
	void insertCircles(uint32_t token) ;

	protected:
		virtual void updateDisplay(bool complete);

private slots:
	void cw_askForGXSIdentityWidget(RsGxsId gxs_id);
	void cw_askForPGPIdentityWidget(RsPgpId pgp_id);
	void fl_flowLayoutItemDropped(QList<FlowLayoutItem *> flListItem, bool &bAccept);
	void pf_centerIndexChanged(int index);
	void pf_mouseMoveOverSlideEvent(QMouseEvent* event, int slideIndex);
	void pf_dragEnterEventOccurs(QDragEnterEvent *event);
	void pf_dragMoveEventOccurs(QDragMoveEvent *event);
	void pf_dropEventOccurs(QDropEvent *event);

private:
	void populatePictureFlow();

	TokenQueue *mIdentityQueue;
	TokenQueue *mCirclesQueue;
	UIStateHelper *mStateHelper;

	FlowLayout *_flowLayout;
	std::map<RsPgpId,IdentityWidget *> _pgp_identity_widgets ;
	std::map<RsGxsId,IdentityWidget *> _gxs_identity_widgets ;
	std::map<RsGxsGroupId,CircleWidget *> _circles_widgets ;
	//QList<IdentityWidget*> listId;
	QList<CircleWidget*> _listCir;
};

