/*******************************************************************************
 * gui/TheWire/WireDialog.h                                                    *
 *                                                                             *
 * Copyright (c) 2012-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#ifndef MRK_WIRE_DIALOG_H
#define MRK_WIRE_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_WireDialog.h"

#include <retroshare/rswire.h>

#include <map>

#include "gui/TheWire/WireGroupItem.h"
#include "gui/TheWire/PulseAddDialog.h"

#include "gui/TheWire/PulseViewItem.h"
#include "gui/TheWire/PulseTopLevel.h"
#include "gui/TheWire/PulseReply.h"


#include "util/TokenQueue.h"

#define IMAGE_WIRE              ":/icons/wire.png"

class WireDialog : public MainPage, public TokenResponse, public WireGroupHolder, public PulseViewHolder
{
  Q_OBJECT

public:
	WireDialog(QWidget *parent = 0);

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_WIRE) ; }
	virtual QString pageName() const { return tr("The Wire") ; }
	virtual QString helpText() const { return ""; }

	// WireGroupHolder interface.
	virtual void subscribe(RsGxsGroupId &groupId) override;
	virtual void unsubscribe(RsGxsGroupId &groupId) override;
	virtual void notifyGroupSelection(WireGroupItem *item) override;

	// PulseViewItem interface
	virtual void PVHreply(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) override;
	virtual void PVHrepublish(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) override;
	virtual void PVHlike(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) override;

	virtual void PVHviewGroup(const RsGxsGroupId &groupId) override;
	virtual void PVHviewPulse(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) override;
	virtual void PVHviewReply(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) override;

	virtual void PVHfollow(const RsGxsGroupId &groupId) override;
	virtual void PVHrate(const RsGxsId &authorId) override;

	// New TwitterView
	void postTestTwitterView();
	void clearTwitterView();
	void addTwitterView(PulseViewItem *item);

	void showPulseFocus(const RsGxsGroupId groupId, const RsGxsMessageId msgId);
	void postPulseFocus(RsWirePulseSPtr pulse);

	void showGroupFocus(const RsGxsGroupId groupId);
	void postGroupFocus(RsWireGroupSPtr group, std::list<RsWirePulseSPtr> pulses);

	void showGroupsPulses(const std::list<RsGxsGroupId> groupIds);
	void postGroupsPulses(std::list<RsWirePulseSPtr> pulses);

private slots:

	void createGroup();
	void createPulse();
	void checkUpdate();
	void refreshGroups();
	void selectGroupSet(int index);
	void selectFilterTime(int index);

private:

	bool setupPulseAddDialog();

	void addGroup(QWidget *item);
	void addGroup(const RsWireGroup &group);

	void deleteGroups();
	void showGroups();
	void showSelectedGroups();
	void updateGroups(std::vector<RsWireGroup> &groups);

	// utils.
	rstime_t getFilterTimestamp();

	// Loading Data.
	void requestGroupData();
	bool loadGroupData(const uint32_t &token);
	void acknowledgeGroup(const uint32_t &token, const uint32_t &userType);

	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

	int mGroupSet;

	PulseAddDialog *mAddDialog;
	WireGroupItem *mGroupSelected;
	TokenQueue *mWireQueue;

	std::map<RsGxsGroupId, RsWireGroup> mAllGroups;
	std::vector<RsWireGroup> mOwnGroups;

	/* UI - from Designer */
	Ui::WireDialog ui;

};

#endif

