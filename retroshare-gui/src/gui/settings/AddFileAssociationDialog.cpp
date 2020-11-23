/*******************************************************************************
 * gui/settings/AddFileAssociationDialog.cpp                                   *
 *                                                                             *
 * Copyright 2009, Retroshare Team <retroshare.project@gmail.com>              *
 * Copyright 2009, Oleksiy Bilyanskyy                                          *
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

#include "AddFileAssociationDialog.h"

//#include "rshare.h" // for Rshare::dataDirectory() method
//#include "rsharesettings.h"
#include <QSettings> //for win registry access
#include <QApplication>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSizePolicy>

#include <QLabel>
#include <QLineEdit>

#include <QPushButton>
#include <QDialogButtonBox>

#include <QMessageBox>
#include <QDebug>

//============================================================================

AddFileAssociationDialog::AddFileAssociationDialog(bool onlyEdit,
                                                   QWidget* parent)
                         :QDialog(parent)
{
    fileTypeLabel = new QLabel();
    fileTypeLabel->setText(tr("File type(extension):"));


    fileTypeEdit = new QLineEdit();
    fileTypeEdit->setEnabled( !onlyEdit );
    fileTypeEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    fileTypeEdit->setFixedWidth(50);
    connect(fileTypeEdit, SIGNAL( textEdited( const QString& )),
            this        , SLOT( fileTypeEdited(const QString & )) ) ;

    loadSystemDefault = new QPushButton();
    loadSystemDefault->setText(tr("Use default command"));
    loadSystemDefault->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    loadSystemDefault->setEnabled( fileTypeEdit &&
                                   (!(fileTypeEdit->text().isEmpty())) ) ;
    connect(loadSystemDefault, SIGNAL(clicked()),
            this, SLOT(loadSystemDefaultCommand()));
    loadSystemDefault->setVisible(false); //the button sholud remain unvisivle,
                                         //until it will work properly

    commandLabel = new QLabel();
    commandLabel->setText(tr("Command"));

    commandEdit = new QLineEdit;

    selectExecutable = new QPushButton();
    selectExecutable->setText("...");
    selectExecutable->setFixedWidth(30);

    QHBoxLayout* ftlay = new QHBoxLayout();
    ftlay->addWidget( fileTypeLabel );
    ftlay->addWidget( fileTypeEdit );

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel );
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout* mainLay = new QGridLayout();
    mainLay->addWidget(fileTypeLabel,0,0,1,1, Qt::AlignLeft);
    mainLay->addWidget(fileTypeEdit,0,1,1,1, Qt::AlignLeft);
    mainLay->addWidget(loadSystemDefault, 0,2,1,2,Qt::AlignRight);
    mainLay->addWidget( commandLabel, 1,0,1,1, Qt::AlignLeft);
    mainLay->addWidget( commandEdit, 1,1,1,2);
    mainLay->addWidget( selectExecutable, 1,3,1,1);
    mainLay->addWidget( buttonBox, 2,1,1,4, Qt::AlignLeft);

    mainLay->setColumnStretch(2,1);
    mainLay->setRowStretch(2,1);

    this->setLayout( mainLay );

    //TODO: in the next line we have to count a real height of the dialog;
    //      It's not good to use a hardcoded const value;
    this->resize(600,200);//this->height());

}

//============================================================================

void
AddFileAssociationDialog::fileTypeEdited(const QString & text )
{
    loadSystemDefault->setEnabled( !( text.isEmpty() ) );
}

//============================================================================

void
AddFileAssociationDialog::setFileType(QString ft)
{
    fileTypeEdit->setText(ft);
}

//============================================================================

void
AddFileAssociationDialog::loadSystemDefaultCommand()
{
    qDebug() << "  lsdc is here";

    QString fileExt = cleanFileType(fileTypeEdit->text()) ;
    //fileExt.prepend('/');
    fileExt.append("/.");
    qDebug() << " fileExt is " << fileExt;
    QSettings reg("HKEY_CLASSES_ROOT", QSettings::NativeFormat);

    if (reg.contains(fileExt))
    {
        QString appKey = reg.value(fileExt).toString();
        qDebug() << "  got app key " << appKey;
        appKey.append("/shell/open/command/.");
        QString command = reg.value(appKey, "-").toString();
        if (command!="-")
        {
            qDebug() << "  got command :" << command ;
            commandEdit->setText( command );
            return;
        }
    }

    QMessageBox::warning(this, tr("RetroShare"),
                               tr("Sorry, can't determine system "
                                  "default command for this file\n"),
                                     QMessageBox::Ok);


}

//============================================================================

QString
AddFileAssociationDialog::cleanFileType(QString ft)
{
    QString result = ft;

    //-- first remove possible filder names from received filename. (like
    // "like "/moviedir/file.avi", we will leave only "file.avi"
    int ti;
    // dirs may be separated with "/"
    ti = result.lastIndexOf('/');
    if (ti > -1)
    {
        result.remove(0,ti+1);
    }

    // or "\"
    ti = result.lastIndexOf('\\');
    if (ti > -1)
    {
        result.remove(0,ti+1);
    }

    //-- then, if it is filename (like "file.avi"), we'll have to remove
    //-- the name (ans leave just ".avi")
    ti = result.lastIndexOf('.');
    if (ti > -1)
    {
        result.remove(0,ti);
        return result;
    }

    //-- also, if filename still is not prepended with dot, do it
    if ((!result.isEmpty()) && (result.at(0)!='.'))
        result.prepend('.');

    //-- that's all
    return result;
}

//============================================================================

QString
AddFileAssociationDialog::resultCommand()
{
    return commandEdit->text();
}

//============================================================================

QString
AddFileAssociationDialog::resultFileType()
{
    return cleanFileType( fileTypeEdit->text() ) ;
}

//============================================================================

void
AddFileAssociationDialog::setCommand(QString cmd)
{
    commandEdit->setText( cmd );
}

//============================================================================

//============================================================================


