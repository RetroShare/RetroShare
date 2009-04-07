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

#ifndef GSETTINGSWIN_HPP_
# define GSETTINGSWIN_HPP_

# include <QtGui/QDialog>
# include "ui_settings.h"

/*!	\class GSettingsWin
	\brief The Gloster control panel.

 */
class GSettingsWin: public QDialog, private Ui::Settings
{
    Q_OBJECT

    public:
        enum PageType { General = 0, Network, Server,
                        Directories, FileAssotiations  };

        GSettingsWin(QWidget * parent = 0, Qt::WFlags flags = 0);
        ~GSettingsWin() {}

    public slots:
        //! Go to a specific part of the control panel.
        void setNewPage(int page);

    private:
        void closeEvent (QCloseEvent * event);
        void initStackedWidget();
};

#endif // !GSETTINGSWIN_HPP_
