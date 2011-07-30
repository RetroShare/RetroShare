/****************************************************************
*  RetroShare is distributed under the following license:
*
*  Copyright (C) 2011, drbob
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

#ifndef _GETTING_STARTED_DIALOG_H
#define _GETTING_STARTED_DIALOG_H

//#include <retroshare/rstypes.h>
#include "ui_GetStartedDialog.h"
#include "mainpage.h"

#include <QTimer>

class GetStartedDialog : public MainPage
{
    Q_OBJECT

        public:
/** Default Constructor */
    GetStartedDialog(QWidget *parent = 0);
/** Default Destructor */
    ~GetStartedDialog();

/*** signals: ***/

protected:
	// Overloaded to get first show!
virtual void showEvent ( QShowEvent * event );
virtual void changeEvent(QEvent *e);

private slots:

  void tickInviteChanged();
  void tickAddChanged();
  void tickConnectChanged();
  void tickFirewallChanged();

  void addFriends();
  void inviteFriends();

  void emailFeedback();
  void emailSupport();
  void emailSubscribe();
  void emailUnsubscribe();

  void OpenFAQ();
  void OpenForums();
  void OpenWebsite();

private:

  void updateFromUserLevel();


  bool mFirstShow;

	private:

 	QTimer *mTimer;
 	QTimer *mInviteTimer;

/** Qt Designer generated object */
    Ui::GetStartedDialog ui;

};

#endif

