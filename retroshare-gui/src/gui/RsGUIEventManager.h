/*******************************************************************************
 * gui/RsGUIEventManager.h                                                              *
 *                                                                             *
 * Copyright (c) 2010 Retroshare Team  <retroshare.project@gmail.com>          *
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

#ifndef RSIFACE_NOTIFY_TXT_H
#define RSIFACE_NOTIFY_TXT_H

#include <retroshare/rsiface.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rsmail.h>
#include <retroshare/rschats.h>
#include <QObject>
#include <QMutex>
#include <QPoint>
//#include <QMutex>

#include "settings/rsharesettings.h"

#include <string>

class QTimer;
class NetworkDialog;
class FriendsDialog;
class TransfersDialog;
class ChatDialog;
class MessagesDialog;
class ChannelsDialog;
class MessengerWindow;
class ToasterItem;
class ToasterNotify;
class SignatureEventData ;

struct TurtleFileInfo;
struct TurtleGxsInfo;

class RsGUIEventManager: public QObject
{
	Q_OBJECT
	public:
        static void Create();
        static RsGUIEventManager *getInstance ();
		static bool isAllDisable();
		void enable() ;

        virtual ~RsGUIEventManager() = default;

        virtual bool GUI_askForPassword(const std::string& title, const std::string& key_details, bool prev_is_bad);
        virtual bool GUI_askForPluginConfirmation(const std::string& plugin_filename, const RsFileHash& plugin_file_hash,bool first_time);

		/* Notify from GUI */
		void notifyChatFontChanged();
		void notifyChatStyleChanged(int /*ChatStyle::enumStyleType*/ styleType);

        void testToasters(RsNotifyPopupFlags notifyFlags, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin);
		void testToaster(ToasterNotify *toasterNotify, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin);
		void testToaster(QString tag, ToasterNotify *toasterNotify, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin);
#ifdef TO_REMOVE
		void addToaster(uint notifyFlags, const std::string& id, const std::string& title, const std::string& msg);
#endif
		void notifySettingsChanged();

	signals:
		// It's beneficial to send info to the GUI using signals, because signals are thread-safe
		// as they get queued by Qt.
		//
		void configChanged() const ;
		void logInfoChanged(const QString&) const ;
		void chatCleared(const ChatId&) const ;
		void historyChanged(uint msgId, int type);

		/* Notify from GUI */
		void chatFontChanged();
		void chatStyleChanged(int /*ChatStyle::enumStyleType*/ styleType);
		void settingsChanged();
		void disableAllChanged(bool disableAll) const;

	public slots:
		void SetDisableAll(bool bValue);

	private slots:
		void runningTick();

	private:
        RsGUIEventManager();

        static void displayDiskSpaceWarning(int loc,int size_limit_mb);
        static void displayErrorMessage(RsNotifySysFlags type,const QString& title,const QString& error_msg);

        static RsGUIEventManager *_instance;
		static bool _disableAllToaster;

		/* system notifications */

		void startWaitingToasters();

		QList<ToasterItem*> waitingToasterList;

		QTimer *runningToasterTimer;
		QList<ToasterItem*> runningToasterList;

		bool _enabled ;
		QMutex _mutex ;

		std::map<std::string,SignatureEventData*> _deferred_signature_queue ;

		/* so we can update windows */
		NetworkDialog *cDialog;

        void async_handleIncomingEvent(std::shared_ptr<const RsEvent> e);
        void sync_handleIncomingEvent(std::shared_ptr<const RsEvent> e);

        RsEventsHandlerId_t mEventHandlerId;
};

#endif
