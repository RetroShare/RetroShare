/*******************************************************************************
 * gui/settings/FileAssociationPage.cpp                                        *
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

#include "FileAssociationsPage.h"
#include "AddFileAssociationDialog.h"
//#include "rshare.h" // for Rshare::dataDirectory() method
#include "rsharesettings.h"

#include <QSettings>
#include <QApplication>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSizePolicy>

//#include <QLabel>
//#include <QLineEdit>
#include <QToolBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QDialogButtonBox>

#include <QStringList>
#include <QAction>

#include <QMessageBox>

#include <QProcess>

#include <QDebug>
#include <QMessageBox>
//#include <iostream>

//============================================================================

FileAssociationsPage::FileAssociationsPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
//:QFrame()
{
    QVBoxLayout* pageLay = new QVBoxLayout(this);

    toolBar = new QToolBar("actions", this);
    newAction = new QAction(QIcon(":/images/add_24x24.png"), tr("&New"), this);
    //newAction->setShortcut(tr("Ctrl+N"));
    newAction->setStatusTip(tr("Add new Association"));
    connect(newAction, SIGNAL(triggered()), this, SLOT(addnew()));
    toolBar->addAction(newAction);

    editAction = new QAction(QIcon(":/images/kcmsystem24.png"),
                             tr("&Edit"), this);
    editAction->setStatusTip(tr("Edit this Association"));
    connect(editAction, SIGNAL(triggered()), this, SLOT(edit()));
    toolBar->addAction(editAction);

    removeAction = new QAction(QIcon(":/images/edit_remove24.png"),
                               tr("&Remove"), this);
    removeAction->setStatusTip(tr("Remove this Association"));
    connect(removeAction, SIGNAL(triggered()), this, SLOT(remove()));
    toolBar->addAction( removeAction );

    pageLay->addWidget( toolBar );

    table = new QTableWidget(5,2,this);//default 5 rows, 2 columns
    table->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("File type") ) );

    table->setHorizontalHeaderItem(1,  new QTableWidgetItem("Command") );
    connect( table, SIGNAL( cellActivated(int, int)),
             this,  SLOT( tableCellActivated(int, int)) );
    connect( table, SIGNAL( cellClicked(int, int)),
             this,  SLOT( tableCellActivated(int, int)) );

//     connect( table, SIGNAL( cellChanged(int, int)),
//              this,  SLOT( tableCellActivated(int, int)) );
//
//     connect( table, SIGNAL( cellDoubleClicked(int, int)),
//              this,  SLOT( tableCellActivated(int, int)) );
//
//     connect( table, SIGNAL( cellEntered(int, int)),
//              this,  SLOT( tableCellActivated(int, int)) );
//
//     connect( table, SIGNAL( cellPressed(int, int)),
//              this,  SLOT( tableCellActivated(int, int)) );


//    connect( table, SIGNAL( itemClicked(QTableWidgetItem*)),
//             this,  SLOT( tableItemActivated(QTableWidgetItem*)) );

    pageLay->addWidget(table);

//    addNewAssotiationButton = new QPushButton;
//    addNewAssotiationButton->setText(tr("Add.."));
//    QHBoxLayout* anbLay = new QHBoxLayout;
//    anbLay->addStretch();
//    anbLay->addWidget(addNewAssotiationButton);
//    pageLay->addLayout(anbLay);
//    connect( addNewAssotiationButton, SIGNAL( clicked() ),
//             this,                    SLOT( testButtonClicked() ) );

//    RshareSettings settings;
               //new QSettings( qApp->applicationDirPath()+"/sett.ini",
               //               QSettings::IniFormat);
//    settings.beginGroup("FileAssociations");



}

//============================================================================

FileAssociationsPage::~FileAssociationsPage()
{
}

void
FileAssociationsPage::load()
{
//     Settings->beginGroup("FileAssotiations");
    QStringList keys = Settings->allKeys();

    table->setRowCount( keys.count() );

    int rowi = 0;
    QStringList::const_iterator ki;
    for(ki=keys.constBegin(); ki!=keys.constEnd(); ++ki)
    {
        QString val = (Settings->value(*ki, "")).toString();

        addNewItemToTable( rowi, 0, *ki );
        addNewItemToTable( rowi, 1, val );

        ++rowi;
    }

    if (keys.count()==0)
    {
        removeAction->setEnabled(false);
        editAction->setEnabled(false);
    }

    table->selectRow(0);
}

//============================================================================

void
FileAssociationsPage::remove()
{
    int currentRow = table->currentRow() ;
    QTableWidgetItem const * titem = table->item( currentRow,0);
    QString key = (titem->data(QTableWidgetItem::Type)).toString();

    Settings->remove(key);
    table->removeRow( currentRow );

    if ( table->rowCount()==0 )
    {
        removeAction->setEnabled(false);
        editAction->setEnabled(false);
    }
}

//============================================================================

void
FileAssociationsPage::addnew()
{
    AddFileAssociationDialog afad(false, this);//'add file assotiations' dialog

    QTableWidgetItem* titem;

    int ti = afad.exec();

    if (ti==QDialog::Accepted)
    {
        QString currType = afad.resultFileType() ;
        QString currCmd = afad.resultCommand() ;


        if ( !Settings->contains(currType) )//new item should be added only if
        {                              // it wasn't entered before.
            int nridx = table->rowCount();//new row index
            table->setRowCount(nridx+1);
            addNewItemToTable(nridx,0, currType) ;
            addNewItemToTable(nridx,1, currCmd);
        }
        else
        {
            for(int rowi=0; rowi<table->rowCount(); ++rowi)
            {
                titem = table->item( rowi, 0);
                if (titem->data(QTableWidgetItem::Type).toString()==currType)
                {
                    titem = table->item( rowi, 1);
                    titem->setData(QTableWidgetItem::Type, currCmd);
                    break;
                }
            }
        }

        Settings->setValue(currType, currCmd);

        removeAction->setEnabled(true);
        editAction->setEnabled(true);
    }
}

//============================================================================

void
FileAssociationsPage::edit()
{
    AddFileAssociationDialog afad(true, this);//'add file assotiations' dialog

    int currentRow = table->currentRow() ;
    QTableWidgetItem* titem;

    titem = table->item( currentRow,0);
    QString currType = (titem->data(QTableWidgetItem::Type)).toString();
    titem = table->item( currentRow,1);
    QString currCmd = (titem->data(QTableWidgetItem::Type)).toString();
    afad.setCommand(currCmd);
    afad.setFileType(currType);

    int ti = afad.exec();

    if (ti==QDialog::Accepted)
    {
        currCmd = afad.resultCommand() ;
        titem = table->item( currentRow,1);

        titem->setData(QTableWidgetItem::Type, currCmd);

        Settings->setValue(currType, currCmd);
    }
}

//============================================================================

void
FileAssociationsPage::tableCellActivated ( int row, int /*column*/ )
{
    table->selectRow(row);
}

//============================================================================

void
FileAssociationsPage::tableItemActivated ( QTableWidgetItem * item )
{
    qDebug() << "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n";
    QMessageBox::information(this,
                             tr(" Friend Help"),
                             tr("You  this"));
    table->selectRow(table->row(item));

}

//============================================================================

void
FileAssociationsPage::addNewItemToTable(int row, int column,
                                          QString itemText)
{
    QTableWidgetItem* tmpitem ;

    tmpitem = new QTableWidgetItem(itemText) ;
    tmpitem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                     // | Qt::ItemIsUserCheckable);
    table->setItem(row, column, tmpitem );
}

//============================================================================

void
FileAssociationsPage::testButtonClicked()
{
    AddFileAssociationDialog afad(true, this);// = new AddFileAssotiationDialog();

//  commented code below is a test for
//       AddFileAssotiationDialog::loadSystemDefaultCommand(QString ft) method
//    QString tmps;
//    tmps = "/home/folder/file";
//    qDebug() << "  for " << tmps <<" is " << afad.cleanFileType(tmps);
//    tmps = "/home/folder/file.avi";
//	qDebug() << "  for " << tmps <<" is " << afad.cleanFileType(tmps);
//	tmps = "file.avi";
//	qDebug() << "  for " << tmps <<" is " << afad.cleanFileType(tmps);
//	tmps = ".file";
//	qDebug() << "  for " << tmps <<" is " << afad.cleanFileType(tmps);
//	tmps = "c:\\home\\folder\\file";
//	qDebug() << "  for " << tmps <<" is " << afad.cleanFileType(tmps);
//	tmps = "/home/folder/.file";
//	qDebug() << "  for " << tmps <<" is " << afad.cleanFileType(tmps);
//	tmps = "D:\\folder\\file.asd.avi";
//	qDebug() << "  for " << tmps <<" is " << afad.cleanFileType(tmps);



    int ti = afad.exec();

    if (ti==QDialog::Accepted)
    {
        qDebug() << "  dialog was accepted";
        QProcess::execute(afad.resultCommand());//,
        		          //QStringList("D:\\prog\\eclipse_workspace\\tapp-fa\\tt.txt") );
        qDebug() << "  process finished?";
    }
    else
        if (ti == QDialog::Rejected)
            qDebug() << "  dialog rejected" ;
        else
            qDebug() << "dialog returned something else" ;


}

//============================================================================



