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

#include "gui/TheWire/PulseItem.h"
#include "gui/TheWire/WireGroupItem.h"
#include "gui/TheWire/PulseAddDialog.h"

#include "util/TokenQueue.h"

#define IMAGE_WIRE              ":/icons/wire.png"

class WireDialog : public MainPage, public TokenResponse, public PulseHolder, public WireGroupHolder
{
  Q_OBJECT

public:
	WireDialog(QWidget *parent = 0);

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_WIRE) ; }
	virtual QString pageName() const { return tr("The Wire") ; }
	virtual QString helpText() const { return ""; }

	// PulseHolder interface.
	virtual void deletePulseItem(PulseItem *, uint32_t type);
	virtual void notifyPulseSelection(PulseItem *item);

	virtual void follow(RsGxsGroupId &groupId);
	virtual void rate(RsGxsId &authorId);
	virtual void reply(RsWirePulse &pulse, std::string &groupName);


	// WireGroupHolder interface.
	virtual void subscribe(RsGxsGroupId &groupId);
	virtual void unsubscribe(RsGxsGroupId &groupId);
	virtual void notifyGroupSelection(WireGroupItem *item);

private slots:

	void createGroup();
	void createPulse();
	void checkUpdate();
	void refreshGroups();
	void selectGroupSet(int index);

private:

	void addGroup(QWidget *item);

	void addPulse(RsWirePulse *pulse, RsWireGroup *group,
						std::map<rstime_t, RsWirePulse *> replies);

	void addGroup(const RsWireGroup &group);

	void deletePulses();
	void deleteGroups();
	void showGroups();
	void updateGroups(std::vector<RsWireGroup> &groups);

	// Loading Data.
	void requestGroupData();
	bool loadGroupData(const uint32_t &token);
	void acknowledgeGroup(const uint32_t &token, const uint32_t &userType);

	void requestPulseData(const std::list<RsGxsGroupId>& grpIds);
	bool loadPulseData(const uint32_t &token);

	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

	int mGroupSet;

	PulseAddDialog *mAddDialog;

	PulseItem *mPulseSelected;
	WireGroupItem *mGroupSelected;

	TokenQueue *mWireQueue;

	std::map<RsGxsGroupId, RsWireGroup> mAllGroups;
	std::vector<RsWireGroup> mOwnGroups;

	/* UI - from Designer */
	Ui::WireDialog ui;

};

#endif

