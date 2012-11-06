/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2010 Christopher Evi-Parker
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

#ifndef _EDITCHANDETAILS_H
#define _EDITCHANDETAILS_H

#include <QDialog>

#include "ui_EditChanDetails.h"

class EditChanDetails : public QDialog
{
  Q_OBJECT

public:
    /** Default constructor */
    EditChanDetails(QWidget *parent = 0, std::string cId = 0);

signals:
    void configChanged();

private slots:
	void applyDialog();
    void addChannelLogo();

private:
    void loadChannel();

    std::string mChannelId;
    QPixmap picture;

    /** Qt Designer generated object */
    Ui::EditChanDetails ui;
};

#endif

