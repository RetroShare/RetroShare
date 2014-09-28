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

#include <QMessageBox>

#include "PeopleDialog.h"
#include "gui/Circles/CreateCircleDialog.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/common/FlowLayout.h"
#include "gui/common/UIStateHelper.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxscircles.h>
#include "retroshare/rsgxsflags.h"

//#include "IdentityItem.h"
//#include "CircleItem.h"

#include <iostream>

/******
 * #define ID_DEBUG 1
 *****/

// Data Requests.
#define IDDIALOG_IDLIST		1
#define IDDIALOG_IDDETAILS	2
#define IDDIALOG_REPLIST	3
#define IDDIALOG_REFRESH	4

/****************************************************************
 */

#define RSID_COL_NICKNAME   0
#define RSID_COL_KEYID      1
#define RSID_COL_IDTYPE     2

#define RSIDREP_COL_NAME       0
#define RSIDREP_COL_OPINION    1
#define RSIDREP_COL_COMMENT    2
#define RSIDREP_COL_REPUTATION 3

#define RSID_FILTER_YOURSELF     0x0001
#define RSID_FILTER_FRIENDS      0x0002
#define RSID_FILTER_OTHERS       0x0004
#define RSID_FILTER_PSEUDONYMS   0x0008
#define RSID_FILTER_ALL          0xffff

const uint32_t PeopleDialog::PD_IDLIST    = 0x0001 ;
const uint32_t PeopleDialog::PD_IDDETAILS = 0x0002 ;
const uint32_t PeopleDialog::PD_REFRESH   = 0x0003 ;
const uint32_t PeopleDialog::PD_CIRCLES   = 0x0004 ;

/** Constructor */
PeopleDialog::PeopleDialog(QWidget *parent)
	: RsGxsUpdateBroadcastPage(rsIdentity, parent)
{
	setupUi(this);

	mStateHelper = new UIStateHelper(this);
	mIdentityQueue = new TokenQueue(rsIdentity->getTokenService(), this);
	mCirclesQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);

	//need erase QtCreator Layout first(for Win)
	delete id->layout();
	//QT Designer don't accept Custom Layout, maybe on QT5
	_flowLayout = new FlowLayout(id);

	//First Get Item created in Qt Designer
	int count = id->children().count();
	for (int curs = 0; curs < count; ++curs){
		QObject *obj = id->children().at(curs);
		QWidget *wid = qobject_cast<QWidget *>(obj);
		if (wid) _flowLayout->addWidget(wid);
	}//for (int curs = 0; curs < count; ++curs)

	pictureFlowWidget->setAcceptDrops(true);
	QObject::connect(pictureFlowWidget, SIGNAL(centerIndexChanged(int)), this, SLOT(pf_centerIndexChanged(int)));
	QObject::connect(pictureFlowWidget, SIGNAL(mouseMoveOverSlideEvent(QMouseEvent*,int)), this, SLOT(pf_mouseMoveOverSlideEvent(QMouseEvent*,int)));
	QObject::connect(pictureFlowWidget, SIGNAL(dragEnterEventOccurs(QDragEnterEvent*)), this, SLOT(pf_dragEnterEventOccurs(QDragEnterEvent*)));
	QObject::connect(pictureFlowWidget, SIGNAL(dragMoveEventOccurs(QDragMoveEvent*)), this, SLOT(pf_dragMoveEventOccurs(QDragMoveEvent*)));
	QObject::connect(pictureFlowWidget, SIGNAL(dropEventOccurs(QDropEvent*)), this, SLOT(pf_dropEventOccurs(QDropEvent*)));
	pictureFlowWidget->setMinimumHeight(60);
	pictureFlowWidget->setSlideSizeRatio(4/4.0);

#if 0
	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
	mStateHelper->addWidget(IDDIALOG_IDLIST, ui.treeWidget_IdList);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDLIST, ui.treeWidget_IdList, false);
	mStateHelper->addClear(IDDIALOG_IDLIST, ui.treeWidget_IdList);

	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_Nickname);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_KeyId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.toolButton_Reputation);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.toolButton_Delete);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.toolButton_EditId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingOwn);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingPeers);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repModButton);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Accept);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Ban);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Negative);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Positive);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Custom);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_spinBox);

	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_KeyId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingOwn);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingPeers);

	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_Nickname);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_KeyId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingOwn);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingPeers);

	//mStateHelper->addWidget(IDDIALOG_REPLIST, ui.treeWidget_RepList);
	//mStateHelper->addLoadPlaceholder(IDDIALOG_REPLIST, ui.treeWidget_RepList);
	//mStateHelper->addClear(IDDIALOG_REPLIST, ui.treeWidget_RepList);

	/* Connect signals */
	connect(ui.toolButton_NewId, SIGNAL(clicked()), this, SLOT(addIdentity()));
	connect(ui.todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));
	connect(ui.toolButton_EditId, SIGNAL(clicked()), this, SLOT(editIdentity()));
	connect(ui.treeWidget_IdList, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));

	connect(ui.filterComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterComboBoxChanged()));
	connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
	connect(ui.repModButton, SIGNAL(clicked()), this, SLOT(modifyReputation()));

	/* Add filter types */
	ui.filterComboBox->addItem(tr("All"), RSID_FILTER_ALL);
	ui.filterComboBox->addItem(tr("Yourself"), RSID_FILTER_YOURSELF);
	ui.filterComboBox->addItem(tr("Friends / Friends of Friends"), RSID_FILTER_FRIENDS);
	ui.filterComboBox->addItem(tr("Others"), RSID_FILTER_OTHERS);
	ui.filterComboBox->addItem(tr("Pseudonyms"), RSID_FILTER_PSEUDONYMS);
	ui.filterComboBox->setCurrentIndex(0);

	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui.treeWidget_IdList->headerItem();
	QString headerText = headerItem->text(RSID_COL_NICKNAME);
	ui.filterLineEdit->addFilter(QIcon(), headerText, RSID_COL_NICKNAME, QString("%1 %2").arg(tr("Search"), headerText));
	headerText = headerItem->text(RSID_COL_KEYID);
	ui.filterLineEdit->addFilter(QIcon(), headerItem->text(RSID_COL_KEYID), RSID_COL_KEYID, QString("%1 %2").arg(tr("Search"), headerText));

	/* Setup tree */
	ui.treeWidget_IdList->sortByColumn(RSID_COL_NICKNAME, Qt::AscendingOrder);

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

	mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
	mStateHelper->setActive(IDDIALOG_REPLIST, false);

	// Hiding RepList until that part is finished.
	//ui.treeWidget_RepList->setVisible(false);
	ui.toolButton_Reputation->setVisible(false);
#endif
}

void PeopleDialog::updateDisplay(bool complete)
{
	Q_UNUSED(complete);
	/* Update identity list */
	requestIdList();
	requestCirclesList();

	/* grab all ids */
	std::list<RsPgpId> friend_pgpIds;
	std::list<RsPgpId> all_pgpIds;
	std::list<RsPgpId>::iterator it;
	std::set<RsPgpId> friend_set;

	rsPeers->getGPGAcceptedList(friend_pgpIds);
	rsPeers->getGPGAllList(all_pgpIds);

	for(it = friend_pgpIds.begin(); it != friend_pgpIds.end(); ++it) {
		friend_set.insert(*it);
	}//for(it = friend_pgpIds.begin(); it != friend_pgpIds.end(); ++it)
	for(it = all_pgpIds.begin(); it != all_pgpIds.end(); ++it) {
		if (friend_set.find(*it) != friend_set.end()) continue;// already added as a friend.
		friend_set.insert(*it);
	}//for(it = all_pgpIds.begin(); it != all_pgpIds.end(); ++it)

	for(std::set<RsPgpId>::iterator it = friend_set.begin()
	    ; it != friend_set.end()
	    ;++it){
		RsPeerDetails details;
		if (rsPeers->getGPGDetails(*it, details)) {
			std::map<RsPgpId,IdentityWidget *>::iterator itFound;
			if((itFound=_pgp_identity_widgets.find(*it)) == _pgp_identity_widgets.end()) {
				std::cerr << "Loading pgp identity ID = " << it->toStdString() << std::endl;

				IdentityWidget *new_item = new IdentityWidget(details) ;
				_pgp_identity_widgets[*it] = new_item ;

				QObject::connect(new_item, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)));
				_flowLayout->addWidget(new_item);
			}//if((itFound=_pgp_identity_widgets.find(*it)) == _pgp_identity_widgets.end())
		}//if (rsPeers->getGPGDetails(*it, details))
	}//for(std::set<RsPgpId>::iterator it = friend_set.begin()

}

void PeopleDialog::insertIdList(uint32_t token)
{
	std::cerr << "**** In insertIdList() ****" << std::endl;
	mStateHelper->setLoading(PD_IDLIST, false);

	std::vector<RsGxsIdGroup> gdataVector;
	std::vector<RsGxsIdGroup>::iterator gdIt;

	if (!rsIdentity->getGroupData(token, gdataVector)) {
		std::cerr << "PeopleDialog::insertIdList() Error getting GroupData";
		std::cerr << std::endl;

		mStateHelper->setLoading(PD_IDDETAILS, false);
		mStateHelper->setLoading(PD_CIRCLES, false);

		mStateHelper->setActive(PD_IDLIST, false);
		mStateHelper->setActive(PD_IDDETAILS, false);
		mStateHelper->setActive(PD_CIRCLES, false);

		mStateHelper->clear(PD_IDLIST);
		mStateHelper->clear(PD_IDDETAILS);
		mStateHelper->clear(PD_CIRCLES);

		return;
	}//if (!rsIdentity->getGroupData(token, gdataVector))

	mStateHelper->setActive(PD_IDLIST, true);

	//RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	/* Insert items */
	int i=0 ;
	for (gdIt = gdataVector.begin(); gdIt != gdataVector.end(); ++gdIt){
		RsGxsIdGroup gdItem = (*gdIt);

		std::map<RsGxsId,IdentityWidget *>::iterator itFound;
		if((itFound=_gxs_identity_widgets.find(RsGxsId(gdItem.mMeta.mGroupId))) == _gxs_identity_widgets.end()) {
			std::cerr << "Loading data vector identity ID = " << gdItem.mMeta.mGroupId << ", i="<< i << std::endl;

			IdentityWidget *new_item = new IdentityWidget(gdItem) ;
			_gxs_identity_widgets[RsGxsId(gdItem.mMeta.mGroupId)] = new_item ;

			QObject::connect(new_item, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)));
			_flowLayout->addWidget(new_item);
			++i ;
		} else {//if((itFound=_identity_widgets.find(gdItem.mMeta.mGroupId)) == _identity_widgets.end())

			std::cerr << "Updating data vector identity ID = " << gdItem.mMeta.mGroupId << std::endl;
			//TODO
			IdentityWidget *idWidget = itFound->second;
			idWidget->setName("TODO updated");
			//RsGxsIdGroup idGroup = gdIt;
			//idWidget->update(idGroup);

		}//else (_identity_widgets.find((*vit).mMeta.mGroupId) == _identity_widgets.end())
	}//for (gdIt = gdataVector.begin(); gdIt != gdataVector.end(); ++gdIt)
}

void PeopleDialog::insertCircles(uint32_t token)
{
	std::cerr << "PeopleDialog::insertCircles(token==" << token << ")" << std::endl;
	mStateHelper->setLoading(PD_CIRCLES, false);

	std::list<RsGroupMetaData> gSummaryList;
	std::list<RsGroupMetaData>::iterator gsIt;

	if (!rsGxsCircles->getGroupSummary(token,gSummaryList)) {
		std::cerr << "PeopleDialog::insertCircles() Error getting GroupSummary";
		std::cerr << std::endl;

		mStateHelper->setActive(PD_CIRCLES, false);

		return;
	}//if (!rsGxsCircles->getGroupSummary(token,gSummaryList))

	mStateHelper->setActive(PD_CIRCLES, true);

	/* add the top level item */
	for(gsIt = gSummaryList.begin(); gsIt != gSummaryList.end(); gsIt++) {
		RsGroupMetaData gsItem = (*gsIt);

		RsGxsCircleDetails details ;
		if(!rsGxsCircles->getCircleDetails(RsGxsCircleId(gsItem.mGroupId), details)){
			std::cerr << "(EE) Cannot get details for circle id " << gsItem.mGroupId << ". Circle item is not created!" << std::endl;
			continue ;
		}//if(!rsGxsCircles->getCircleDetails(RsGxsCircleId(git->mGroupId), details))

		std::map<RsGxsGroupId, CircleWidget*>::iterator itFound;
		if((itFound=_circles_widgets.find(gsItem.mGroupId)) == _circles_widgets.end()) {
			std::cerr << "PeopleDialog::insertCircles() add new GroupId: " << gsItem.mGroupId;
			std::cerr << " GroupName: " << gsItem.mGroupName;
			std::cerr << std::endl;

			CircleWidget *gitem = new CircleWidget() ;
			QObject::connect(gitem, SIGNAL(flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)), this, SLOT(fl_flowLayoutItemDropped(QList<FlowLayoutItem*>,bool&)));
			QObject::connect(gitem, SIGNAL(askForGXSIdentityWidget(RsGxsId)), this, SLOT(cw_askForGXSIdentityWidget(RsGxsId)));
			QObject::connect(gitem, SIGNAL(askForPGPIdentityWidget(RsGxsId)), this, SLOT(cw_askForPGPIdentityWidget(RsPgpId)));
			gitem->updateData( gsItem, details );
			_circles_widgets[gsItem.mGroupId] = gitem ;


			_flowLayout->addWidget(gitem);

			QPixmap pixmap = gitem->getImage();
			pictureFlowWidget->addSlide( pixmap );
			_listCir << gitem;
		} else {//if((itFound=_circles_widgets.find(gsItem.mGroupId)) == _circles_widgets.end())
			std::cerr << "PeopleDialog::insertCircles() Update GroupId: " << gsItem.mGroupId;
			std::cerr << " GroupName: " << gsItem.mGroupName;
			std::cerr << std::endl;

			//TODO
			CircleWidget *cirWidget = itFound->second;
			cirWidget->setName("TODO updated");
			//cirWidget->update(gsItem, details);
			int index = _listCir.indexOf(cirWidget);
			QPixmap pixmap = cirWidget->getImage();
			pictureFlowWidget->setSlide(index, pixmap);
		}//if((item=_circles_items.find(gsItem.mGroupId)) == _circles_items.end())
	}//for(gsIt = gSummaryList.begin(); gsIt != gSummaryList.end(); gsIt++)
}

void PeopleDialog::requestIdList()
{
	std::cerr << "Requesting ID list..." << std::endl;

	if (!mIdentityQueue) return;

	mStateHelper->setLoading(PD_IDLIST,    true);
	//mStateHelper->setLoading(PD_IDDETAILS, true);
	//mStateHelper->setLoading(PD_REPLIST,   true);

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

	mStateHelper->setLoading(PD_CIRCLES,    true);
	//mStateHelper->setLoading(PD_IDDETAILS, true);
	//mStateHelper->setLoading(PD_REPLIST,   true);

	mCirclesQueue->cancelActiveRequestTokens(PD_CIRCLES);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mCirclesQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, PD_CIRCLES);
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
	}//if (dest)
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
	}//if (dest)
}

void PeopleDialog::fl_flowLayoutItemDropped(QList<FlowLayoutItem *>flListItem, bool &bAccept)
{
	bAccept=false;
	bool bCreateNewCircle=false;
	bool bIsExternal=false;//External if one at least is unknow or external
	bool bDestCirIsLocal=false;
	FlowLayoutItem *dest =
	    qobject_cast<FlowLayoutItem *>(QObject::sender());
	if (dest) {
		CreateCircleDialog dlg;

		CircleWidget* cirDest = qobject_cast<CircleWidget*>(dest);
		if (cirDest) {
			bDestCirIsLocal = (cirDest->groupInfo().mCircleType == GXS_CIRCLE_TYPE_LOCAL);
			bIsExternal |= ! bDestCirIsLocal;
			dlg.addCircle(cirDest->circleDetails());
		} else {//if (cirDest)
			bCreateNewCircle=true;
		}//else (cirDest)

		IdentityWidget* idDest = qobject_cast<IdentityWidget*>(dest);
		if (idDest) {
			if (idDest->isGXS()){
				bIsExternal |= ! idDest->groupInfo().mPgpKnown;
				dlg.addMember(idDest->groupInfo());
			} else {//if (idDest->isGXS())
				///TODO: How to get RSGxsIdGrop from pgp???
				//dlg.addMember(idDest->details().id);

			}//else (idDest->isGXS())
		}//if (idDest)

		typedef QList<FlowLayoutItem *>::Iterator itList;
		for (itList listCurs = flListItem.begin()
		     ; listCurs != flListItem.end()
		     ; ++listCurs) {
			FlowLayoutItem *flCurs = *listCurs;
			CircleWidget* cirDropped = qobject_cast<CircleWidget*>(flCurs);
			//Create new circle if circle dropped in circle or ident
			if (cirDropped) {
				bCreateNewCircle = true;
				bIsExternal |= (cirDropped->groupInfo().mCircleType != GXS_CIRCLE_TYPE_LOCAL);
				dlg.addCircle(cirDropped->circleDetails());

			} else {//if (cirDropped)
				IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
				if (idDropped){
					bIsExternal |= ! idDropped->groupInfo().mPgpKnown;
					dlg.addMember(idDropped->groupInfo());

				}//if (idDropped)
			}//else (cirDropped)

		}//for (itList listCurs = flListItem.begin()

		bCreateNewCircle |= (bIsExternal && bDestCirIsLocal);
		if (bCreateNewCircle){
			dlg.editNewId(bIsExternal);
		} else {//if (bCreateNewCircle)
			dlg.editExistingId(cirDest->groupInfo().mGroupId);
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

void PeopleDialog::pf_dropEventOccurs(QDropEvent *event)
{
	bool bCreateNewCircle=false;
	bool bIsExternal=false;//External if one at least is unknow or external
	bool bDestCirIsLocal=false;
	bool atLeastOne = false;

	int index = pictureFlowWidget->centerIndex();
	CircleWidget* cirDest = _listCir[index];
	if (cirDest) {
		CreateCircleDialog dlg;

		bDestCirIsLocal = (cirDest->groupInfo().mCircleType == GXS_CIRCLE_TYPE_LOCAL);
		bIsExternal |= ! bDestCirIsLocal;
		dlg.addCircle(cirDest->circleDetails());

		{//Test if source is only one FlowLayoutItem
			FlowLayoutItem *flCurs =
			    qobject_cast<FlowLayoutItem *>(event->source());
			if (flCurs) {
				CircleWidget* cirDropped = qobject_cast<CircleWidget*>(flCurs);
				//Create new circle if circle dropped in circle or ident
				if (cirDropped) {
					bCreateNewCircle = true;
					bIsExternal |= (cirDropped->groupInfo().mCircleType != GXS_CIRCLE_TYPE_LOCAL);
					dlg.addCircle(cirDropped->circleDetails());
					atLeastOne = true;

				} else {//if (cirDropped)
					IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
					if (idDropped){
						if (idDropped->isGXS()){
							bIsExternal |= ! idDropped->groupInfo().mPgpKnown;
							dlg.addMember(idDropped->groupInfo());
							atLeastOne = true;
						} else {//if (idDropped->isGXS())
							///TODO: How to get RSGxsIdGrop from pgp???
							//dlg.addMember(idDropped->details().id);
							//atLeastOne = true;

						}//else (idDropped->isGXS())

					}//if (idDropped)
				}//else (cirDropped)

			}//if (flCurs)
		}//End Test if source is only one IdentityWidget

		QWidget *wid =
		    qobject_cast<QWidget *>(event->source());//QT5 return QObject
		FlowLayout *layout;
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
							bIsExternal |= (cirDropped->groupInfo().mCircleType != GXS_CIRCLE_TYPE_LOCAL);
							dlg.addCircle(cirDropped->circleDetails());
							atLeastOne = true;

						} else {//if (cirDropped)
							IdentityWidget* idDropped = qobject_cast<IdentityWidget*>(flCurs);
							if (idDropped){
								bIsExternal |= ! idDropped->groupInfo().mPgpKnown;
								dlg.addMember(idDropped->groupInfo());
								atLeastOne = true;

							}//if (idDropped)
						}//else (cirDropped)

					}//if (flCurs)
				}//if (layoutItem)
			}//for (int curs = 0; curs < count; ++curs)
		}//if (layout)

		if (atLeastOne) {
			bCreateNewCircle |= (bIsExternal && bDestCirIsLocal);
			if (bCreateNewCircle){
				dlg.editNewId(bIsExternal);
			} else {//if (bCreateNewCircle)
				dlg.editExistingId(cirDest->groupInfo().mGroupId);
			}//else (bCreateNewCircle)

			dlg.exec();

			event->setDropAction(Qt::CopyAction);
			event->accept();
		}//if (atLeastOne)
	}//if (cirDest)
}

void PeopleDialog::populatePictureFlow()
{
	std::map<RsGxsGroupId,CircleWidget *>::iterator it;
	for (it=_circles_widgets.begin(); it!=_circles_widgets.end(); ++it) {
		CircleWidget *item = it->second;
		QPixmap pixmap = item->getImage();
		pictureFlowWidget->addSlide( pixmap );
	}//for (it=_circles_items.begin(); it!=_circles_items.end(); ++it)
	pictureFlowWidget->setSlideSizeRatio(4/4.0);
}







#if 0
void IdDialog::todo()
{
	QMessageBox::information(this, "Todo",
							 "<b>Open points:</b><ul>"
							 "<li>Delete ID"
							 "<li>Reputation"
							 "<li>Load/save settings"
							 "</ul>");
}

void IdDialog::filterComboBoxChanged()
{
	requestIdList();
}

void IdDialog::filterChanged(const QString& /*text*/)
{
	filterIds();
}

void IdDialog::updateSelection()
{
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
    RsGxsGroupId id;

	if (item)
	{
        id = RsGxsGroupId(item->text(RSID_COL_KEYID).toStdString());
	}

	requestIdDetails(id);
}

bool IdDialog::fillIdListItem(const RsGxsIdGroup& data, QTreeWidgetItem *&item, const RsPgpId &ownPgpId, int accept)
{
	bool isOwnId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) || (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

	/* do filtering */
	bool ok = false;
	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (isOwnId && (accept & RSID_FILTER_YOURSELF))
		{
			ok = true;
		}
		else
		{
			if (data.mPgpKnown)
			{
				if (accept & RSID_FILTER_FRIENDS)
				{
					ok = true;
				}
			}
			else
			{
				if (accept & RSID_FILTER_OTHERS)
				{
					ok = true;
				}
			}
		}
	}
	else
	{
		if (accept & RSID_FILTER_PSEUDONYMS)
		{
			ok = true;
		}

		if (isOwnId && (accept & RSID_FILTER_YOURSELF))
		{
			ok = true;
		}
	}

	if (!ok)
	{
		return false;
	}

	if (!item)
	{
		item = new QTreeWidgetItem();
	}
	item->setText(RSID_COL_NICKNAME, QString::fromUtf8(data.mMeta.mGroupName.c_str()));
    item->setText(RSID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId.toStdString()));

	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (data.mPgpKnown)
		{
			RsPeerDetails details;
			rsPeers->getGPGDetails(data.mPgpId, details);
			item->setText(RSID_COL_IDTYPE, QString::fromUtf8(details.name.c_str()));
		}
		else
		{
			item->setText(RSID_COL_IDTYPE, tr("PGP Linked Id"));
		}
	}
	else
	{
		item->setText(RSID_COL_IDTYPE, tr("Anon Id"));
	}

	return true;
}

void IdDialog::requestIdDetails(RsGxsGroupId &id)
{
	mIdQueue->cancelActiveRequestTokens(IDDIALOG_IDDETAILS);

    if (id.isNull())
	{
		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->setLoading(IDDIALOG_IDDETAILS, false);
		mStateHelper->setLoading(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);
		mStateHelper->clear(IDDIALOG_REPLIST);

		return;
	}

	mStateHelper->setLoading(IDDIALOG_IDDETAILS, true);
	mStateHelper->setLoading(IDDIALOG_REPLIST, true);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
    std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(id);

	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDDIALOG_IDDETAILS);
}

void IdDialog::insertIdDetails(uint32_t token)
{
	mStateHelper->setLoading(IDDIALOG_IDDETAILS, false);

	/* get details from libretroshare */
	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	if (!rsIdentity->getGroupData(token, datavector))
	{
		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->setLoading(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);
		mStateHelper->clear(IDDIALOG_REPLIST);

		ui.lineEdit_KeyId->setText("ERROR GETTING KEY!");

		return;
	}

	if (datavector.size() != 1)
	{
		std::cerr << "IdDialog::insertIdDetails() Invalid datavector size";

		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->setLoading(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);
		mStateHelper->clear(IDDIALOG_REPLIST);

		ui.lineEdit_KeyId->setText("INVALID DV SIZE");

		return;
	}

	mStateHelper->setActive(IDDIALOG_IDDETAILS, true);

	data = datavector[0];

	/* get GPG Details from rsPeers */
	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	ui.lineEdit_Nickname->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()));
    ui.lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId.toStdString()));
	ui.lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash.toStdString()));
	ui.lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()));

	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		ui.lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			ui.lineEdit_GpgName->setText(tr("Unknown real name"));
		}
		else
		{
			ui.lineEdit_GpgName->setText(tr("Anonymous Id"));
		}
	}

	bool isOwnId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) || (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

	if (isOwnId)
	{
		ui.radioButton_IdYourself->setChecked(true);
	}
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (data.mPgpKnown)
		{
			if (rsPeers->isGPGAccepted(data.mPgpId))
			{
				ui.radioButton_IdFriend->setChecked(true);
			}
			else
			{
				ui.radioButton_IdFOF->setChecked(true);
			}
		}
		else
		{
			ui.radioButton_IdOther->setChecked(true);
		}
	}
	else
	{
		ui.radioButton_IdPseudo->setChecked(true);
	}

	if (isOwnId)
	{
		mStateHelper->setWidgetEnabled(ui.toolButton_Reputation, false);
		// No Delete Ids yet!
		mStateHelper->setWidgetEnabled(ui.toolButton_Delete, /*true*/ false);
		mStateHelper->setWidgetEnabled(ui.toolButton_EditId, true);
	}
	else
	{
		// No Reputation yet!
		mStateHelper->setWidgetEnabled(ui.toolButton_Reputation, /*true*/ false);
		mStateHelper->setWidgetEnabled(ui.toolButton_Delete, false);
		mStateHelper->setWidgetEnabled(ui.toolButton_EditId, false);
	}

	/* now fill in the reputation information */
	ui.line_RatingOverall->setText("Overall Rating TODO");
	ui.line_RatingOwn->setText("Own Rating TODO");

	if (data.mPgpKnown)
	{
		ui.line_RatingImplicit->setText("+50 Known PGP");
	}
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		ui.line_RatingImplicit->setText("+10 UnKnown PGP");
	}
	else
	{
		ui.line_RatingImplicit->setText("+5 Anon Id");
	}

	{
		QString rating = QString::number(data.mReputation.mOverallScore);
		ui.line_RatingOverall->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mIdScore);
		ui.line_RatingImplicit->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mOwnOpinion);
		ui.line_RatingOwn->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mPeerOpinion);
		ui.line_RatingPeers->setText(rating);
	}

	/* request network ratings */
	// Removing this for the moment.
	// requestRepList(data.mMeta.mGroupId);
}

void IdDialog::modifyReputation()
{
	std::cerr << "IdDialog::modifyReputation()";
	std::cerr << std::endl;

	RsGxsId id(ui.lineEdit_KeyId->text().toStdString());

	int mod = 0;
	if (ui.repMod_Accept->isChecked())
	{
		mod += 100;
	}
	else if (ui.repMod_Positive->isChecked())
	{
		mod += 10;
	}
	else if (ui.repMod_Negative->isChecked())
	{
		mod += -10;
	}
	else if (ui.repMod_Ban->isChecked())
	{
		mod += -100;
	}
	else if (ui.repMod_Custom->isChecked())
	{
		mod += ui.repMod_spinBox->value();
	}
	else
	{
		// invalid
		return;
	}

	std::cerr << "IdDialog::modifyReputation() ID: " << id << " Mod: " << mod;
	std::cerr << std::endl;

	uint32_t token;
	if (!rsIdentity->submitOpinion(token, id, false, mod))
	{
		std::cerr << "IdDialog::modifyReputation() Error submitting Opinion";
		std::cerr << std::endl;

	}

	std::cerr << "IdDialog::modifyReputation() queuingRequest(), token: " << token;
	std::cerr << std::endl;

	// trigger refresh when finished.
	// basic / anstype are not needed.
	mIdQueue->queueRequest(token, 0, 0, IDDIALOG_REFRESH);

	return;
}
	
void IdDialog::addIdentity()
{
	IdEditDialog dlg(this);
	dlg.setupNewId(false);
	dlg.exec();
}

void IdDialog::editIdentity()
{
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
	if (!item)
	{
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
		return;
	}

	std::string keyId = item->text(RSID_COL_KEYID).toStdString();

	IdEditDialog dlg(this);
	dlg.setupExistingId(keyId);
	dlg.exec();
}

void IdDialog::filterIds()
{
	int filterColumn = ui.filterLineEdit->currentFilter();
	QString text = ui.filterLineEdit->text();

	ui.treeWidget_IdList->filterItems(filterColumn, text);
}

void IdDialog::requestRepList(const RsGxsGroupId &aboutId)
{
	mStateHelper->setLoading(IDDIALOG_REPLIST, true);

	mIdQueue->cancelActiveRequestTokens(IDDIALOG_REPLIST);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(aboutId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;
	mIdQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDDIALOG_REPLIST);
}

void IdDialog::insertRepList(uint32_t token)
{
	mStateHelper->setLoading(IDDIALOG_REPLIST, false);
#if 0

	std::vector<RsGxsIdOpinion> opinions;
	std::vector<RsGxsIdOpinion>::iterator vit;
	if (!rsIdentity->getMsgData(token, opinions))
	{
		std::cerr << "IdDialog::insertRepList() Error getting Opinions";
		std::cerr << std::endl;

		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_REPLIST);

		return;
	}

	for(vit = opinions.begin(); vit != opinions.end(); vit++)
	{
		RsGxsIdOpinion &op = (*vit);
		GxsIdTreeWidgetItem *item = new GxsIdTreeWidgetItem();

		/* insert 4 columns */

		/* friend name */
        item->setId(op.mMeta.mAuthorId, RSIDREP_COL_NAME);

		/* score */
		item->setText(RSIDREP_COL_OPINION, QString::number(op.getOpinion()));

		/* comment */
		item->setText(RSIDREP_COL_COMMENT, QString::fromUtf8(op.mComment.c_str()));

		/* local reputation */
		item->setText(RSIDREP_COL_REPUTATION, QString::number(op.getReputation()));

		ui.treeWidget_RepList->addTopLevelItem(item);
	}
#endif
	mStateHelper->setActive(IDDIALOG_REPLIST, true);
}


#endif
