/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006- 2010 RetroShare Team
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
#include "ShareDialog.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rstypes.h>

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QLayout>
#include <QTextEdit>
#include <QComboBox>
#include <QSizePolicy>
#include <QGroupBox>

#include <gui/common/GroupSelectionBox.h>
#include <gui/common/GroupFlagsWidget.h>

/** Default constructor */
ShareDialog::ShareDialog(std::string filename, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    ui.headerFrame->setHeaderImage(QPixmap(":/images/fileshare48.png"));
    ui.headerFrame->setHeaderText(tr("Share Folder"));

    connect(ui.browseButton, SIGNAL(clicked( bool ) ), this , SLOT( browseDirectory() ) );
    connect(ui.okButton, SIGNAL(clicked( bool ) ), this , SLOT( addDirectory() ) );
    connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(close()));

    ui.okButton->setEnabled(false);

	 QVBoxLayout *vbox = new QVBoxLayout() ;

	 groupflagsbox = new GroupFlagsWidget(ui.shareflags_GB) ;
	 messageBox = new QTextEdit(ui.shareflags_GB) ;
	 messageBox->setReadOnly(true) ;
	 messageBox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Preferred)) ;

	 vbox->addWidget(groupflagsbox) ;
	 vbox->addWidget(messageBox) ;

	 QHBoxLayout *hbox = new QHBoxLayout() ;
	 groupselectionbox = new GroupSelectionBox(ui.shareflags_GB);
	 hbox->addLayout(vbox) ;
	 hbox->addWidget(groupselectionbox) ;

	 ui.shareflags_GB->setLayout(hbox) ;

	 connect(groupselectionbox,SIGNAL(itemSelectionChanged()),this,SLOT(updateInfoMessage())) ;
	 connect(groupflagsbox,SIGNAL(flagsChanged(FileStorageFlags)),this,SLOT(updateInfoMessage())) ;

    if (!filename.empty()) 
	 {
        std::list<SharedDirInfo> dirs;
        rsFiles->getSharedDirectories(dirs);

        std::list<SharedDirInfo>::const_iterator it;
        for (it = dirs.begin(); it != dirs.end(); it++) {
            if (it->filename == filename) 
				{
                /* fill dialog */
                ui.okButton->setEnabled(true);

                ui.localpath_lineEdit->setText(QString::fromUtf8(it->filename.c_str()));
                ui.localpath_lineEdit->setDisabled(true);
                ui.browseButton->setDisabled(true);
                ui.virtualpath_lineEdit->setText(QString::fromUtf8(it->virtualname.c_str()));

					 groupflagsbox->setFlags(it->shareflags) ;
                groupselectionbox->setSelectedGroups(it->parent_groups) ;

                break;
            }
        }
    }
}

void ShareDialog::updateInfoMessage()
{
	messageBox->setText(GroupFlagsWidget::groupInfoString(groupflagsbox->flags(),groupselectionbox->selectedGroups())) ;
}

void ShareDialog::browseDirectory()
{
    /* select a dir*/
    QString qdir = QFileDialog::getExistingDirectory(this, tr("Select A Folder To Share"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    /* add it to the server */
    if (qdir.isEmpty()) {
        ui.okButton->setEnabled(false);
        return;
    }
    ui.okButton->setEnabled(true);
    ui.localpath_lineEdit->setText(qdir);
}

void ShareDialog::addDirectory()
{
    SharedDirInfo sdi ;
    sdi.filename = ui.localpath_lineEdit->text().toUtf8().constData();
    sdi.virtualname = ui.virtualpath_lineEdit->text().toUtf8().constData();
    sdi.shareflags = groupflagsbox->flags() ;
	 sdi.parent_groups = groupselectionbox->selectedGroups() ;

    if (ui.localpath_lineEdit->isEnabled()) 
	 {
        /* add new share */
        rsFiles->addSharedDirectory(sdi);
    } 
	 else 
	 {
        /* edit exisiting share */
        bool found = false;

        std::list<SharedDirInfo> dirs;
        rsFiles->getSharedDirectories(dirs);

        std::list<SharedDirInfo>::iterator it;
        for (it = dirs.begin(); it != dirs.end(); it++) {
            if (it->filename == sdi.filename) {
                found = true;

                if (it->virtualname != sdi.virtualname) {
                    /* virtual name changed, remove shared directory and add it again */

                    rsFiles->removeSharedDirectory(it->filename);
                    rsFiles->addSharedDirectory(sdi);
                    break;
                }
                if (it->shareflags != sdi.shareflags || it->parent_groups != sdi.parent_groups) {
                    /* modifies the flags */
                    it->shareflags = sdi.shareflags;
                    it->parent_groups = sdi.parent_groups;
                    rsFiles->updateShareFlags(*it);
                    break;
                }

                /* nothing changed */
                break;
            }
        }

        if (found == false) {
            /* not modified, add share directory instead */
            rsFiles->addSharedDirectory(sdi);
        }
    }

    close();
}
