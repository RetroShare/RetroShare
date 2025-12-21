/*******************************************************************************
 * gui/ShareManager.h                                                          *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
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

#ifndef _SHAREMANAGER_H
#define _SHAREMANAGER_H

#include <QDialog>
#include <QFileDialog>

#include <retroshare/rsfiles.h>
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
    void shareddirListCustomPopupMenu( QPoint point );
    void addShare();
    void doubleClickedCell(int,int);
    void handleCellChange(int row,int column);
	void editShareDirectory();

    void removeShareDirectory();
    void updateFlags();
    void applyAndClose() ;
    void cancel() ;
    void reload() ;

    static QString getGroupString(const std::list<RsNodeGroupId>& groups);
private:
    static ShareManager *_instance;
    bool isLoading;

    /** Define the popup menus for the Context menu */
    QMenu* contextMnu;

    /** Qt Designer generated object */
    Ui::ShareManager ui;

    std::vector<SharedDirInfo> mDirInfos ;
    RsEventsHandlerId_t mEventHandlerId;
};

#endif

