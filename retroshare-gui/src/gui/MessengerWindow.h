/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007, RetroShare Team
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

#ifndef _MESSENGERWINDOW_H
#define _MESSENGERWINDOW_H

#include "ui_MessengerWindow.h"

#include <gui/common/rwindow.h>

class MessengerWindow : public RWindow
{
    Q_OBJECT

public:
    static void showYourself ();
    static MessengerWindow* getInstance();
    static void releaseInstance();

public slots:
    void loadmystatusmessage();

protected:
    /** Default Constructor */
    MessengerWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    /** Default Destructor */
    ~MessengerWindow();

    void closeEvent (QCloseEvent * event);

private slots:
    /** Add a new friend */
    void addFriend();

    /** Open Shared Manager **/
    void openShareManager();

    void updateOwnStatus(const QString &peer_id, int status);

    void savestatusmessage();

private:
    static MessengerWindow *_instance;

    void processSettings(bool bLoad);

    QString m_nickName;

    /** Qt Designer generated object */
    Ui::MessengerWindow ui;
};

#endif
