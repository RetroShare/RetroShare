/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2011 RetroShare Team
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

#ifndef _FRIENDSDIALOG_H
#define _FRIENDSDIALOG_H

#include "retroshare-gui/RsAutoUpdatePage.h"

#include "ui_FriendsDialog.h"

#define IMAGE_NETWORK         	":/icons/png/network.png"

class QAction;
class NetworkDialog;
class NetworkView;
class IdDialog;
class CirclesDialog;

class FriendsDialog : public RsAutoUpdatePage
{
    Q_OBJECT

public:
		 enum Page {
						 /* Fixed numbers for load and save the last page */
			 				IdTab              = 0,  /** Identities page. */
#ifdef RS_USE_CIRCLES
							CirclesTab         = 1,  /** Circles page. */
#endif
							NetworkTab         = 2,  /** Network page. */
							NetworkViewTab     = 3,  /** Network new graph. */
							BroadcastTab       = 4   /** Old group chat page. */
							
		 };

    /** Default Constructor */
    FriendsDialog(QWidget *parent = 0);
    /** Default Destructor */
    ~FriendsDialog ();

    virtual QIcon iconPixmap() const { return QIcon(IMAGE_NETWORK) ; } //MainPage
    virtual QString pageName() const { return tr("Friends") ; } //MainPage
    virtual QString helpText() const { return ""; } //MainPage

    virtual UserNotify *getUserNotify(QObject *parent);

    virtual void updateDisplay() ;	// overloaded from RsAutoUpdatePage

    static bool isGroupChatActive();
    static void groupChatActivate();

	 void activatePage(FriendsDialog::Page page) ;

	 NetworkDialog *networkDialog ;
	 NetworkView *networkView ;
	 
#ifdef RS_USE_CIRCLES
	 CirclesDialog *circlesDialog;
#endif
	 IdDialog *idDialog;
	 
protected:
    void showEvent (QShowEvent *event);

private slots:
    void chatMessageReceived(const ChatMessage& msg);
    void chatStatusReceived(const ChatId& chat_id, const QString& status_string);

    void addFriend();

    void statusmessage();

    void getAvatar();

    void loadmypersonalstatus();

    void clearChatNotify();

    //void newsFeedChanged(int count);

signals:
    void notifyGroupChat(const QString&,const QString&) ;

private:
    void processSettings(bool bLoad);

    /** Qt Designer generated object */
    Ui::FriendsDialog ui;
};

#endif
