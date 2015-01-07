/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 RetroShare Team
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

#include <QLayout>
#include <QLabel>
#include <QMovie>

#include "hashingstatus.h"
#include "gui/common/ElidedLabel.h"
#include "gui/notifyqt.h"

HashingStatus::HashingStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(6);
        
    movie = new QMovie(":/images/loader/16-loader.gif");
    movie->setSpeed(80); // 2x speed
    hashloader = new QLabel(this);
    hashloader->setMovie(movie);
    hbox->addWidget(hashloader);

    movie->jumpToNextFrame(); // to calculate the real width
    statusHashing = new ElidedLabel(this);
    hbox->addWidget(statusHashing);

    _compactMode = false;

    setLayout(hbox);

    hashloader->hide();
    statusHashing->hide();

    connect(NotifyQt::getInstance(), SIGNAL(hashingInfoChanged(const QString&)), SLOT(updateHashingInfo(const QString&)));
}

HashingStatus::~HashingStatus()
{
    delete(movie);
}

void HashingStatus::updateHashingInfo(const QString& s)
{
    if (s.isEmpty()) {
        statusHashing->hide() ;
        hashloader->hide() ;

        movie->stop() ;
    } else {
        hashloader->setToolTip(s) ;

        if (_compactMode) {
            statusHashing->hide() ;
        } else {
            statusHashing->setText(s) ;
            statusHashing->show() ;
        }
        hashloader->show() ;

        movie->start() ;
    }
}
