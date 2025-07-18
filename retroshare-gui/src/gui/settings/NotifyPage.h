/*******************************************************************************
 * gui/settings/NotifyPage.h                                                   *
 *                                                                             *
 * Copyright 2009, Retroshare Team <retroshare.project@gmail.com>              *
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

#ifndef NOTIFYPAGE_H
#define NOTIFYPAGE_H

#include <retroshare-gui/configpage.h>
#include "ui_NotifyPage.h"

#include "gui/chat/ChatLobbyUserNotify.h"
#include "gui/common/FilesDefs.h"

class UserNotify;
class FeedNotify;
class ToasterNotify;

class UserNotifySetting
{
public:
    UserNotify *mUserNotify;
    QCheckBox *mEnabledCheckBox;
    QCheckBox *mCombinedCheckBox;
    QCheckBox *mBlinkCheckBox;

public:
    UserNotifySetting(UserNotify *userNotify, QCheckBox *enabledCheckBox, QCheckBox *combinedCheckBox, QCheckBox *blinkCheckBox)
        : mUserNotify(userNotify), mEnabledCheckBox(enabledCheckBox), mCombinedCheckBox(combinedCheckBox), mBlinkCheckBox(blinkCheckBox) {}
};

class FeedNotifySetting
{
public:
    FeedNotify *mFeedNotify;
    QCheckBox *mEnabledCheckBox;

public:
    FeedNotifySetting(FeedNotify *feedNotify, QCheckBox *enabledCheckBox)
        : mFeedNotify(feedNotify), mEnabledCheckBox(enabledCheckBox) {}
};

class ToasterNotifySetting
{
public:
    ToasterNotify *mToasterNotify;
    QCheckBox *mEnabledCheckBox;

public:
    ToasterNotifySetting(ToasterNotify *toasterNotify, QCheckBox *enabledCheckBox)
        : mToasterNotify(toasterNotify), mEnabledCheckBox(enabledCheckBox) {}
};

class NotifyPage : public ConfigPage
{
    Q_OBJECT

public:
    /** Default Constructor */
    NotifyPage(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
    /** Default Destructor */
    ~NotifyPage();

    /** Loads the settings for this page */
    virtual void load();

     virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/settings/notify.svg") ; }
	 virtual QString pageName() const { return tr("Notify") ; }
	 virtual QString helpText() const ;

private slots:
	void notifyToggled();
	void testToaster();
	void testFeed();

	void updateFeedNotifySettings();
	void updateToasterNotifySettings();
	void updateUserNotifySettings();
	void updateMessageFlags() ;
	void updateNotifyFlags()  ;
	void updateNewsFeedFlags();

	void updateSystrayChatLobby();
	void updateSystrayGroupChat();
	void updateToasterMargin();

	void updateToasterPosition();

private:
	uint getNewsFlags();
	uint getNotifyFlags();

    QList<FeedNotifySetting> mFeedNotifySettingList;
    QList<ToasterNotifySetting> mToasterNotifySettingList;
    QList<UserNotifySetting> mUserNotifySettingList;

    /** Qt Designer generated object */
    Ui::NotifyPage ui;
};

#endif // !NOTIFYPAGE_H

