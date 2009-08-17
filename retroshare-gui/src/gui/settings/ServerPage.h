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

#ifndef SERVERPAGE_H
# define SERVERPAGE_H

# include <QtGui/QWidget>
# include "ui_ServerPage.h"

class ServerPage: public QWidget
{
  Q_OBJECT

    public:
        ServerPage(QWidget * parent = 0, Qt::WFlags flags = 0);
        ~ServerPage() {}
        
    /** Saves the changes on this page */
    bool save(QString &errmsg);
    /** Loads the settings for this page */
    void load();

public slots:
  void updateStatus();

private slots:
  void saveAddresses();
  void toggleUPnP();
  void toggleIpDetermination(bool) ;
  void showTurtleRouterDialog(); 


    private:
    
       void closeEvent (QCloseEvent * event);
        
       Ui::ServerPage ui;
};

#endif // !SERVERPAGE_H

