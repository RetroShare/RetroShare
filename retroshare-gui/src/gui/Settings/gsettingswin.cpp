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


#include "GeneralPage.h"
#include "DirectoriesPage.h"
#include "ServerPage.h"
#include "NetworkPage.h"
#include "gsettingswin.h"

GSettingsWin::GSettingsWin(QWidget * parent, Qt::WFlags flags)
                            : QDialog(parent, flags)
{
    setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
    setModal(false);
    //setWindowTitle(windowTitle() + QLatin1String(___GLOSTER_WINTITLE));

    initStackedWidget();

    connect(listWidget, SIGNAL(currentRowChanged(int)),
            this, SLOT(setNewPage(int)));

}

void
GSettingsWin::closeEvent (QCloseEvent * event)
{

    QWidget::closeEvent(event);
}

void
GSettingsWin::initStackedWidget()
{
    stackedWidget->setCurrentIndex(-1);
    stackedWidget->removeWidget(stackedWidget->widget(0));

    stackedWidget->addWidget(new GeneralPage(false));    
    stackedWidget->addWidget(new NetworkPage());
    stackedWidget->addWidget(new ServerPage());
    stackedWidget->addWidget(new DirectoriesPage());
   

    setNewPage(General);
    
    
}

void
GSettingsWin::setNewPage(int page)
{
    QString text;

    if (page == General)
        text = tr("General");
    else if (page == Network)
        text = tr("Network");
    else if (page == Directories)
        text = tr("Directories");
    else if (page == Server)

        text = tr("Server");

    pageName->setText(tr("%1").arg(text));
    stackedWidget->setCurrentIndex(page);
    listWidget->setCurrentRow(page);
}



