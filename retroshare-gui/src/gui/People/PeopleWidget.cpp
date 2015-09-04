/*
 * Retroshare Identity.
 *
 * Copyright 2014-2014 by Cyril Soler
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
 * Please report all bugs and problems to "retroshare.project@gmail.com".
 *
 */


#include "PeopleWidget.h"
#include "gui/common/FlowLayout.h"
#include "gui/settings/rsharesettings.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/gxs/GxsIdDetails.h"

#include "retroshare/rspeers.h"
#include "retroshare/rsidentity.h"
#include "retroshare/rsgxscircles.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsmsgs.h" 

#include <iostream>
#include <QMenu>
#include <QMessageBox>

/******
 * #define PEOPLE_DIALOG_DEBUG 1
 *****/


const uint32_t PeopleWidget::PD_IDLIST    = 0x0001 ;
const uint32_t PeopleWidget::PD_IDDETAILS = 0x0002 ;
const uint32_t PeopleWidget::PD_REFRESH   = 0x0003 ;
const uint32_t PeopleWidget::PD_CIRCLES   = 0x0004 ;

/** Constructor */
PeopleWidget::PeopleWidget(QWidget *parent)
	: RsGxsUpdateBroadcastPage(rsIdentity, parent)
{
	setupUi(this);

	/* Setup TokenQueue */
	mIdentityQueue = new TokenQueue(rsIdentity->getTokenService(), this);

	//need erase QtCreator Layout first(for Win)
	delete idExternal->layout();
	//QT Designer don't accept Custom Layout, maybe on QT5
	_flowLayoutExt = new FlowLayout(idExternal);

	{
		int count = idExternal->children().count();
		for (int curs = 0; curs < count; ++curs){
			QObject *obj = idExternal->children().at(curs);
			QWidget *wid = qobject_cast<QWidget *>(obj);
			if (wid) _flowLayoutExt->addWidget(wid);
		}
	}

}

/** Destructor. */
PeopleWidget::~PeopleWidget()
{
	delete mIdentityQueue;

}

void PeopleWidget::updateDisplay(bool complete)
{
	Q_UNUSED(complete);
	reloadAll();
}

void PeopleWidget::reloadAll()
{
	/* Update identity list */
	requestIdList();

}

void PeopleWidget::insertIdList(uint32_t token)
{
	std::cerr << "**** In insertIdList() ****" << std::endl;

	std::vector<RsGxsIdGroup> gdataVector;
	std::vector<RsGxsIdGroup>::iterator gdIt;

	if (!rsIdentity->getGroupData(token, gdataVector)) {
		std::cerr << "PeopleWidget::insertIdList() Error getting GroupData";
		std::cerr << std::endl;

		return;
	}

	/* Insert items */
	int i=0 ;
	for (gdIt = gdataVector.begin(); gdIt != gdataVector.end(); ++gdIt){
		RsGxsIdGroup gdItem = (*gdIt);
		bool bGotDetail = false;

		RsPeerDetails details;
		if (gdItem.mPgpKnown) {
			bGotDetail = rsPeers->getGPGDetails(gdItem.mPgpId, details);
		}

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

			QObject::connect(new_item, SIGNAL(addButtonClicked()), this, SLOT(iw_AddButtonClicked()));
			QObject::connect(new_item, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedExt(QList<FlowLayoutItem*>,bool&)));
			_flowLayoutExt->addWidget(new_item);
			++i ;
		} else {

			std::cerr << "Updating data vector identity GXS ID = " << gdItem.mMeta.mGroupId << std::endl;
			IdentityWidget *idWidget = itFound->second;

			if (bGotDetail) {
				idWidget->updateData(gdItem, details) ;
			} else {//if (bGotDetail)
				idWidget->updateData(gdItem) ;
			}//else (bGotDetail)

		}
	}
}



void PeopleWidget::requestIdList()
{
	std::cerr << "Requesting ID list..." << std::endl;

	if (!mIdentityQueue) return;

	mIdentityQueue->cancelActiveRequestTokens(PD_IDLIST);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;

	mIdentityQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, PD_IDLIST);
}

void PeopleWidget::loadRequest(const TokenQueue * /*queue*/, const TokenRequest &req)
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
			//insertCircles(req.mToken);
		break;

		case PD_REFRESH:
			updateDisplay(true);
		break;
		default:
			std::cerr << "IdDialog::loadRequest() ERROR";
			std::cerr << std::endl;
		break;
	}
}

void PeopleWidget::iw_AddButtonClicked()
{
	IdentityWidget *dest=
	    qobject_cast<IdentityWidget *>(QObject::sender());
	    
	if (dest) {
      QMenu contextMnu( this );
      
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
				QMenu *mnu = contextMnu.addMenu(QIcon(":/images/chat_24.png"),tr("Chat with this person as...")) ;

				for(std::list<RsGxsId>::const_iterator it=own_identities.begin();it!=own_identities.end();++it)
				{
					RsIdentityDetails idd ;
					rsIdentity->getIdDetails(*it,idd) ;

					QPixmap pixmap ;

					if(idd.mAvatar.mSize == 0 || !pixmap.loadFromData(idd.mAvatar.mData, idd.mAvatar.mSize, "PNG"))
						pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(*it)) ;

					QAction *action = mnu->addAction(QIcon(pixmap), QString("%1 (%2)").arg(QString::fromUtf8(idd.mNickname.c_str()), QString::fromStdString((*it).toStdString())), this, SLOT(chatIdentity()));
					action->setData(QString::fromStdString((*it).toStdString()) + ";" + QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString())) ;
				}
			}
			
			QAction *actionsendmsg = contextMnu.addAction(QIcon(":/images/mail_new.png"), tr("Send message to this person"), this, SLOT(sendMessage()));
			actionsendmsg->setData( QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));
		
			
      contextMnu.exec(QCursor::pos());
		}


}

void PeopleWidget::chatIdentity()
{
	QAction *action =
	    qobject_cast<QAction *>(QObject::sender());
	if (action) {
      QString data = action->data().toString();
      QStringList idList = data.split(";");

      RsGxsId from_gxs_id = RsGxsId(idList.at(0).toStdString());
			RsGxsId gxs_id = RsGxsId(idList.at(1).toStdString());
				
			uint32_t error_code ;

      if(!rsMsgs->initiateDistantChatConnexion(RsGxsId(gxs_id), from_gxs_id, error_code))
      QMessageBox::information(NULL, tr("Distant chat cannot work"), QString("%1 %2: %3").arg(tr("Distant chat refused with this person.")).arg(tr("Error code")).arg(error_code)) ;

		}
}

void PeopleWidget::sendMessage()
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

void PeopleWidget::pf_centerIndexChanged(int index)
{
	Q_UNUSED(index)
}

void PeopleWidget::pf_mouseMoveOverSlideEvent(QMouseEvent* event, int slideIndex)
{
	Q_UNUSED(event)
	Q_UNUSED(slideIndex)
}

void PeopleWidget::pf_dragEnterEventOccurs(QDragEnterEvent *event)
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

void PeopleWidget::pf_dragMoveEventOccurs(QDragMoveEvent *event)
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

