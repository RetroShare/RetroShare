/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2016, defnax
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
	
	  virtual QIcon iconPixmap() const { return QPixmap(":/icons/svg/profile.svg") ; } //MainPage
    virtual QString pageName() const { return tr("Home") ; } //MainPage
    virtual QString helpText() const { return ""; } //MainPage

private slots:
	void updateOwnCert();
	void runEmailClient();
	void copyCert();
	void saveCert();
  void addFriend();
	void runStartWizard() ;
	void recommendFriends();

private:
	Ui::HomePage *ui;
	

};

#endif // HomePage_H
