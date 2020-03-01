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
#include "gui/TheWire/PulseAddDialog.h"

#include "util/TokenQueue.h"

#define IMAGE_WIRE              ":/images/kgames.png"

class WireDialog : public MainPage, public TokenResponse, public PulseHolder
{
  Q_OBJECT

public:
	WireDialog(QWidget *parent = 0);

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_WIRE) ; }
	virtual QString pageName() const { return tr("The Wire") ; }
	virtual QString helpText() const { return ""; }

	// PulseHolder interface.
	virtual void deletePulseItem(PulseItem *, uint32_t type);
	virtual void notifySelection(PulseItem *item, int ptype);

	virtual void follow(RsGxsGroupId &groupId);
	virtual void rate(RsGxsId &authorId);
	virtual void reply(RsWirePulse &pulse, std::string &groupName);

	void notifyPulseSelection(PulseItem *item);

private slots:

	void createGroup();
	void createPulse();
	void checkUpdate();
	void refreshGroups();

private:

	void addItem(QWidget *item);
	void addGroup(QWidget *item);

	void addPulse(RsWirePulse &pulse, RsWireGroup &group);
	void addGroup(RsWireGroup &group);

	void deletePulses();
	void deleteGroups();
	void updateGroups(std::vector<RsWireGroup> &groups);

	// Loading Data.
	void requestGroupData();
	bool loadGroupData(const uint32_t &token);

	void requestPulseData(const std::list<RsGxsGroupId>& grpIds);
	bool loadPulseData(const uint32_t &token);

	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

	PulseAddDialog *mAddDialog;

	PulseItem *mPulseSelected;

	TokenQueue *mWireQueue;

	std::map<RsGxsGroupId, RsWireGroup> mAllGroups;
	std::vector<RsWireGroup> mOwnGroups;

	/* UI - from Designer */
	Ui::WireDialog ui;

};

#endif

