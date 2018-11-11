/*******************************************************************************
 * gui/chat/ChatUserNotify.h                                                   *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef CHATUSERNOTIFY_H
#define CHATUSERNOTIFY_H

#include <retroshare/rsmsgs.h>
#include "gui/common/UserNotify.h"

// this class uses lots of global state
// so only one instance is allowed
// (it does not make sense to have multiple instances of this class anyway)
class ChatUserNotify : public UserNotify
{
	Q_OBJECT

public:
    static void getPeersWithWaitingChat(std::vector<RsPeerId>& peers);
    static void clearWaitingChat(ChatId id);

	ChatUserNotify(QObject *parent = 0);
    ~ChatUserNotify();

	virtual bool hasSetting(QString *name, QString *group);

private slots:
    void chatMessageReceived(ChatMessage msg);

private:
	virtual QIcon getIcon();
	virtual QIcon getMainIcon(bool hasNew);
	virtual unsigned int getNewCount();
	virtual void iconClicked();
};

#endif // CHATUSERNOTIFY_H
