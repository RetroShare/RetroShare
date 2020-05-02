/*******************************************************************************
 * retroshare-gui/src/gui/People/IdentityWidget.h                              *
 *                                                                             *
 * Copyright (C) 2014 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#include "PeopleDialog.h"
#include "gui/Circles/CreateCircleDialog.h"
#include "gui/common/FlowLayout.h"
#include "gui/settings/rsharesettings.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsIdDetails.h"
//#include "gui/gxs/RsGxsUpdateBroadcastBase.h"
#include "gui/Identity/IdDetailsDialog.h"
#include "gui/Identity/IdDialog.h"
#include "gui/MainWindow.h"

#include "retroshare/rspeers.h"
#include "retroshare/rsidentity.h"
#include "retroshare/rsgxscircles.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsmsgs.h" 
#include "retroshare/rsids.h" 

#include <iostream>
#include <QMenu>
#include <QMessageBox>

/******
 * #define PEOPLE_DIALOG_DEBUG 1
 *****/


const uint32_t PeopleDialog::PD_IDLIST    = 0x0001 ;
const uint32_t PeopleDialog::PD_IDDETAILS = 0x0002 ;
const uint32_t PeopleDialog::PD_REFRESH   = 0x0003 ;
const uint32_t PeopleDialog::PD_CIRCLES   = 0x0004 ;

/** Constructor */
PeopleDialog::PeopleDialog(QWidget *parent)
	: MainPage(parent)
{
	setupUi(this);

	/* Setup TokenQueue */
	mIdentityQueue = new TokenQueue(rsIdentity->getTokenService(), this);
	mCirclesQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);
	// This is used to grab the broadcast of changes from p3GxsCircles, which is discarded by the current dialog, since it expects data for p3Identity only.
	//mCirclesBroadcastBase = new RsGxsUpdateBroadcastBase(rsGxsCircles, this);
	//connect(mCirclesBroadcastBase, SIGNAL(fillDisplay(bool)), this, SLOT(updateCirclesDisplay(bool)));

	
	tabWidget->removeTab(1);
	//hide circle flow widget not functional yet
	pictureFlowWidgetExternal->hide();

	//need erase QtCreator Layout first(for Win)
	delete idExternal->layout();
	delete idInternal->layout();
	//QT Designer don't accept Custom Layout, maybe on QT5
	_flowLayoutExt = new FlowLayout(idExternal);
	_flowLayoutInt = new FlowLayout(idInternal);

	{//First Get Item created in Qt Designer for External
		int count = idExternal->children().count();
		for (int curs = 0; curs < count; ++curs){
			QObject *obj = idExternal->children().at(curs);
			QWidget *wid = qobject_cast<QWidget *>(obj);
			if (wid) _flowLayoutExt->addWidget(wid);
		}//for (int curs = 0; curs < count; ++curs)
	}//End First Get Item created in Qt Designer for External

	{//First Get Item created in Qt Designer for Internal
		int count = idInternal->children().count();
		for (int curs = 0; curs < count; ++curs){
			QObject *obj = idInternal->children().at(curs);
			QWidget *wid = qobject_cast<QWidget *>(obj);
			if (wid) _flowLayoutInt->addWidget(wid);
		}//for (int curs = 0; curs < count; ++curs)
	}//End First Get Item created in Qt Designer for Internal

	pictureFlowWidgetExternal->setAcceptDrops(true);
	QObject::connect(pictureFlowWidgetExternal, SIGNAL(centerIndexChanged(int)), this, SLOT(pf_centerIndexChanged(int)));
	QObject::connect(pictureFlowWidgetExternal, SIGNAL(mouseMoveOverSlideEvent(QMouseEvent*,int)), this, SLOT(pf_mouseMoveOverSlideEvent(QMouseEvent*,int)));
	QObject::connect(pictureFlowWidgetExternal, SIGNAL(dragEnterEventOccurs(QDragEnterEvent*)), this, SLOT(pf_dragEnterEventOccurs(QDragEnterEvent*)));
	QObject::connect(pictureFlowWidgetExternal, SIGNAL(dragMoveEventOccurs(QDragMoveEvent*)), this, SLOT(pf_dragMoveEventOccurs(QDragMoveEvent*)));
	QObject::connect(pictureFlowWidgetExternal, SIGNAL(dropEventOccurs(QDropEvent*)), this, SLOT(pf_dropEventOccursExt(QDropEvent*)));
	pictureFlowWidgetExternal->setMinimumHeight(60);
	pictureFlowWidgetExternal->setSlideSizeRatio(4/4.0);

	pictureFlowWidgetInternal->setAcceptDrops(true);
	QObject::connect(pictureFlowWidgetInternal, SIGNAL(centerIndexChanged(int)), this, SLOT(pf_centerIndexChanged(int)));
	QObject::connect(pictureFlowWidgetInternal, SIGNAL(mouseMoveOverSlideEvent(QMouseEvent*,int)), this, SLOT(pf_mouseMoveOverSlideEvent(QMouseEvent*,int)));
	QObject::connect(pictureFlowWidgetInternal, SIGNAL(dragEnterEventOccurs(QDragEnterEvent*)), this, SLOT(pf_dragEnterEventOccurs(QDragEnterEvent*)));
	QObject::connect(pictureFlowWidgetInternal, SIGNAL(dragMoveEventOccurs(QDragMoveEvent*)), this, SLOT(pf_dragMoveEventOccurs(QDragMoveEvent*)));
	QObject::connect(pictureFlowWidgetInternal, SIGNAL(dropEventOccurs(QDropEvent*)), this, SLOT(pf_dropEventOccursInt(QDropEvent*)));
	pictureFlowWidgetInternal->setMinimumHeight(60);
	pictureFlowWidgetInternal->setSlideSizeRatio(4/4.0);

	QByteArray geometryExt = Settings->valueFromGroup("PeopleDialog", "SplitterExtState", QByteArray()).toByteArray();
	if (geometryExt.isEmpty() == false) {
		splitterExternal->restoreState(geometryExt);
	}
	QByteArray geometryInt = Settings->valueFromGroup("PeopleDialog", "SplitterIntState", QByteArray()).toByteArray();
	if (geometryInt.isEmpty() == false) {
		splitterInternal->restoreState(geometryInt);
	}

	reloadAll();

}

/** Destructor. */
PeopleDialog::~PeopleDialog()
{
	delete mIdentityQueue;
	delete mCirclesQueue;

	Settings->setValueToGroup("PeopleDialog", "SplitterExtState", splitterExternal->saveState());
	Settings->setValueToGroup("PeopleDialog", "SplitterIntState", splitterInternal->saveState());
}

void PeopleDialog::updateDisplay(bool complete)
{
	Q_UNUSED(complete);
	reloadAll();
}

void PeopleDialog::reloadAll()
{
	/* Update identity list */
	requestIdList();
	requestCirclesList();

	/* grab all ids */
	std::list<RsPgpId> friend_pgpIds;
	std::list<RsPgpId> all_pgpIds;
	std::list<RsPgpId>::iterator it;

	std::set<RsPgpId> friend_set;

	rsPeers->getGPGAcceptedList(friend_pgpIds);
	//rsPeers->getGPGAllList(all_pgpIds);


	for(it = friend_pgpIds.begin(); it != friend_pgpIds.end(); ++it) {
		RsPeerDetails details;
		if(rsPeers->getGPGDetails(*it, details)){
			std::map<RsPgpId,IdentityWidget *>::iterator itFound;
			if((itFound=_pgp_identity_widgets.find(*it)) == _pgp_identity_widgets.end()) {
				IdentityWidget *new_item = new IdentityWidget();
				new_item->updateData(details) ;
				_pgp_identity_widgets[*it] = new_item ;

				QObject::connect(new_item, SIGNAL(addButtonClicked()), this, SLOT(iw_AddButtonClickedInt()));
				QObject::connect(new_item, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedInt(QList<FlowLayoutItem*>,bool&)));
				_flowLayoutInt->addWidget(new_item);
			} else {//if((itFound=_pgp_identity_widgets.find(gdItem.mPgpId)) == _pgp_identity_widgets.end())
				IdentityWidget *idWidget = itFound->second;

				idWidget->updateData(details) ;
			}//else ((itFound=_pgp_identity_widgets.find(gdItem.mPgpId)) == _pgp_identity_widgets.end())

		friend_set.insert(*it);
		}//if(rsPeers->getGPGDetails(*it, details))
	}//for(it = friend_pgpIds.begin(); it != friend_pgpIds.end(); ++it)

	for(it = all_pgpIds.begin(); it != all_pgpIds.end(); ++it) {
		if(friend_set.end() != friend_set.find(*it)) {
			// already added as a friend.
			continue;
		}//if(friend_set.end() != friend_set.find(*it))

		RsPeerDetails details;
		if (rsPeers->getGPGDetails(*it, details)) {
			std::map<RsPgpId,IdentityWidget *>::iterator itFound;
			if((itFound=_pgp_identity_widgets.find(*it)) == _pgp_identity_widgets.end()) {
				IdentityWidget *new_item = new IdentityWidget();
				new_item->updateData(details) ;
				_pgp_identity_widgets[*it] = new_item ;

				QObject::connect(new_item, SIGNAL(addButtonClicked()), this, SLOT(iw_AddButtonClickedInt()));
				QObject::connect(new_item, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedInt(QList<FlowLayoutItem*>,bool&)));
				_flowLayoutInt->addWidget(new_item);
			} else {//if((itFound=_pgp_identity_widgets.find(gdItem.mPgpId)) == _pgp_identity_widgets.end())
				IdentityWidget *idWidget = itFound->second;

				idWidget->updateData(details) ;
			}//else ((itFound=_pgp_identity_widgets.find(gdItem.mPgpId)) == _pgp_identity_widgets.end())
		}//if(rsPeers->getGPGDetails(*it, details))
	}//for(it = all_pgpIds.begin(); it != all_pgpIds.end(); ++it)
}

void PeopleDialog::insertIdList(uint32_t token)
{
	std::cerr << "**** In insertIdList() ****" << std::endl;

	std::vector<RsGxsIdGroup> gdataVector;
	std::vector<RsGxsIdGroup>::iterator gdIt;

	if (!rsIdentity->getGroupData(token, gdataVector)) {
		std::cerr << "PeopleDialog::insertIdList() Error getting GroupData";
		std::cerr << std::endl;

		return;
	}//if (!rsIdentity->getGroupData(token, gdataVector))

	//RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	/* Insert items */
	int i=0 ;
	for (gdIt = gdataVector.begin(); gdIt != gdataVector.end(); ++gdIt){
		RsGxsIdGroup gdItem = (*gdIt);
		bool bGotDetail = false;

		RsPeerDetails details;
		if (gdItem.mPgpKnown) {
			bGotDetail = rsPeers->getGPGDetails(gdItem.mPgpId, details);
		}//if (gdItem.mPgpKnown)

		std::map<RsGxsId,IdentityWidget *>::iterator itFound;
		if((itFound=_gxs_identity_widgets.find(RsGxsId(gdItem.mMeta.mGroupId))) == _gxs_identity_widgets.end()) {
			std::cerr << "Loading data vector identity GXS ID = " << gdItem.mMeta.mGroupId << ", i="<< i << std::endl;

			IdentityWidget *new_item = new IdentityWidget();
			if (bGotDetail) {
				new_item->updateData(gdItem, details);
			} else {//if (bGotDetail)
				new_item->updateData(gdItem);
			}//else (bGotDetail)
			_gxs_identity_widgets[RsGxsId(gdItem.mMeta.mGroupId)] = new_item ;

			QObject::connect(new_item, SIGNAL(addButtonClicked()), this, SLOT(iw_AddButtonClickedExt()));
			QObject::connect(new_item, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedExt(QList<FlowLayoutItem*>,bool&)));
			_flowLayoutExt->addWidget(new_item);
			++i ;
		} else {//if((itFound=_gxs_identity_widgets.find(RsGxsId(gdItem.mMeta.mGroupId))) == _gxs_identity_widgets.end())

			std::cerr << "Updating data vector identity GXS ID = " << gdItem.mMeta.mGroupId << std::endl;
			IdentityWidget *idWidget = itFound->second;

			if (bGotDetail) {
				idWidget->updateData(gdItem, details) ;
			} else {//if (bGotDetail)
				idWidget->updateData(gdItem) ;
			}//else (bGotDetail)

		}//else ((itFound=_gxs_identity_widgets.find(RsGxsId(gdItem.mMeta.mGroupId))) == _gxs_identity_widgets.end()))
	}//for (gdIt = gdataVector.begin(); gdIt != gdataVector.end(); ++gdIt)
}

void PeopleDialog::insertCircles(uint32_t token)
{
	std::cerr << "PeopleDialog::insertExtCircles(token==" << token << ")" << std::endl;

	std::list<RsGroupMetaData> gSummaryList;
	std::list<RsGroupMetaData>::iterator gsIt;

	if (!rsGxsCircles->getGroupSummary(token,gSummaryList))
    {
		std::cerr << "PeopleDialog::insertExtCircles() Error getting GroupSummary";
		std::cerr << std::endl;

		return;
	}

	for(gsIt = gSummaryList.begin(); gsIt != gSummaryList.end(); ++gsIt) {
		RsGroupMetaData gsItem = (*gsIt);

		RsGxsCircleDetails details ;
		if(!rsGxsCircles->getCircleDetails(RsGxsCircleId(gsItem.mGroupId), details))
        {
			std::cerr << "(EE) Cannot get details for circle id " << gsItem.mGroupId << ". Circle item is not created!" << std::endl;
			continue ;
		}

		if (details.mCircleType != RsGxsCircleType::EXTERNAL)
        {
			std::map<RsGxsGroupId, CircleWidget*>::iterator itFound;
			if((itFound=_int_circles_widgets.find(gsItem.mGroupId)) == _int_circles_widgets.end())
            {
				std::cerr << "PeopleDialog::insertExtCircles() add new Internal GroupId: " << gsItem.mGroupId;
				std::cerr << " GroupName: " << gsItem.mGroupName;
				std::cerr << std::endl;

				CircleWidget *gitem = new CircleWidget() ;
				QObject::connect(gitem, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedInt(QList<FlowLayoutItem*>,bool&)));
				QObject::connect(gitem, SIGNAL(askForGXSIdentityWidget(RsGxsId)), this, SLOT(cw_askForGXSIdentityWidget(RsGxsId)));
				QObject::connect(gitem, SIGNAL(askForPGPIdentityWidget(RsPgpId)), this, SLOT(cw_askForPGPIdentityWidget(RsPgpId)));
				QObject::connect(gitem, SIGNAL(imageUpdated()), this, SLOT(cw_imageUpdatedInt()));
				gitem->updateData( gsItem, details );
				_int_circles_widgets[gsItem.mGroupId] = gitem ;

				_flowLayoutInt->addWidget(gitem);

				QPixmap pixmap = gitem->getImage();
				pictureFlowWidgetInternal->addSlide( pixmap );
				_intListCir << gitem;
			}
            else
            {
				std::cerr << "PeopleDialog::insertExtCircles() Update GroupId: " << gsItem.mGroupId;
				std::cerr << " GroupName: " << gsItem.mGroupName;
				std::cerr << std::endl;

				CircleWidget *cirWidget = itFound->second;
				cirWidget->updateData( gsItem, details );

				//int index = _intListCir.indexOf(cirWidget);
				//QPixmap pixmap = cirWidget->getImage();
				//pictureFlowWidgetInternal->setSlide(index, pixmap);
			}
		}
        else
        {
			std::map<RsGxsGroupId, CircleWidget*>::iterator itFound;
			if((itFound=_ext_circles_widgets.find(gsItem.mGroupId)) == _ext_circles_widgets.end()) {
				std::cerr << "PeopleDialog::insertExtCircles() add new GroupId: " << gsItem.mGroupId;
				std::cerr << " GroupName: " << gsItem.mGroupName;
				std::cerr << std::endl;

				CircleWidget *gitem = new CircleWidget() ;
				QObject::connect(gitem, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedExt(QList<FlowLayoutItem*>,bool&)));
				QObject::connect(gitem, SIGNAL(askForGXSIdentityWidget(RsGxsId)), this, SLOT(cw_askForGXSIdentityWidget(RsGxsId)));
				QObject::connect(gitem, SIGNAL(askForPGPIdentityWidget(RsPgpId)), this, SLOT(cw_askForPGPIdentityWidget(RsPgpId)));
				QObject::connect(gitem, SIGNAL(imageUpdated()), this, SLOT(cw_imageUpdatedExt()));
				gitem->updateData( gsItem, details );
				_ext_circles_widgets[gsItem.mGroupId] = gitem ;

				_flowLayoutExt->addWidget(gitem);

				QPixmap pixmap = gitem->getImage();
				pictureFlowWidgetExternal->addSlide( pixmap );
				_extListCir << gitem;
			}
            else
            {
				std::cerr << "PeopleDialog::insertExtCircles() Update GroupId: " << gsItem.mGroupId;
				std::cerr << " GroupName: " << gsItem.mGroupName;
				std::cerr << std::endl;

				CircleWidget *cirWidget = itFound->second;
				cirWidget->updateData( gsItem, details );

				//int index = _extListCir.indexOf(cirWidget);
				//QPixmap pixmap = cirWidget->getImage();
				//pictureFlowWidgetExternal->setSlide(index, pixmap);
			}
		}
	}
}

void PeopleDialog::requestIdList()
{
	std::cerr << "Requesting ID list..." << std::endl;

	if (!mIdentityQueue) return;

	mIdentityQueue->cancelActiveRequestTokens(PD_IDLIST);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;

	mIdentityQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, PD_IDLIST);
}

void PeopleDialog::requestCirclesList()
{
	std::cerr << "Requesting Circles list..." << std::endl;

	if (!mCirclesQueue) return;

	mCirclesQueue->cancelActiveRequestTokens(PD_CIRCLES);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mCirclesQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, PD_CIRCLES);
}

void PeopleDialog::updateCirclesDisplay(bool)
{
	std::cerr << "!!Updating circles display!" << std::endl;

	requestCirclesList() ;
}

void PeopleDialog::loadRequest(const TokenQueue * /*queue*/, const TokenRequest &req)
{
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	switch(req.mUserType) {
	case PD_IDLIST:
		insertIdList(req.mToken);
	break;

	case PD_IDDETAILS:
		//insertIdDetails(req.mToken);
	break;

	case PD_CIRCLES:
		insertCircles(req.mToken);
	break;

	case PD_REFRESH:
		updateDisplay(true);
	break;
	default:
		std::cerr << "IdDialog::loadRequest() ERROR";
		std::cerr << std::endl;
	break;
	}//switch(req.mUserType)

}

void PeopleDialog::iw_AddButtonClickedExt()
{
	IdentityWidget *dest=
	    qobject_cast<IdentityWidget *>(QObject::sender());
	if (dest)
    {
		QMenu contextMnu( this );
		
		QMenu *mnu = contextMnu.addMenu(QIcon(":/icons/png/circles.png"),tr("Invite to Circle")) ;

		std::map<RsGxsGroupId, CircleWidget*>::iterator itCurs;
		for( itCurs =_ext_circles_widgets.begin(); itCurs != _ext_circles_widgets.end(); ++itCurs)
        {
			CircleWidget *curs = itCurs->second;
			QIcon icon = QIcon(curs->getImage());
			QString name = curs->getName();

			QAction *action = mnu->addAction(icon, name, this, SLOT(addToCircleExt()));
			action->setData(QString::fromStdString(curs->groupInfo().mGroupId.toStdString())
			                + ";" + QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));
		}
		
		  std::list<RsGxsId> own_identities ;
      rsIdentity->getOwnIds(own_identities) ;
      
      if(own_identities.size() <= 1)
			{
				QAction *action = contextMnu.addAction(QIcon(":/images/chat_24.png"), tr("Chat with this person"), this, SLOT(chatIdentity()));

				if(own_identities.empty())
					action->setEnabled(false) ;
				else
					action->setData(QString::fromStdString((own_identities.front()).toStdString()) + ";" + QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString())) ;
			}
			else
			{
				QMenu *mnu = contextMnu.addMenu(QIcon(":/icons/png/chats.png"),tr("Chat with this person as...")) ;

				for(std::list<RsGxsId>::const_iterator it=own_identities.begin();it!=own_identities.end();++it)
				{
					RsIdentityDetails idd ;
					rsIdentity->getIdDetails(*it,idd) ;

					QPixmap pixmap ;

					if(idd.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idd.mAvatar.mData, idd.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
						pixmap = GxsIdDetails::makeDefaultIcon(*it,GxsIdDetails::SMALL) ;

					QAction *action = mnu->addAction(QIcon(pixmap), QString("%1 (%2)").arg(QString::fromUtf8(idd.mNickname.c_str()), QString::fromStdString((*it).toStdString())), this, SLOT(chatIdentity()));
					action->setData(QString::fromStdString((*it).toStdString()) + ";" + QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString())) ;
				}
			}
			
			QAction *actionsendmsg = contextMnu.addAction(QIcon(":/icons/mail/write-mail.png"), tr("Send message"), this, SLOT(sendMessage()));
			actionsendmsg->setData( QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));
			
			QAction *actionsendinvite = contextMnu.addAction(QIcon(":/icons/mail/write-mail.png"), tr("Send invite"), this, SLOT(sendInvite()));
			actionsendinvite->setData( QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));
			
			contextMnu.addSeparator();
			
			QAction *actionaddcontact = contextMnu.addAction(QIcon(""), tr("Add to Contacts"), this, SLOT(addtoContacts()));
			actionaddcontact->setData( QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));
			
			contextMnu.addSeparator();
			
			QAction *actionDetails = contextMnu.addAction(QIcon(":/images/info16.png"), tr("Person details"), this, SLOT(personDetails()));
			actionDetails->setData( QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));

		contextMnu.exec(QCursor::pos());
	}
}

void PeopleDialog::iw_AddButtonClickedInt()
{
	IdentityWidget *dest=
	    qobject_cast<IdentityWidget *>(QObject::sender());
	if (dest) {
		QMenu contextMnu( this );

		std::map<RsGxsGroupId, CircleWidget*>::iterator itCurs;
		for( itCurs =_int_circles_widgets.begin();
		     itCurs != _int_circles_widgets.end();
		     ++itCurs) {
			CircleWidget *curs = itCurs->second;
			QIcon icon = QIcon(curs->getImage());
			QString name = curs->getName();

			QAction *action = contextMnu.addAction(icon, name, this, SLOT(addToCircleInt()));
			action->setData(QString::fromStdString(curs->groupInfo().mGroupId.toStdString())
			                + ";" + QString::fromStdString(dest->details().gpg_id.toStdString()));
		}

		contextMnu.exec(QCursor::pos());
	}
}

void PeopleDialog::addToCircleExt()
{
	QAction *action =
	    qobject_cast<QAction *>(QObject::sender());
	if (action) {
		QString data = action->data().toString();
		QStringList idList = data.split(";");

		RsGxsGroupId groupId = RsGxsGroupId(idList.at(0).toStdString());
		std::map<RsGxsGroupId, CircleWidget*>::iterator itCirFound;
		if((itCirFound=_ext_circles_widgets.find(groupId)) != _ext_circles_widgets.end()) {
			CircleWidget *circle = itCirFound->second;
			CreateCircleDialog dlg;
			dlg.addCircle(circle->circleDetails());

			RsGxsId gxs_id = RsGxsId(idList.at(1).toStdString());

			std::map<RsGxsId,IdentityWidget *>::iterator itIdFound;
			if((itIdFound=_gxs_identity_widgets.find(gxs_id)) != _gxs_identity_widgets.end()) {
				IdentityWidget *idWidget = itIdFound->second;
				dlg.addMember(idWidget->groupInfo());
			}//if((itFound=_gxs_identity_widgets.find(gxs_id)) != _gxs_identity_widgets.end())

			dlg.editExistingId(circle->groupInfo().mGroupId, false,false);
			dlg.exec();
		}//if((itFound=_ext_circles_widgets.find(groupId)) != _ext_circles_widgets.end())
	}//if (action)
}

void PeopleDialog::addToCircleInt()
{
	QAction *action =
	    qobject_cast<QAction *>(QObject::sender());
	if (action) {
		QString data = action->data().toString();
		QStringList idList = data.split(";");

		RsGxsGroupId groupId = RsGxsGroupId(idList.at(0).toStdString());
		std::map<RsGxsGroupId, CircleWidget*>::iterator itCirFound;
		if((itCirFound=_int_circles_widgets.find(groupId)) != _int_circles_widgets.end()) {
			CircleWidget *circle = itCirFound->second;
			CreateCircleDialog dlg;
			dlg.addCircle(circle->circleDetails());

			RsPgpId pgp_id = RsPgpId(idList.at(1).toStdString());

			std::map<RsPgpId,IdentityWidget *>::iterator itIdFound;
			if((itIdFound=_pgp_identity_widgets.find(pgp_id)) != _pgp_identity_widgets.end()) {
				IdentityWidget *idWidget = itIdFound->second;
				dlg.addMember(idWidget->keyId(), idWidget->idtype(), idWidget->nickname(), QIcon(QPixmap::fromImage(idWidget->avatar())) );
			}//if((itFound=_pgp_identity_widgets.find(pgp_id)) != _pgp_identity_widgets.end())

			dlg.editExistingId(circle->groupInfo().mGroupId, false,false);
			dlg.exec();
		}//if((itFound=_ext_circles_widgets.find(groupId)) != _ext_circles_widgets.end())
	}//if (action)
}

void PeopleDialog::chatIdentity()
{
	QAction *action =
	    qobject_cast<QAction *>(QObject::sender());
	if (action) {
      QString data = action->data().toString();
      QStringList idList = data.split(";");

      RsGxsId from_gxs_id = RsGxsId(idList.at(0).toStdString());
			RsGxsId gxs_id = RsGxsId(idList.at(1).toStdString());
				
			uint32_t error_code ;

            DistantChatPeerId dpid ;
            
      if(!rsMsgs->initiateDistantChatConnexion(RsGxsId(gxs_id), from_gxs_id, dpid,error_code))
	      QMessageBox::information(NULL, tr("Distant chat cannot work"), QString("%1 %2: %3").arg(tr("Distant chat refused with this person.")).arg(tr("Error code")).arg(error_code)) ;

		}
}

void PeopleDialog::sendMessage()
{
	QAction *action =
	    qobject_cast<QAction *>(QObject::sender());
	if (action) {
		QString data = action->data().toString();

    MessageComposer *nMsgDialog = MessageComposer::newMsg();
    if (nMsgDialog == NULL) {
      return;
    }

   	RsGxsId gxs_id = RsGxsId(data.toStdString());;

    nMsgDialog->addRecipient(MessageComposer::TO,  RsGxsId(gxs_id));
		nMsgDialog->show();
		nMsgDialog->activateWindow();

    /* window will destroy itself! */
    
    }

}

void PeopleDialog::sendInvite()
{
	QAction *action =
	    qobject_cast<QAction *>(QObject::sender());
	if (action) {
		QString data = action->data().toString();

   	RsGxsId gxs_id = RsGxsId(data.toStdString());;
    
    MessageComposer::sendInvite(gxs_id,false);

	}
    

}

void PeopleDialog::addtoContacts()
{
	QAction *action =
	    qobject_cast<QAction *>(QObject::sender());
	if (action) {
		QString data = action->data().toString();
		
	RsGxsId gxs_id = RsGxsId(data.toStdString());;

	rsIdentity->setAsRegularContact(RsGxsId(gxs_id),true);
    }

}

void PeopleDialog::personDetails()
{
	QAction *action =
	    qobject_cast<QAction *>(QObject::sender());
	if (action) {
		QString data = action->data().toString();

   	RsGxsId gxs_id = RsGxsId(data.toStdString());
   	
    if (RsGxsGroupId(gxs_id).isNull()) {
        return;
    }

	/* window will destroy itself! */
	IdDialog *idDialog = dynamic_cast<IdDialog*>(MainWindow::getPage(MainWindow::People));

	if (!idDialog)
		return ;

	MainWindow::showWindow(MainWindow::People);
	idDialog->navigate(RsGxsId(gxs_id));

    }

}

void PeopleDialog::cw_askForGXSIdentityWidget(RsGxsId gxs_id)
{
	CircleWidget *dest =
	    qobject_cast<CircleWidget *>(QObject::sender());
	if (dest) {

		std::map<RsGxsId,IdentityWidget *>::iterator itFound;
		if((itFound=_gxs_identity_widgets.find(gxs_id)) != _gxs_identity_widgets.end()) {
			IdentityWidget *idWidget = itFound->second;
			dest->addIdent(idWidget);
		}//if((itFound=_gxs_identity_widgets.find(gxs_id)) != _gxs_identity_widgets.end()) {
	} else {
		reloadAll();
	}
}

void PeopleDialog::cw_askForPGPIdentityWidget(RsPgpId pgp_id)
{
	CircleWidget *dest =
	    qobject_cast<CircleWidget *>(QObject::sender());
	if (dest) {

		std::map<RsPgpId,IdentityWidget *>::iterator itFound;
		if((itFound=_pgp_identity_widgets.find(pgp_id)) != _pgp_identity_widgets.end()) {
			IdentityWidget *idWidget = itFound->second;
			dest->addIdent(idWidget);
		}//if((itFound=_pgp_identity_widgets.find(gxs_id)) != _pgp_identity_widgets.end()) {
	} else {
		reloadAll();
	}
}

void PeopleDialog::cw_imageUpdatedInt()
{
	CircleWidget *cirWidget =
	    qobject_cast<CircleWidget *>(QObject::sender());
	if (cirWidget){
		int index = _intListCir.indexOf(cirWidget);
		QPixmap pixmap = cirWidget->getImage();
		pictureFlowWidgetInternal->setSlide(index, pixmap);
	}//if (cirWidget)
}

void PeopleDialog::cw_imageUpdatedExt()
{
	CircleWidget *cirWidget =
	    qobject_cast<CircleWidget *>(QObject::sender());
	if (cirWidget){
		int index = _extListCir.indexOf(cirWidget);
		QPixmap pixmap = cirWidget->getImage();
		pictureFlowWidgetExternal->setSlide(index, pixmap);
	}//if (cirWidget)
}

void PeopleDialog::fl_flowLayoutItemDroppedExt(QList<FlowLayoutItem *>flListItem, bool &bAccept)
{
	bAccept=false;
	bool bCreateNewCircle=false;
	QApplication::restoreOverrideCursor();

	FlowLayoutItem *dest =
	    qobject_cast<FlowLayoutItem *>(QObject::sender());
	if (dest) {
		CreateCircleDialog dlg;

		CircleWidget* cirDest = qobject_cast<CircleWidget*>(dest);
		if (cirDest) {
			dlg.addCircle(cirDest->circleDetails());

		} else {//if (cirDest)
			bCreateNewCircle=true;
		IdentityWidget* idDest = qobject_cast<IdentityWidget*>(dest);
		if (idDest) {
				if (idDest->haveGXSId()){
				dlg.addMember(idDest->groupInfo());

				}//if (idDest->haveGXSId())
		}//if (idDest)
		}//else (cirDest)


		typedef QList<FlowLayoutItem *>::Iterator itList;
		for (itList listCurs = flListItem.begin()
		     ; listCurs != flListItem.end()
		     ; ++listCurs) {
			FlowLayoutItem *flCurs = *listCurs;
			CircleWidget* cirDropped = qobject_cast<CircleWidget*>(flCurs);
			//Create new circle if circle dropped in circle or ident
			if (cirDropped) {
				bCreateNewCircle = true;
				dlg.addCircle(cirDropped->circleDetails());

			} else {//if (cirDropped)
				IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
				if (idDropped){
					if (idDropped->haveGXSId()){
					dlg.addMember(idDropped->groupInfo());

					}//if (idDropped->haveGXSId())
				}//if (idDropped)
			}//else (cirDropped)

		}//for (itList listCurs = flListItem.begin()

		if (bCreateNewCircle){
			dlg.editNewId(true);
		} else {//if (bCreateNewCircle)
			dlg.editExistingId(cirDest->groupInfo().mGroupId, false,false);
		}//else (bCreateNewCircle)

		dlg.exec();

		bAccept=true;
	}//if (dest)
}

void PeopleDialog::fl_flowLayoutItemDroppedInt(QList<FlowLayoutItem *>flListItem, bool &bAccept)
{
	bAccept=false;
	bool bCreateNewCircle=false;
	QApplication::restoreOverrideCursor();

	FlowLayoutItem *dest =
	    qobject_cast<FlowLayoutItem *>(QObject::sender());
	if (dest) {
		CreateCircleDialog dlg;

		CircleWidget* cirDest = qobject_cast<CircleWidget*>(dest);
		if (cirDest) {
			dlg.addCircle(cirDest->circleDetails());

		} else {//if (cirDest)
			bCreateNewCircle=true;
			IdentityWidget* idDest = qobject_cast<IdentityWidget*>(dest);
			if (idDest) {
				if (idDest->havePGPDetail()){
					dlg.addMember(idDest->keyId(), idDest->idtype(), idDest->nickname(), QIcon(QPixmap::fromImage(idDest->avatar())) );

				}//if (idDest->havePGPDetail())
			}//if (idDest)
		}//else (cirDest)


		typedef QList<FlowLayoutItem *>::Iterator itList;
		for (itList listCurs = flListItem.begin()
		     ; listCurs != flListItem.end()
		     ; ++listCurs) {
			FlowLayoutItem *flCurs = *listCurs;
			CircleWidget* cirDropped = qobject_cast<CircleWidget*>(flCurs);
			//Create new circle if circle dropped in circle or ident
			if (cirDropped) {
				bCreateNewCircle = true;
				dlg.addCircle(cirDropped->circleDetails());

			} else {//if (cirDropped)
				IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
				if (idDropped){
					dlg.addMember(idDropped->keyId(), idDropped->idtype(), idDropped->nickname(), QIcon(QPixmap::fromImage(idDropped->avatar())) );

				}//if (idDropped)
			}//else (cirDropped)

		}//for (itList listCurs = flListItem.begin()

		if (bCreateNewCircle){
			dlg.editNewId(false);
		} else {//if (bCreateNewCircle)
			dlg.editExistingId(cirDest->groupInfo().mGroupId, false,false);
		}//else (bCreateNewCircle)

		dlg.exec();

		bAccept=true;
	}//if (dest)
}

void PeopleDialog::pf_centerIndexChanged(int index)
{
	Q_UNUSED(index)
}

void PeopleDialog::pf_mouseMoveOverSlideEvent(QMouseEvent* event, int slideIndex)
{
	Q_UNUSED(event)
	Q_UNUSED(slideIndex)
}

void PeopleDialog::pf_dragEnterEventOccurs(QDragEnterEvent *event)
{
	FlowLayoutItem *flItem =
	    qobject_cast<FlowLayoutItem *>(event->source());
	if (flItem) {
		event->setDropAction(Qt::CopyAction);
		event->accept();
		return;
	}//if (flItem)
	QWidget *wid =
	    qobject_cast<QWidget *>(event->source());//QT5 return QObject
	FlowLayout *layout = 0;
	if (wid) layout =
	    qobject_cast<FlowLayout *>(wid->layout());
	if (layout) {
		event->setDropAction(Qt::CopyAction);
		event->accept();
		return;
	}//if (layout)
}

void PeopleDialog::pf_dragMoveEventOccurs(QDragMoveEvent *event)
{
	FlowLayoutItem *flItem =
	    qobject_cast<FlowLayoutItem *>(event->source());
	if (flItem) {
		event->setDropAction(Qt::CopyAction);
		event->accept();
		return;
	}//if (flItem)
	QWidget *wid =
	    qobject_cast<QWidget *>(event->source());//QT5 return QObject
	FlowLayout *layout = 0;
	if (wid) layout =
	    qobject_cast<FlowLayout *>(wid->layout());
	if (layout) {
		event->setDropAction(Qt::CopyAction);
		event->accept();
		return;
	}//if (layout)
}

void PeopleDialog::pf_dropEventOccursExt(QDropEvent *event)
{
	bool bCreateNewCircle=false;
	bool atLeastOne = false;
	QApplication::restoreOverrideCursor();

	int index = pictureFlowWidgetExternal->centerIndex();
	CircleWidget* cirDest = _extListCir[index];
	if (cirDest) {
		CreateCircleDialog dlg;

		dlg.addCircle(cirDest->circleDetails());

		{//Test if source is only one FlowLayoutItem
			FlowLayoutItem *flCurs =
			    qobject_cast<FlowLayoutItem *>(event->source());
			if (flCurs) {
				CircleWidget* cirDropped = qobject_cast<CircleWidget*>(flCurs);
				//Create new circle if circle dropped in circle or ident
				if (cirDropped) {
					bCreateNewCircle = true;
					dlg.addCircle(cirDropped->circleDetails());
					atLeastOne = true;

				} else {//if (cirDropped)
					IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
					if (idDropped){
						if (idDropped->haveGXSId()){
							dlg.addMember(idDropped->groupInfo());
							atLeastOne = true;
						}//if (idDropped->haveGXSId())
					}//if (idDropped)
				}//else (cirDropped)

			}//if (flCurs)
		}//End Test if source is only one FlowLayoutItem

		QWidget *wid =
		    qobject_cast<QWidget *>(event->source());//QT5 return QObject
		FlowLayout *layout = NULL;
		if (wid) layout =
		    qobject_cast<FlowLayout *>(wid->layout());
		if (layout) {

			QList<QLayoutItem *> list = layout->selectionList();
			int count = list.count();
			for (int curs = 0; curs < count; ++curs){
				QLayoutItem *layoutItem = list.at(curs);
				if (layoutItem){
					FlowLayoutItem *flCurs =
					    qobject_cast<FlowLayoutItem *>(layoutItem->widget());
					if (flCurs){
						CircleWidget* cirDropped = qobject_cast<CircleWidget*>(flCurs);
						//Create new circle if circle dropped in circle or ident
						if (cirDropped) {
							bCreateNewCircle = true;
							dlg.addCircle(cirDropped->circleDetails());
							atLeastOne = true;

						} else {//if (cirDropped)
							IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
							if (idDropped){
								if (idDropped->haveGXSId()){
								dlg.addMember(idDropped->groupInfo());
								atLeastOne = true;
								}//if (idDropped->haveGXSId())

							}//if (idDropped)
						}//else (cirDropped)

					}//if (flCurs)
				}//if (layoutItem)
			}//for (int curs = 0; curs < count; ++curs)
		}//if (layout)

		if (atLeastOne) {
			if (bCreateNewCircle){
				dlg.editNewId(true);
			} else {//if (bCreateNewCircle)
				dlg.editExistingId(cirDest->groupInfo().mGroupId, false,false);
			}//else (bCreateNewCircle)

			dlg.exec();

			event->setDropAction(Qt::CopyAction);
			event->accept();
		}//if (atLeastOne)
	}//if (cirDest)
}

void PeopleDialog::pf_dropEventOccursInt(QDropEvent *event)
		{
	bool bCreateNewCircle=false;
	bool atLeastOne = false;
	QApplication::restoreOverrideCursor();

	int index = pictureFlowWidgetInternal->centerIndex();
	CircleWidget* cirDest = _intListCir[index];
	if (cirDest) {
		CreateCircleDialog dlg;

		dlg.addCircle(cirDest->circleDetails());

		{//Test if source is only one FlowLayoutItem
			FlowLayoutItem *flCurs =
			    qobject_cast<FlowLayoutItem *>(event->source());
			if (flCurs) {
				CircleWidget* cirDropped = qobject_cast<CircleWidget*>(flCurs);
				//Create new circle if circle dropped in circle or ident
				if (cirDropped) {
					bCreateNewCircle = true;
					dlg.addCircle(cirDropped->circleDetails());
					atLeastOne = true;

				} else {//if (cirDropped)
					IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
					if (idDropped){
						if (idDropped->havePGPDetail()){
							dlg.addMember(idDropped->keyId(), idDropped->idtype(), idDropped->nickname(), QIcon(QPixmap::fromImage(idDropped->avatar())) );
							atLeastOne = true;
						}//if (idDropped->havePGPDetail())
					}//if (idDropped)
				}//else (cirDropped)

			}//if (flCurs)
		}//End Test if source is only one FlowLayoutItem

		QWidget *wid =
		    qobject_cast<QWidget *>(event->source());//QT5 return QObject
		FlowLayout *layout = NULL;
		if (wid) layout =
		    qobject_cast<FlowLayout *>(wid->layout());
		if (layout) {

			QList<QLayoutItem *> list = layout->selectionList();
			int count = list.count();
			for (int curs = 0; curs < count; ++curs){
				QLayoutItem *layoutItem = list.at(curs);
				if (layoutItem){
					FlowLayoutItem *flCurs =
					    qobject_cast<FlowLayoutItem *>(layoutItem->widget());
					if (flCurs){
						CircleWidget* cirDropped = qobject_cast<CircleWidget*>(flCurs);
						//Create new circle if circle dropped in circle or ident
						if (cirDropped) {
							bCreateNewCircle = true;
							dlg.addCircle(cirDropped->circleDetails());
							atLeastOne = true;

						} else {//if (cirDropped)
							IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
							if (idDropped){
								if (idDropped->havePGPDetail()){
									dlg.addMember(idDropped->keyId(), idDropped->idtype(), idDropped->nickname(), QIcon(QPixmap::fromImage(idDropped->avatar())) );
									atLeastOne = true;
								}//if (idDropped->havePGPDetail())
	
							}//if (idDropped)
						}//else (cirDropped)

					}//if (flCurs)
				}//if (layoutItem)
			}//for (int curs = 0; curs < count; ++curs)
		}//if (layout)

		if (atLeastOne) {
			if (bCreateNewCircle){
				dlg.editNewId(false);
			} else {//if (bCreateNewCircle)
				dlg.editExistingId(cirDest->groupInfo().mGroupId, false,false);
			}//else (bCreateNewCircle)

	dlg.exec();

			event->setDropAction(Qt::CopyAction);
			event->accept();
		}//if (atLeastOne)
	}//if (cirDest)
}

void PeopleDialog::populatePictureFlowExt()
	{
	std::map<RsGxsGroupId,CircleWidget *>::iterator it;
	for (it=_ext_circles_widgets.begin(); it!=_ext_circles_widgets.end(); ++it) {
		CircleWidget *item = it->second;
		QPixmap pixmap = item->getImage();
		pictureFlowWidgetExternal->addSlide( pixmap );
	}//for (it=_ext_circles_widgets.begin(); it!=_ext_circles_widgets.end(); ++it)
	pictureFlowWidgetExternal->setSlideSizeRatio(4/4.0);
	}

void PeopleDialog::populatePictureFlowInt()
	{
	std::map<RsGxsGroupId,CircleWidget *>::iterator it;
	for (it=_int_circles_widgets.begin(); it!=_int_circles_widgets.end(); ++it) {
		CircleWidget *item = it->second;
		QPixmap pixmap = item->getImage();
		pictureFlowWidgetInternal->addSlide( pixmap );
	}//for (it=_int_circles_widgets.begin(); it!=_int_circles_widgets.end(); ++it)
	pictureFlowWidgetInternal->setSlideSizeRatio(4/4.0);
}
