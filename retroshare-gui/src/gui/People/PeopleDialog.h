/*******************************************************************************
 * retroshare-gui/src/gui/People/IdentityWidget.h                              *
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

#pragma once

#include <map>

#include <retroshare/rsidentity.h>

#include "gui/People/CircleWidget.h"
#include "gui/People/IdentityWidget.h"
#include "gui/Identity/UsageStatistics.h"
#include "gui/gxs/RsGxsUpdateBroadcastPage.h"
#include "util/TokenQueue.h"

#include "ui_PeopleDialog.h"

#define IMAGE_IDENTITY          ":/icons/png/people.png"

class PeopleDialog : public MainPage, public Ui::PeopleDialog, public TokenResponse
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

	void loadRequest(const TokenQueue * queue, const TokenRequest &req) ;

	void requestIdList() ;
	void requestCirclesList() ;

	void insertIdList(uint32_t token) ;
	void insertCircles(uint32_t token) ;

	protected:
	// Derives from MainPage
		virtual void updateDisplay(bool complete);
	//End RsGxsUpdateBroadcastPage

private slots:
	void updateCirclesDisplay(bool);

	void iw_AddButtonClickedExt();
	void iw_AddButtonClickedInt();
	void addToCircleExt();
	void addToCircleInt();
	void cw_askForGXSIdentityWidget(RsGxsId gxs_id);
	void cw_askForPGPIdentityWidget(RsPgpId pgp_id);
	void cw_imageUpdatedExt();
	void cw_imageUpdatedInt();
	void fl_flowLayoutItemDroppedExt(QList<FlowLayoutItem *> flListItem, bool &bAccept);
	void fl_flowLayoutItemDroppedInt(QList<FlowLayoutItem *> flListItem, bool &bAccept);
	void pf_centerIndexChanged(int index);
	void pf_mouseMoveOverSlideEvent(QMouseEvent* event, int slideIndex);
	void pf_dragEnterEventOccurs(QDragEnterEvent *event);
	void pf_dragMoveEventOccurs(QDragMoveEvent *event);
	void pf_dropEventOccursExt(QDropEvent *event);
	void pf_dropEventOccursInt(QDropEvent *event);
	
	void chatIdentity();
	void sendMessage();
	void personDetails();
	void sendInvite();
	void addtoContacts();
	void filterChanged(const QString &text);
	void sortByName();
	void sortByPopularity();
	void clearAllSelections();
	void onIdentitySelected();
	void onCircleSelected();        // circle widget clicked → circle-selected state
	void onCircleTreeItemClicked(QTreeWidgetItem *item, int column);
	void onCircleTreeContextMenuRequested(const QPoint &pos);
	void onBackClicked();           // back button → return to no-selection state
	void clearPerson();
	void toggleStackedPage();
    void toggledetailsStackedPage();
	void onDetailsPageChanged(int index);
	void modifyReputation();
	void requestJoinLeaveCircle();  // join/leave button in circle details page

private:
	void reloadAll();
	void populatePictureFlowExt();
	void populatePictureFlowInt();
	void applySortAndFilter(bool);
	void loadIdentityLabels(const RsGxsIdGroup& data);
	void populateCirclesTree(const std::list<RsGroupMetaData>& circles);
	void setVoteControlsVisible(bool visible);

	// Selection state machine
	enum class SelectionMode { None, Identity, Circle };
	void showNoneSelected();
	void showIdentitySelected(const RsGxsId& id);
	void showCircleSelected(const RsGxsGroupId& circleId);
	void loadCircleLabels(const CircleWidget* cw);
	void loadCircleLabels(const RsGxsGroupId& circleId);

	TokenQueue *mIdentityQueue;
	TokenQueue *mCirclesQueue;
	//RsGxsUpdateBroadcastBase *mCirclesBroadcastBase ;
	RsGxsId mCurrentSelectedId; // Store the ID of the person currently clicked
    QWidget *UsagePage;

	// Selection state
	SelectionMode _selectionMode = SelectionMode::None;
	RsGxsId      _selectedGxsId;
	RsGxsGroupId _selectedCircleId;
	bool         _inSelectionUpdate = false;
	std::set<RsGxsGroupId> _requestedCircles;

	FlowLayout *_flowLayoutExt;
	std::map<RsGxsId,IdentityWidget *> _gxs_identity_widgets ;
	std::map<RsGxsGroupId,CircleWidget *> _ext_circles_widgets ;
	QList<CircleWidget*> _extListCir;

	FlowLayout *_flowLayoutInt;
	std::map<RsPgpId,IdentityWidget *> _pgp_identity_widgets ;
	std::map<RsGxsGroupId,CircleWidget *> _int_circles_widgets ;
	QList<CircleWidget*> _intListCir;
};

