/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#ifndef _SHAREMANAGER_H
#define _SHAREMANAGER_H

#include <QDialog>
#include <QFileDialog>

#include "ui_ShareManager.h"

class ShareManager : public QDialog
{
    Q_OBJECT

public:
    static void showYourself() ;
    static void postModDirectories(bool update_local);

private:
    /** Default constructor */
    ShareManager();
    /** Default destructor */
    ~ShareManager();

    /** Loads the settings for this page */
    void load();

protected:
    virtual void showEvent(QShowEvent * event);

    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

private slots:
    /** Create the context popup menu and it's submenus */
    void shareddirListCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void shareddirListCostumPopupMenu( QPoint point );

    void showShareDialog();
    void editShareDirectory();
    void removeShareDirectory();
    void updateFlags();

private:
    static ShareManager *_instance;
    bool isLoading;

    /** Define the popup menus for the Context menu */
    QMenu* contextMnu;

    /** Qt Designer generated object */
    Ui::ShareManager ui;
};

#endif

