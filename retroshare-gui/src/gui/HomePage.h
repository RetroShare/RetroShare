/*******************************************************************************
 * gui/HomePage.h                                                              *
 *                                                                             *
 * Copyright (C) 2016 Defnax          <retroshare.project@gmail.com>           *
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

#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <retroshare-gui/mainpage.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>

#include <QWidget>


class QAction;

namespace Ui {
class HomePage;
}

class HomePage : public MainPage
{
	Q_OBJECT

public:
	explicit HomePage(QWidget *parent);
	~HomePage();

	virtual QIcon iconPixmap() const { return QIcon(":/icons/png/home.png") ; } //MainPage
	virtual QString pageName() const { return tr("Home") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

    // Returns the certificate along with its description using current options.

    void getOwnCert(QString& invite,QString& description) const;

private slots:
#ifdef DEAD_CODE
    void certContextMenu(QPoint);
#endif
	void updateOwnCert();
	void runEmailClient();
	void copyCert();
	void copyId();
	void saveCert();
	void addFriend();
	void webMail();
	void openWebHelp() ;
    void recommendFriends();
    void switchCertificateFormat();

private:
	Ui::HomePage *ui;

    QAction *mIncludeDNSact;
    QAction *mIncludeIPHistoryact;
    QAction *mIncludeExtIPact;
    QAction *mIncludeLocIPact;
    QAction *mUseOldFormatact;

    RsEventsHandlerId_t mEventHandlerId;

    void handleEvent(std::shared_ptr<const RsEvent> event);
};

#endif // HomePage_H
