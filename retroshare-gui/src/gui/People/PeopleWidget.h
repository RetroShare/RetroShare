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

#include "ui_PeopleWidget.h"

#define IMAGE_IDENTITY          ":/icons/friends_128.png"

class PeopleWidget : public RsGxsUpdateBroadcastPage, public Ui::PeopleWidget, public TokenResponse
{
	Q_OBJECT

	public:
	static const uint32_t PD_IDLIST    ;
	static const uint32_t PD_IDDETAILS ;
	static const uint32_t PD_REFRESH   ;
	static const uint32_t PD_CIRCLES   ;

		PeopleWidget(QWidget *parent = 0);
		~PeopleWidget();

		virtual QIcon iconPixmap() const { return QIcon(IMAGE_IDENTITY) ; } //MainPage
		virtual QString pageName() const { return tr("People") ; } //MainPage
		virtual QString helpText() const { return ""; } //MainPage

	void loadRequest(const TokenQueue * /*queue*/, const TokenRequest &req) ;

	void requestIdList() ;

	void insertIdList(uint32_t token) ;

	protected:
	// Derives from RsGxsUpdateBroadcastPage
		virtual void updateDisplay(bool complete);
	//End RsGxsUpdateBroadcastPage

private slots:
	void iw_AddButtonClicked();
	void pf_centerIndexChanged(int index);
	void pf_mouseMoveOverSlideEvent(QMouseEvent* event, int slideIndex);
	void pf_dragEnterEventOccurs(QDragEnterEvent *event);
	void pf_dragMoveEventOccurs(QDragMoveEvent *event);
	
	void chatIdentity();
  void sendMessage();
  void personDetails();

private:
	void reloadAll();
	void populatePictureFlowExt();
	void populatePictureFlowInt();

	TokenQueue *mIdentityQueue;

	FlowLayout *_flowLayoutExt;
	std::map<RsGxsId,IdentityWidget *> _gxs_identity_widgets ;

};

