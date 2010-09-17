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

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>

/** Default constructor */
ShareDialog::ShareDialog(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    connect(ui.browseButton, SIGNAL(clicked( bool ) ), this , SLOT( browseDirectory() ) );
    connect(ui.okButton, SIGNAL(clicked( bool ) ), this , SLOT( addDirectory() ) );
    connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(closedialog()));

    ui.okButton->setEnabled(false);

    load();
}

void ShareDialog::load()
{
    ui.localpath_lineEdit->clear();
    ui.browsableCheckBox->setChecked(false);
    ui.networkwideCheckBox->setChecked(false);
}

void ShareDialog::browseDirectory()
{
    /* select a dir*/
    QString qdir = QFileDialog::getExistingDirectory(this, tr("Select A Folder To Share"), "", QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

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

    sdi.shareflags = 0;

    if (ui.browsableCheckBox->isChecked()) {
        sdi.shareflags |= RS_FILE_HINTS_BROWSABLE ;
    }
    if (ui.networkwideCheckBox->isChecked()) {
        sdi.shareflags |= RS_FILE_HINTS_NETWORK_WIDE;
    }

    rsFiles->addSharedDirectory(sdi);

    load();
    close();
}

void ShareDialog::showEvent(QShowEvent *event)
{
    if (!event->spontaneous())
    {
        load();
    }
}

void ShareDialog::closedialog()
{
    ui.localpath_lineEdit->clear();
    ui.browsableCheckBox->setChecked(false);
    ui.networkwideCheckBox->setChecked(false);

    close();
}
