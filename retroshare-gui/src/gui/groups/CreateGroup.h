/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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


#ifndef _CREATEGROUP_H
#define _CREATEGROUP_H

#include "ui_CreateGroup.h"


class CreateGroup : public QDialog
{
    Q_OBJECT

public:
    /** Default constructor */
    CreateGroup(const std::string groupId, QWidget *parent = 0);
    /** Default destructor */
    ~CreateGroup();

public slots:

private slots:

private:
    std::string m_groupId;

    QStringList usedGroupNames;

    /** Qt Designer generated object */
    Ui::CreateGroup ui;

private slots:
    void on_buttonBox_accepted();
    void on_groupname_textChanged(QString );
};

#endif

