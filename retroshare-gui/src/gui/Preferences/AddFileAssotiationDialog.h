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

#ifndef __AddFileAssotiationDialog__
#define __AddFileAssotiationDialog__


#include <QString>
#include <QDialog>

class QPushButton;
class QDialogButtonBox;

class QLabel;
class QLineEdit;

//=============================================================================
//!  A dialog for specifying file type and associated command
class AddFileAssotiationDialog: public QDialog
{
    Q_OBJECT
public:
    //! constructor

    //! if (onlyEdit == true), user woll not be able to change file type,
    //! only command (used for editing existing commands)
    AddFileAssotiationDialog( bool onlyEdit = false, QWidget *parent = 0 ) ;
    virtual ~AddFileAssotiationDialog(){};
    void setFileType(QString ft);
    void setCommand(QString cmd);

    //! Gets file type (file extension) from given filename (or other string)

    //! "file type" has to be like '.png'(some symbols, prepended by a dot)
    static QString cleanFileType(QString ft);

    QString resultCommand();
    QString resultFileType();

protected:
    //QTabWidget *tabWidget;
    QLabel* fileTypeLabel;
    QLineEdit* fileTypeEdit;
    QPushButton* loadSystemDefault;
    QPushButton* selectExecutable;
    QLabel* commandLabel;
    QLineEdit* commandEdit;
    QDialogButtonBox *buttonBox;    

protected slots:
    void fileTypeEdited(const QString & text );

    //! On win32, loads default command from system registry.

    //! Unfinished. Is not used in current version.
    void loadSystemDefaultCommand();


};

#endif

