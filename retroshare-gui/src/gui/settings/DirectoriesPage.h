/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
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

#ifndef DIRECTORIESPAGE_H
# define DIRECTORIESPAGE_H

#include <QFileDialog>
#include <QWidget>
#include <QtGui>


# include "ui_DirectoriesPage.h"

class DirectoriesPage: public QWidget 
{
  Q_OBJECT

    public:
        DirectoriesPage(QWidget * parent = 0, Qt::WFlags flags = 0);
      //  ~DirectoriesPage() {}
        
    /** Saves the changes on this page */
    bool save(QString &errmsg);
    /** Loads the settings for this page */
    void load();



    private slots:
    
#ifdef TO_REMOVE
	void addShareDirectory();
	void removeShareDirectory();
#endif

    void editDirectories() ;
    void setIncomingDirectory();
    void setPartialsDirectory();
    void shareDownloadDirectory(int state);

    private:
    
       void closeEvent (QCloseEvent * event);
        
       Ui::DirectoriesPage ui;
};

#endif // !GENERALPAGE_H

