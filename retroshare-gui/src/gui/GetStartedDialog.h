/*******************************************************************************
 * gui/GetStartedDialog.h                                                      *
 *                                                                             *
 * Copyright (C) 2011 Robert Fernie   <retroshare.project@gmail.com>           *
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

#ifndef _GETTING_STARTED_DIALOG_H
#define _GETTING_STARTED_DIALOG_H

#include <retroshare-gui/mainpage.h>

#include "ui_GetStartedDialog.h"

#include <QTimer>

#define IMG_HELP                ":/icons/png/help.png"

class GetStartedDialog : public MainPage
{
	Q_OBJECT

public:
	/** Default Constructor */
	GetStartedDialog(QWidget *parent = 0);
	/** Default Destructor */
	~GetStartedDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMG_HELP) ; } //MainPage
	virtual QString pageName() const { return tr("Getting Started") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	// Single Point for (English) Text of the Invitation.
	// This is used by other classes.
	static QString GetInviteText();
	static QString GetCutBelowText();

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
