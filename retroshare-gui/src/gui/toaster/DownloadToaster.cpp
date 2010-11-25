/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2007 - 2010 Xesc & Technology
 * Copyright (c) 2010 RetroShare Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************/

#include <QTimer>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QMessageBox>
#include <QFileInfo>

#include "DownloadToaster.h"

#include <retroshare/rsfiles.h>

DownloadToaster::DownloadToaster(QWidget * parent, Qt::WFlags flags)
    : QWidget(parent, flags)
{
    setupUi(this);

    /* set window flags */
    QWidget::setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);

    /* init the timer */
    displayTimer = new QTimer(this);
    connect(displayTimer, SIGNAL(timeout()), this, SLOT(displayTimerOnTimer()));

    /* connect buttons */
    connect(spbClose, SIGNAL(clicked()), this, SLOT(closeClicked()));
    connect(startButton, SIGNAL(clicked()), this, SLOT(play()));

    /* init state */
    displayState = dsInactive;
}

DownloadToaster::~DownloadToaster()
{
    delete displayTimer;
}

void DownloadToaster::displayTimerOnTimer()
{
    if (!isVisible()) return;

    QDesktopWidget *desktop = QApplication::desktop();
    QRect availableGeometry  = desktop->availableGeometry(this);

    // display popup animation
    if (displayState == dsShowing)
        if (pos().x() > availableGeometry.width() - size().width())// - 15)
            move(pos().x() - 2, pos().y());
    else
    {
        displayState = dsWaiting;
        displayTimer->start(5000);
    }
    // hide popup animation
    else if (displayState == dsHiding)
        if (pos().x() < availableGeometry.width())
            move(pos().x() + 2, pos().y());
    else
    {
        displayState = dsWaiting;
        displayTimer->stop();
        hide();
    }
    else if (displayState == dsWaiting)
    {
        displayState = dsHiding;
        displayTimer->start(2);
    }
}

void DownloadToaster::displayPopup(const std::string &hash, const QString &name)
{
    fileHash = hash;
    labelTitle->setText(name);

    QDesktopWidget *desktop = QApplication::desktop();
    QRect availableGeometry  = desktop->availableGeometry(this);
    move(desktop->width(), availableGeometry.height() - size().height());
    show();

    displayState = dsShowing;
    displayTimer->start(2);
}

void DownloadToaster::closeClicked()
{
    displayState = dsHiding;
    displayTimer->start(2);
}

void DownloadToaster::play()
{
    /* look up path */
    FileInfo fi;
    if (!rsFiles->FileDetails(fileHash, RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY, fi)) {
        return;
    }

    std::string filename = fi.path + "/" + fi.fname;

    /* open file with a suitable application */
    QFileInfo qinfo;
    qinfo.setFile(filename.c_str());
    if (qinfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()));
    }else{
        QMessageBox::information(this, "RetroShare", tr("File %1 does not exist at location.").arg(fi.path.c_str()));
    }
}
