/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef GXSUSERNOTIFY_H
#define GXSUSERNOTIFY_H

#include <QObject>
#include "gui/common/UserNotify.h"
#include "util/TokenQueue.h"

class RsGxsIfaceHelper;
class RsGxsUpdateBroadcastBase;

class GxsUserNotify : public UserNotify, public TokenResponse
{
	Q_OBJECT

public:
	GxsUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent = 0);
	virtual ~GxsUserNotify();

	/* TokenResponse */
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

protected:
	virtual void startUpdate();

private:
	virtual unsigned int getNewCount() { return mCountChildMsgs ? (mNewThreadMessageCount + mNewChildMessageCount) : mNewThreadMessageCount; }

protected:
	bool mCountChildMsgs; // Count new child messages?

private:
	RsGxsIfaceHelper *mInterface;
	RsTokenService *mTokenService;
	TokenQueue *mTokenQueue;
	RsGxsUpdateBroadcastBase *mBase;
	unsigned int mNewThreadMessageCount;
	unsigned int mNewChildMessageCount;
};

#endif // GXSUSERNOTIFY_H
