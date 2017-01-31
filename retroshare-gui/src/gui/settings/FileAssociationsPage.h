/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 The RetroShare Team, Oleksiy Bilyanskyy
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

#ifndef __FileAssociationsPage__
#define __FileAssociationsPage__

#include <retroshare-gui/configpage.h>

class QToolBar;
class QAction;
class QTableWidget;
class QTableWidgetItem;
class QPushButton;
class QDialogButtonBox;


class QLabel;
class QLineEdit;

//class QSettings;

//=============================================================================
//! Dialog for setting file assotiations for RS

//! With this config page user can specify, what programs should be executed
//! to open some types of files. Here 'type' means 'file extension'(and
//! 'file extension' means 'some symbols after last dot in filename').
class FileAssociationsPage : public ConfigPage
{
    Q_OBJECT

public:
    FileAssociationsPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    virtual ~FileAssociationsPage();

    virtual void load();
	 virtual QPixmap iconPixmap() const { return QPixmap(":/images/filetype-association.png") ; }
	 virtual QString pageName() const { return tr("Associations") ; }

protected:
    QToolBar* toolBar;

    QAction* newAction;
    QAction* editAction;
    QAction* removeAction;

    QTableWidget* table;
    QPushButton* addNewAssotiationButton;
    QString settingsFileName;

    void addNewItemToTable(int row, int column, QString itemText);

protected slots:
    void remove();
    void addnew();
    void edit();
    void tableCellActivated ( int row, int column );
    void tableItemActivated ( QTableWidgetItem * item ) ;
    void testButtonClicked();//! slot for debuggin purposes, nnot really used
};



#endif
