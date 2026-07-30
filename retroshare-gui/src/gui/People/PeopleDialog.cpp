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
#include "gui/common/FilesDefs.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"

#include "retroshare/rspeers.h"
#include "retroshare/rsidentity.h"
#include "retroshare/rsgxscircles.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsmail.h"
#include "retroshare/rschats.h"
#include "retroshare/rsids.h"

#include <iostream>
#include <QApplication>
#include <QDateTime>
#include <QMenu>
#include <QMessageBox>
#include <QSignalBlocker>

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
	//hide circle flow widget not functional more
	switchButton->hide(); //disable this, to enable the circles flow widget

    UsagePage = new UsageStatistics(this); 
    detailsStackedWidget->addWidget(UsagePage);

	//need erase QtCreator Layout first(for Win)
	delete idExternal->layout();
	delete idInternal->layout();
	//QT Designer don't accept Custom Layout, maybe on QT5
	_flowLayoutExt = new FlowLayout(idExternal);
	_flowLayoutInt = new FlowLayout(idInternal);

	// Setup circlesTreeWidget
	circlesTreeWidget->header()->resizeSection(0, 160);
	circlesTreeWidget->header()->resizeSection(1, 80);
	circlesTreeWidget->header()->resizeSection(2, 60);

	connect(circlesTreeWidget, &QTreeWidget::itemClicked, this, &PeopleDialog::onCircleTreeItemClicked);
	connect(circlesTreeWidget, &QTreeWidget::customContextMenuRequested, this, &PeopleDialog::onCircleTreeContextMenuRequested);

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
    
    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
	connect(inviteButton, SIGNAL(clicked()), this, SLOT(sendInvite()));
	connect(switchButton, SIGNAL(clicked()), this, SLOT(toggleStackedPage()));
	connect(statsButton, SIGNAL(clicked()), this, SLOT(toggledetailsStackedPage()));
	connect(detailsStackedWidget, &QStackedWidget::currentChanged, this, &PeopleDialog::onDetailsPageChanged);
	connect(ownOpinion_CB, SIGNAL(currentIndexChanged(int)), this, SLOT(modifyReputation()));
	connect(backButton, &QPushButton::clicked, this, &PeopleDialog::onBackClicked);
	connect(joinLeaveCircleButton, &QPushButton::clicked, this, &PeopleDialog::requestJoinLeaveCircle);

	QByteArray geometryExt = Settings->valueFromGroup("PeopleDialog", "SplitterExtState", QByteArray()).toByteArray();
	if (geometryExt.isEmpty() == false) {
		splitterExternal->restoreState(geometryExt);
	}
	QByteArray geometryInt = Settings->valueFromGroup("PeopleDialog", "SplitterIntState", QByteArray()).toByteArray();
	if (geometryInt.isEmpty() == false) {
		splitterInternal->restoreState(geometryInt);
	}

	/* Thumbnails (index 0) stretch to fill space; details (index 1) stays compact */
	splitterExternal->setStretchFactor(0, 1);
	splitterExternal->setStretchFactor(1, 0);

	// Create the sort menu
	QMenu *sortMenu = new QMenu(this);
	sortMenu->addAction(tr("Sort by Name"), this, SLOT(sortByName()));
	sortMenu->addAction(tr("Sort by Popularity"), this, SLOT(sortByPopularity()));

	// Assign the menu
	filterButton->setMenu(sortMenu);
	filterButton->setPopupMode(QToolButton::InstantPopup);

	reloadAll();
	showNoneSelected();

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

static QString getHumanReadableDuration(uint32_t seconds)
{
    if(seconds < 60)
        return QString(QObject::tr("%1 seconds ago")).arg(seconds) ;
    else if(seconds < 120)
        return QString(QObject::tr("%1 minute ago")).arg(seconds/60) ;
    else if(seconds < 3600)
        return QString(QObject::tr("%1 minutes ago")).arg(seconds/60) ;
    else if(seconds < 7200)
        return QString(QObject::tr("%1 hour ago")).arg(seconds/3600) ;
    else if(seconds < 24*3600)
        return QString(QObject::tr("%1 hours ago")).arg(seconds/3600) ;
    else if(seconds < 2*24*3600)
        return QString(QObject::tr("%1 day ago")).arg(seconds/86400) ;
    else
        return QString(QObject::tr("%1 days ago")).arg(seconds/86400) ;
}

void PeopleDialog::reloadAll()
{
    /* Update identity list */
    requestIdList();
    requestCirclesList();

    std::list<RsPgpId> friend_pgpIds;
    rsPeers->getGPGAcceptedList(friend_pgpIds);

    // 1. Collect widgets in a list for sorting
    QList<IdentityWidget*> toSort;

    for(std::list<RsPgpId>::iterator it = friend_pgpIds.begin(); it != friend_pgpIds.end(); ++it) {
        RsPeerDetails details;
        if(rsPeers->getGPGDetails(*it, details)) {
            std::map<RsPgpId, IdentityWidget*>::iterator itFound = _pgp_identity_widgets.find(*it);
            
            IdentityWidget *widget = nullptr;
            if(itFound == _pgp_identity_widgets.end()) {
                // Create new if not exists
                widget = new IdentityWidget();
                _pgp_identity_widgets[*it] = widget;
                QObject::connect(widget, SIGNAL(addButtonClicked()), this, SLOT(iw_AddButtonClickedInt()));
                QObject::connect(widget, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedInt(QList<FlowLayoutItem*>,bool&)));
            } else {
                widget = itFound->second;
            }

            widget->updateData(details);
            toSort.append(widget);
        }
    }

    // 2. Sort alphabetically
    std::sort(toSort.begin(), toSort.end(), [](IdentityWidget* a, IdentityWidget* b) {
        return a->getName().compare(b->getName(), Qt::CaseInsensitive) < 0;
    });

    // 3. Add to layout in sorted order
    for(auto* w : toSort) {
        _flowLayoutInt->addWidget(w);
    }
    
    filterChanged(filterLineEdit->text()); 
}

void PeopleDialog::insertIdList(uint32_t token)
{
    std::cerr << "**** In insertIdList() ****" << std::endl;
    std::vector<RsGxsIdGroup> gdataVector;
    if (!rsIdentity->getGroupData(token, gdataVector)) {
        std::cerr << "PeopleDialog::insertIdList() Error getting GroupData";
        std::cerr << std::endl;

        return;
    }

    // Declare the list at the TOP of the function so it is available everywhere
    QList<IdentityWidget*> toSort;

    for (auto const& gdItem : gdataVector) {
        RsPeerDetails details;
        bool bGotDetail = gdItem.mPgpKnown && rsPeers->getGPGDetails(gdItem.mPgpId, details);

        RsGxsId gxsId(gdItem.mMeta.mGroupId);
        std::map<RsGxsId, IdentityWidget*>::iterator itFound = _gxs_identity_widgets.find(gxsId);
        
        IdentityWidget *widget = nullptr;
        if(itFound == _gxs_identity_widgets.end()) {
            widget = new IdentityWidget();
            _gxs_identity_widgets[gxsId] = widget;
            QObject::connect(widget, SIGNAL(addButtonClicked()), this, SLOT(iw_AddButtonClickedExt()));
            QObject::connect(widget, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedExt(QList<FlowLayoutItem*>,bool&)));
            connect(widget, SIGNAL(clicked()), this, SLOT(onIdentitySelected()));
			//connect(widget, SIGNAL(widgetSelected(IdentityWidget*)), this, SLOT(onIdentitySelected(IdentityWidget*)));
        } else {
            widget = itFound->second;
        }

        if (bGotDetail) widget->updateData(gdItem, details);
        else widget->updateData(gdItem);

        toSort.append(widget);
    }

    // Sort and add to layout
    std::sort(toSort.begin(), toSort.end(), [](IdentityWidget* a, IdentityWidget* b) {
        return a->getName().compare(b->getName(), Qt::CaseInsensitive) < 0;
    });

    for(auto* w : toSort) {
        _flowLayoutExt->addWidget(w);
    }

    filterChanged(filterLineEdit->text());
}

void PeopleDialog::populateCirclesTree(const std::list<RsGroupMetaData>& circles)
{
	for (const auto& gsItem : circles) {
		RsGxsCircleDetails details;
		bool hasDetails = rsGxsCircles && rsGxsCircles->getCircleDetails(RsGxsCircleId(gsItem.mGroupId), details);

		auto itFound = _ext_circles_widgets.find(gsItem.mGroupId);
		if (itFound == _ext_circles_widgets.end()) {
			std::cerr << "PeopleDialog::populateCirclesTree() add new GroupId: " << gsItem.mGroupId
			          << " GroupName: " << gsItem.mGroupName << std::endl;

			CircleWidget *gitem = new CircleWidget();
			QObject::connect(gitem, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDroppedExt(QList<FlowLayoutItem*>,bool&)));
			QObject::connect(gitem, SIGNAL(askForGXSIdentityWidget(RsGxsId)), this, SLOT(cw_askForGXSIdentityWidget(RsGxsId)));
			QObject::connect(gitem, SIGNAL(askForPGPIdentityWidget(RsPgpId)), this, SLOT(cw_askForPGPIdentityWidget(RsPgpId)));
			QObject::connect(gitem, SIGNAL(imageUpdated()), this, SLOT(cw_imageUpdatedExt()));
			QObject::connect(gitem, &CircleWidget::clicked, this, &PeopleDialog::onCircleSelected);

			gitem->updateData(gsItem, details);
			_ext_circles_widgets[gsItem.mGroupId] = gitem;

			QPixmap pixmap = gitem->getImage();
			pictureFlowWidgetExternal->addSlide(pixmap);
			_extListCir << gitem;
		} else {
			CircleWidget *cirWidget = itFound->second;
			cirWidget->updateData(gsItem, details);
		}

		// Update or create QTreeWidgetItem in circlesTreeWidget
		QString groupIdStr = QString::fromStdString(gsItem.mGroupId.toStdString());
		QTreeWidgetItem *treeItem = nullptr;
		for (int i = 0; i < circlesTreeWidget->topLevelItemCount(); ++i) {
			if (circlesTreeWidget->topLevelItem(i)->data(0, Qt::UserRole).toString() == groupIdStr) {
				treeItem = circlesTreeWidget->topLevelItem(i);
				break;
			}
		}

		if (!treeItem) {
			treeItem = new QTreeWidgetItem(circlesTreeWidget);
			treeItem->setData(0, Qt::UserRole, groupIdStr);
			treeItem->setIcon(0, QIcon(":/icons/png/circles.png"));
		}

		treeItem->setText(0, QString::fromUtf8(gsItem.mGroupName.c_str()));
		treeItem->setText(1, hasDetails ? QString::number(details.mAllowedGxsIds.size()) : tr("0"));
	}

	filterChanged(filterLineEdit->text());
}

void PeopleDialog::insertCircles(uint32_t token)
{
	std::cerr << "PeopleDialog::insertExtCircles(token==" << token << ")" << std::endl;

	std::list<RsGroupMetaData> gSummaryList;
	if (rsGxsCircles && rsGxsCircles->getGroupSummary(token, gSummaryList))
	{
		populateCirclesTree(gSummaryList);
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

	if (mCirclesQueue) {
		mCirclesQueue->cancelActiveRequestTokens(PD_CIRCLES);

		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

		uint32_t token;
		mCirclesQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, PD_CIRCLES);
	}

	// Directly query rsGxsCircles asynchronously to guarantee circle loading
	RsThread::async([this]()
	{
		std::list<RsGroupMetaData> circles;
		if (!rsGxsCircles || !rsGxsCircles->getCirclesSummaries(circles)) {
			std::cerr << "PeopleDialog::requestCirclesList() failed to get circles summaries" << std::endl;
			return;
		}

		RsQThreadUtils::postToObject([this, circles]()
		{
			populateCirclesTree(circles);
		}, this);
	});
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
		
		QMenu *mnu = contextMnu.addMenu(FilesDefs::getIconFromQtResourcePath(":/icons/png/circles.png"),tr("Invite to Circle")) ;

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
				QAction *action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/chats.png"), tr("Chat with this person"), this, SLOT(chatIdentity()));

				if(own_identities.empty())
					action->setEnabled(false) ;
				else
					action->setData(QString::fromStdString((own_identities.front()).toStdString()) + ";" + QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString())) ;
			}
			else
			{
				QMenu *mnu = contextMnu.addMenu(FilesDefs::getIconFromQtResourcePath(":/icons/png/chats.png"),tr("Chat with this person as...")) ;

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
			
			QAction *actionsendmsg = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/mail/write-mail.png"), tr("Send message"), this, SLOT(sendMessage()));
			actionsendmsg->setData( QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));
			
			QAction *actionsendinvite = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/mail/write-mail.png"), tr("Send invite"), this, SLOT(sendInvite()));
			actionsendinvite->setData( QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));
			
			contextMnu.addSeparator();
			
			QAction *actionaddcontact = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(""), tr("Add to Contacts"), this, SLOT(addtoContacts()));
			actionaddcontact->setData( QString::fromStdString(dest->groupInfo().mMeta.mGroupId.toStdString()));
			
			contextMnu.addSeparator();
			
			QAction *actionDetails = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/info16.png"), tr("Person details"), this, SLOT(personDetails()));
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
            
      if(!rsChats->initiateDistantChatConnexion(RsGxsId(gxs_id), from_gxs_id, dpid,error_code))
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
    RsGxsId gxs_id;

    // Check if triggered by a Context Menu Action
    QAction *action = qobject_cast<QAction *>(QObject::sender());
    if (action) {
        QString data = action->data().toString();
        gxs_id = RsGxsId(data.toStdString());
    } 
    // Otherwise, check if it was the UI Button
    else {
        gxs_id = mCurrentSelectedId;
    }

    // Execute the invite if the ID is valid
    if (!gxs_id.isNull()) {
        MessageComposer::sendInvite(gxs_id, false);
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

void PeopleDialog::filterChanged(const QString &text)
{
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    if (_selectionMode == SelectionMode::Circle) {
        // When a circle is selected, filter only among its members (shown in A)
        auto it = _ext_circles_widgets.find(_selectedCircleId);
        if (it != _ext_circles_widgets.end()) {
            const auto& allowed = it->second->circleDetails().mAllowedGxsIds;
            for (auto& [id, w] : _gxs_identity_widgets) {
                bool isMember = (allowed.count(id) > 0);
                bool matches  = text.isEmpty() || w->getName().contains(text, cs);
                w->setVisible(isMember && matches);
            }
        }
    } else {
        // No selection / identity selected: filter all identity widgets
        for (auto& [id, w] : _gxs_identity_widgets)
            w->setVisible(text.isEmpty() || w->getName().contains(text, cs));
    }

    // Always filter the PGP identity widgets (internal tab, kept for completeness)
    for (auto& [id, w] : _pgp_identity_widgets)
        w->setVisible(text.isEmpty() || w->getName().contains(text, cs));

    // Filter circlesTreeWidget items based on search text
    for (int i = 0; i < circlesTreeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = circlesTreeWidget->topLevelItem(i);
        QString name = item->text(0);
        item->setHidden(!text.isEmpty() && !name.contains(text, cs));
    }

    // CRITICAL: Invalidate layouts so Qt recomputes positions (not just repaints)
    _flowLayoutExt->invalidate();
    _flowLayoutInt->invalidate();
}

void PeopleDialog::sortByName()
{
    clearAllSelections();
	clearPerson();
    applySortAndFilter(true); // true for name
}

void PeopleDialog::sortByPopularity()
{
    clearAllSelections();
	clearPerson();
    applySortAndFilter(false); // false for popularity
}

void PeopleDialog::applySortAndFilter(bool byName)
{
    clearAllSelections();
    
    QString filterText = filterLineEdit->text();
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    // We process internal and external layouts separately
    auto sortLayout = [&](FlowLayout* layout, auto& widgetMap) {
        // 1. Collect widgets from the map
        QList<QWidget*> list;
        for (auto const& [id, w] : widgetMap) {
            list << w;
            
            // Uncheck/Deselect the item here ---
            w->setIsSelected(false);
        }

        // 2. Sort the list
        std::sort(list.begin(), list.end(), [byName](QWidget* a, QWidget* b) {
            auto* idA = qobject_cast<IdentityWidget*>(a);
            auto* idB = qobject_cast<IdentityWidget*>(b);
            if (!idA || !idB) return false;

            if (byName) {
                return idA->getName().compare(idB->getName(), Qt::CaseInsensitive) < 0;
            } else {
                // Popularity (Higher reputation first)
                return idA->getReputation() > idB->getReputation();
            }
        });

        // 3. Clear and Re-add to layout in sorted order
        for (QWidget* w : list) {
            // Apply filter while we are at it
            QString name = qobject_cast<IdentityWidget*>(w)->getName();
            w->setVisible(name.contains(filterText, cs));
            
            layout->addWidget(w); 
        }
    };

    sortLayout(_flowLayoutInt, _pgp_identity_widgets);
    sortLayout(_flowLayoutExt, _gxs_identity_widgets);
}

void PeopleDialog::clearAllSelections()
{
    // Tell every widget to visually uncheck
    auto clearMap = [](auto& widgetMap) {
        for (auto const& [id, w] : widgetMap) {
            if (w) w->setIsSelected(false);
        }
    };

    clearMap(_pgp_identity_widgets);
    clearMap(_gxs_identity_widgets);
    clearMap(_ext_circles_widgets);
    clearMap(_int_circles_widgets);
}

void PeopleDialog::loadIdentityLabels(const RsGxsIdGroup& data)
{
    RsPgpId ownPgpId = rsPeers->getGPGOwnId();

    lineEdit_PublishTS->setText(DateTime::formatDateTime(data.mMeta.mPublishTs));
    lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId.toStdString()));

    if(data.mPgpKnown)
        lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()));
    else
        lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()) + tr(" [unverified]"));

    // Update Visibility for GPG specific items
    bool hasPgp = !data.mPgpId.isNull();
    autoBanIdentities_CB->setVisible(hasPgp);
    //banoption_label->setVisible(hasPgp);
    lineEdit_GpgId->setVisible(hasPgp);
    label_GpgId->setVisible(hasPgp);

    time_t now = time(NULL);
    lineEdit_LastUsed->setText(getHumanReadableDuration(now - data.mLastUsageTS));
    headerTextLabel_Person->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()));

    // Avatar Loading
    QPixmap pixmap;
    if(data.mImage.mSize == 0 || !GxsIdDetails::loadPixmapFromData(data.mImage.mData, data.mImage.mSize, pixmap, GxsIdDetails::LARGE))
        pixmap = GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId), GxsIdDetails::LARGE);

    avatarLabel->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));

	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
			lineEdit_GpgName->setText(tr("[Unknown node]"));
		else
			lineEdit_GpgName->setText(tr("Anonymous Id"));
	}

	if(data.mPgpId.isNull())
	{
		lineEdit_GpgId->hide() ;
		label_GpgId->hide() ;
	}
	else
	{
		lineEdit_GpgId->show() ;
		label_GpgId->show() ;
	}

    if(data.mPgpKnown)
    {
		lineEdit_GpgName->show() ;
		label_GpgName->show() ;
    }
    else
    {
		lineEdit_GpgName->hide() ;
		label_GpgName->hide() ;
    }

    // Type and Node logic

    bool isLinkedToOwnPgpId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) ;
    bool isOwnId = (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

    if(isOwnId)
        if (isLinkedToOwnPgpId)
            lineEdit_Type->setText(tr("Identity owned by you, linked to your Retroshare node")) ;
        else
            if (data.mMeta.mGroupFlags & (GXS_SERV::FLAG_PRIVACY_PRIVATE | RSGXSID_GROUPFLAG_REALID))
                lineEdit_Type->setText(tr("Identity owned by you, linked to your Retroshare node but not yet validated")) ;
            else
                lineEdit_Type->setText(tr("Anonymous identity, owned by you")) ;
    else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
    {
        if (data.mPgpKnown)
            if (rsPeers->isGPGAccepted(data.mPgpId))
                lineEdit_Type->setText(tr("Linked to a friend Retroshare node")) ;
            else
                lineEdit_Type->setText(tr("Linked to a known Retroshare node")) ;
        else
            lineEdit_Type->setText(tr("Linked to unknown Retroshare node")) ;
    }
    else
    {
        lineEdit_Type->setText(tr("Anonymous identity")) ;
    }

    autoBanIdentities_CB->setChecked(rsReputations->isNodeBanned(data.mPgpId));

	/* now fill in the reputation information */

	RsReputationInfo info;
    rsReputations->getReputationInfo(RsGxsId(data.mMeta.mGroupId),data.mPgpId,info) ;

    QString frep_string ;
    if(info.mFriendsPositiveVotes > 0) frep_string += QString::number(info.mFriendsPositiveVotes) + tr(" positive ") ;
    if(info.mFriendsNegativeVotes > 0) frep_string += QString::number(info.mFriendsNegativeVotes) + tr(" negative ") ;

    if(info.mFriendsPositiveVotes==0 && info.mFriendsNegativeVotes==0)
        frep_string = tr("No votes from friends") ;

    neighborNodesOpinion_TF->setText(frep_string) ;

    label_positive->setText(QString::number(info.mFriendsPositiveVotes));
    label_negative->setText(QString::number(info.mFriendsNegativeVotes));

	switch(info.mOverallReputationLevel)
	{
	case RsReputationLevel::LOCALLY_POSITIVE:
		overallOpinion_TF->setText(tr("Positive")); break;
	case RsReputationLevel::LOCALLY_NEGATIVE:
		overallOpinion_TF->setText(tr("Negative (Banned by you)")); break;
	case RsReputationLevel::REMOTELY_POSITIVE:
		overallOpinion_TF->setText(tr("Positive (according to your friends)"));
		break;
	case RsReputationLevel::REMOTELY_NEGATIVE:
		overallOpinion_TF->setText(tr("Negative (according to your friends)"));
		break;
	case RsReputationLevel::NEUTRAL: // fallthrough
	default:
		overallOpinion_TF->setText(tr("Neutral")) ; break ;
	}

	switch(info.mOwnOpinion)
	{
    case RsOpinion::NEGATIVE: ownOpinion_CB->setCurrentIndex(0); break;
    case RsOpinion::NEUTRAL : ownOpinion_CB->setCurrentIndex(1); break;
    case RsOpinion::POSITIVE: ownOpinion_CB->setCurrentIndex(2); break;
	default:
		std::cerr << "Unexpected value in own opinion: "
		          << static_cast<uint32_t>(info.mOwnOpinion) << std::endl;
		break;
	}

    // Toggle Edit/Invite buttons
    editButton->setVisible(isOwnId);
    inviteButton->setVisible(!isOwnId);
}

void PeopleDialog::onIdentitySelected()
{
    IdentityWidget* widget = qobject_cast<IdentityWidget*>(sender());
    if (widget) {
        // Save the ID for the Invite Button to use later
        mCurrentSelectedId = RsGxsId(widget->groupInfo().mMeta.mGroupId);

        // Update the Usage Statistics page
        UsageStatistics* usageWidget = qobject_cast<UsageStatistics*>(UsagePage);
        if (usageWidget)
            usageWidget->setUsageData(widget->groupInfo());

        clearAllSelections();
        widget->setIsSelected(true);

        // Drive the state machine
        showIdentitySelected(mCurrentSelectedId);
    }
}

void PeopleDialog::clearPerson()
{
	headerTextLabel_Person->setText(tr("People"));
	avatarLabel->clear();
	avatarLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/people.png"));

	lineEdit_GpgId->clear();
	lineEdit_KeyId->clear();
	lineEdit_Type->clear();
	lineEdit_GpgName->clear();
	lineEdit_PublishTS->clear();
	lineEdit_LastUsed->clear();
	neighborNodesOpinion_TF->clear();
	overallOpinion_TF->clear();
	label_positive->clear();
	label_negative->clear();
	ownOpinion_CB->setCurrentIndex(1);
	autoBanIdentities_CB->setChecked(false);
}

void PeopleDialog::toggleStackedPage()
{
    if (widgetExternal->currentIndex() == 0) {
        widgetExternal->setCurrentIndex(1);
    } else {
        widgetExternal->setCurrentIndex(0);
    }
}

void PeopleDialog::toggledetailsStackedPage()
{
	// Toggle between usage stats (2) and details (0 for identity, 1 for circle)
	if (detailsStackedWidget->currentIndex() == 2) {
		detailsStackedWidget->setCurrentIndex(_selectionMode == SelectionMode::Circle ? 1 : 0);
	} else {
		detailsStackedWidget->setCurrentIndex(2);
	}
}

void PeopleDialog::onDetailsPageChanged(int index)
{
	if (index == 2) {
		statsButton->setText(tr("View Details"));
	} else {
		statsButton->setText(tr("View Stats"));
	}
}

void PeopleDialog::modifyReputation()
{
	RsGxsId id(lineEdit_KeyId->text().toStdString());

	RsOpinion op;

	switch(ownOpinion_CB->currentIndex())
	{
	case 0: op = RsOpinion::NEGATIVE; break;
	case 1: op = RsOpinion::NEUTRAL ; break;
	case 2: op = RsOpinion::POSITIVE; break;
	default:
		std::cerr << "Wrong value from opinion combobox. Bug??" << std::endl;
		return;
	}
	rsReputations->setOwnOpinion(id,op);

	return;
}

void PeopleDialog::setVoteControlsVisible(bool visible)
{
	label_YourOpinion->setVisible(visible);
	ownOpinion_CB->setVisible(visible);
	autoBanIdentities_CB->setVisible(visible);
	neighborNodesOpinion_LB->setVisible(visible);
	neighborNodesOpinion_TF->setVisible(visible);
	overallOpinion_TF->setVisible(visible);
	label_PosIcon->setVisible(visible);
	label_positive->setVisible(visible);
	line_Opinion->setVisible(visible);
	label_NegIcon->setVisible(visible);
	label_negative->setVisible(visible);
}

// ─── State machine ────────────────────────────────────────────────────────────

void PeopleDialog::showNoneSelected()
{
	_selectionMode   = SelectionMode::None;
	_selectedGxsId   = RsGxsId();
	_selectedCircleId = RsGxsGroupId();

	backButton->setVisible(false);
	joinLeaveCircleButton->setVisible(false);
	setVoteControlsVisible(true);

	// Rule 2c: show all identities in A, nothing in B, and all circles in C.
	for (auto& [id, w] : _gxs_identity_widgets) {
		w->setStyleSheet(QString());
		w->setVisible(true);
	}

	clearPerson();
	detailsStackedWidget->setCurrentIndex(0);

	// Show all circles in circlesTreeWidget
	{
		QSignalBlocker blocker(circlesTreeWidget);
		for (int i = 0; i < circlesTreeWidget->topLevelItemCount(); ++i) {
			QTreeWidgetItem *item = circlesTreeWidget->topLevelItem(i);
			item->setHidden(false);
			item->setForeground(0, QBrush());
			QFont font = item->font(0);
			font.setBold(false);
			item->setFont(0, font);
		}
		circlesTreeWidget->clearSelection();
	}

	_flowLayoutExt->invalidate();
}

void PeopleDialog::showIdentitySelected(const RsGxsId& id)
{
	_selectionMode  = SelectionMode::Identity;
	_selectedGxsId  = id;

	backButton->setVisible(true);
	joinLeaveCircleButton->setVisible(false);
	setVoteControlsVisible(true);

	// Rule 2a: show identity details in B and the circles the identity belongs to in C
	for (auto& [gid, w] : _gxs_identity_widgets) {
		w->setVisible(true);
		w->setStyleSheet(gid == id
			? QStringLiteral("border: 2px solid #2196F3; border-radius: 4px;")
			: QString());
	}

	auto it = _gxs_identity_widgets.find(id);
	if (it != _gxs_identity_widgets.end())
		loadIdentityLabels(it->second->groupInfo());
	detailsStackedWidget->setCurrentIndex(0);

	// Show ONLY circles the identity belongs to in C (and highlight green)
	{
		QSignalBlocker blocker(circlesTreeWidget);
		for (int i = 0; i < circlesTreeWidget->topLevelItemCount(); ++i) {
			QTreeWidgetItem *item = circlesTreeWidget->topLevelItem(i);
			RsGxsGroupId circleId(item->data(0, Qt::UserRole).toString().toStdString());
			auto cit = _ext_circles_widgets.find(circleId);
			bool member = false;
			if (cit != _ext_circles_widgets.end()) {
				member = (cit->second->circleDetails().mAllowedGxsIds.count(id) > 0);
			}
			item->setHidden(!member);
			if (member) {
				item->setForeground(0, QBrush(QColor("#4CAF50")));
				QFont font = item->font(0);
				font.setBold(true);
				item->setFont(0, font);
			}
		}
		circlesTreeWidget->clearSelection();
	}

	_flowLayoutExt->invalidate();
}

void PeopleDialog::showCircleSelected(const RsGxsGroupId& circleId)
{
	_selectionMode    = SelectionMode::Circle;
	_selectedCircleId = circleId;

	backButton->setVisible(true);
	editButton->setVisible(false);
	inviteButton->setVisible(false);
	joinLeaveCircleButton->setVisible(true);
	setVoteControlsVisible(false);

	auto it = _ext_circles_widgets.find(circleId);
	if (it == _ext_circles_widgets.end()) return;

	CircleWidget* cw = it->second;
	const RsGxsCircleDetails& details = cw->circleDetails();

	// Rule 2b: show in A the identities in that circle (with color code for member)
	for (auto& [gid, w] : _gxs_identity_widgets) {
		const bool isMember = (details.mAllowedGxsIds.count(gid) > 0);
		w->setVisible(isMember);
		w->setStyleSheet(isMember
			? QStringLiteral("border: 2px solid #4CAF50; border-radius: 4px;")
			: QString());
	}

	// Circle details in B
	loadCircleLabels(circleId);
	detailsStackedWidget->setCurrentIndex(1);

	// All circles visible in C, select current
	{
		QSignalBlocker blocker(circlesTreeWidget);
		QString targetIdStr = QString::fromStdString(circleId.toStdString());
		for (int i = 0; i < circlesTreeWidget->topLevelItemCount(); ++i) {
			QTreeWidgetItem *item = circlesTreeWidget->topLevelItem(i);
			item->setHidden(false);
			if (item->data(0, Qt::UserRole).toString() == targetIdStr) {
				circlesTreeWidget->setCurrentItem(item);
			}
		}
	}

	_flowLayoutExt->invalidate();
}

void PeopleDialog::loadCircleLabels(const CircleWidget* cw)
{
	if (!cw) return;
	loadCircleLabels(cw->groupInfo().mGroupId);
}

void PeopleDialog::loadCircleLabels(const RsGxsGroupId& circleId)
{
	auto it = _ext_circles_widgets.find(circleId);
	if (it == _ext_circles_widgets.end()) return;

	const RsGroupMetaData&    info    = it->second->groupInfo();
	const RsGxsCircleDetails& details = it->second->circleDetails();

	headerTextLabel_Person->setText(QString::fromUtf8(info.mGroupName.c_str()));
	circleNameEdit->setText(QString::fromUtf8(info.mGroupName.c_str()));
	circleIdEdit->setText(QString::fromStdString(info.mGroupId.toStdString()));

	avatarLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/circles.png"));

	const bool isExternal = (details.mCircleType == RsGxsCircleType::EXTERNAL);
	circleTypeEdit->setText(isExternal ? tr("External") : tr("Personal"));

	circleMemberCountEdit->setText(QString::number(details.mAllowedGxsIds.size()));

	// Join / leave button label & state
	const bool isMember = details.mAmIAllowed;
	const bool isRequested = _requestedCircles.count(circleId) > 0;

	if (isMember) {
		joinLeaveCircleButton->setEnabled(true);
		joinLeaveCircleButton->setText(tr("Leave Circle"));
		joinLeaveCircleButton->setToolTip(tr("Cancel circle membership"));
	} else if (isRequested) {
		joinLeaveCircleButton->setEnabled(false);
		joinLeaveCircleButton->setText(tr("Requested"));
		joinLeaveCircleButton->setToolTip(tr("Membership request sent"));
	} else {
		joinLeaveCircleButton->setEnabled(true);
		joinLeaveCircleButton->setText(tr("Request"));
		joinLeaveCircleButton->setToolTip(tr("Request to join"));
	}
}

void PeopleDialog::onCircleSelected()
{
	CircleWidget* w = qobject_cast<CircleWidget*>(sender());
	if (!w) return;
	clearAllSelections();
	w->setIsSelected(true);
	showCircleSelected(w->groupInfo().mGroupId);
}

void PeopleDialog::onCircleTreeItemClicked(QTreeWidgetItem *item, int /*column*/)
{
	if (!item) return;

	// Ignore right-clicks so right-clicking opens context menu without switching view
	if (QApplication::mouseButtons() & Qt::RightButton) return;

	QString groupIdStr = item->data(0, Qt::UserRole).toString();
	if (!groupIdStr.isEmpty()) {
		RsGxsGroupId groupId(groupIdStr.toStdString());
		showCircleSelected(groupId);
	}
}

void PeopleDialog::onCircleTreeContextMenuRequested(const QPoint &pos)
{
	QTreeWidgetItem *item = circlesTreeWidget->itemAt(pos);
	if (!item) return;

	QString groupIdStr = item->data(0, Qt::UserRole).toString();
	RsGxsGroupId groupId(groupIdStr.toStdString());

	// Check if local user is Admin of this circle
	bool isAdmin = false;
	auto it = _ext_circles_widgets.find(groupId);
	if (it != _ext_circles_widgets.end()) {
		isAdmin = (it->second->groupInfo().mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) != 0;
	}

	QMenu menu(this);
	QAction *actAction = nullptr;
	if (isAdmin) {
		actAction = menu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/pencil-edit-button.png"), tr("Edit Circle"));
	} else {
		actAction = menu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/info16.png"), tr("View Details"));
	}

	QAction *actJoinLeave = menu.addAction(tr("Request to Join / Leave"));

	menu.addSeparator();
	QAction *actCopyLink = menu.addAction(tr("Copy retroshare link"));

	QAction *selected = menu.exec(circlesTreeWidget->mapToGlobal(pos));
	if (selected == actAction) {
		CreateCircleDialog dlg;
		dlg.editExistingId(groupId, true, !isAdmin); // readonly = false if Admin (Edit Circle), readonly = true if Non-Admin (View Details)
		dlg.exec();
	} else if (selected == actJoinLeave) {
		_selectedCircleId = groupId;
		requestJoinLeaveCircle();
	} else if (selected == actCopyLink) {
		RsGxsCircleDetails details;
		if (rsGxsCircles && rsGxsCircles->getCircleDetails(RsGxsCircleId(groupId), details)) {
			QList<RetroShareLink> urls;
			RetroShareLink link = RetroShareLink::createCircle(RsGxsCircleId(groupId), QString::fromUtf8(details.mCircleName.c_str()));
			urls.push_back(link);
			RSLinkClipboard::copyLinks(urls);
		}
	}
}

void PeopleDialog::onBackClicked()
{
	clearAllSelections();
	showNoneSelected();
}

void PeopleDialog::requestJoinLeaveCircle()
{
	auto it = _ext_circles_widgets.find(_selectedCircleId);
	if (it == _ext_circles_widgets.end()) return;

	const RsGxsCircleDetails& details = it->second->circleDetails();
	const bool isMember = details.mAmIAllowed;

	// Pick the first own GXS identity (same approach used elsewhere in the dialog)
	std::list<RsGxsId> ownIds;
	rsIdentity->getOwnIds(ownIds);
	if (ownIds.empty()) {
		std::cerr << "PeopleDialog::requestJoinLeaveCircle() No own identity available." << std::endl;
		return;
	}
	const RsGxsId ownId = ownIds.front();

	if (isMember) {
		rsGxsCircles->cancelCircleMembership(ownId, RsGxsCircleId(_selectedCircleId));
		_requestedCircles.erase(_selectedCircleId);
		joinLeaveCircleButton->setEnabled(true);
		joinLeaveCircleButton->setText(tr("Request"));
		joinLeaveCircleButton->setToolTip(tr("Request to join"));
	} else {
		rsGxsCircles->requestCircleMembership(ownId, RsGxsCircleId(_selectedCircleId));
		_requestedCircles.insert(_selectedCircleId);
		joinLeaveCircleButton->setEnabled(false);
		joinLeaveCircleButton->setText(tr("Requested"));
		joinLeaveCircleButton->setToolTip(tr("Membership request sent"));
	}
}

