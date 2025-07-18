/*******************************************************************************
 * retroshare-gui/src/gui/profile/ProfileManager.h                             *
 *                                                                             *
 * Copyright (C) 2009 by Retroshare Team     <retroshare.project@gmail.com>    *
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


#ifndef _PROFILEWIDGET_H
#define _PROFILEWIDGET_H

#include <QWidget>

#include "ui_ProfileWidget.h"

class ProfileWidget : public QWidget
{
    Q_OBJECT

public:
    /** Default constructor */
    ProfileWidget(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());

private slots:
    void showEvent ( QShowEvent * event );
    void statusmessagedlg();
    void copyCert();
    void profilemanager();

private:
    /** Qt Designer generated object */
    Ui::ProfileWidget ui;
};

#endif

