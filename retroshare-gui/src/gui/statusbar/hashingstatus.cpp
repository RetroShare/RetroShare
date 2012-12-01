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

#include "gui/notifyqt.h"

class StatusLabel : public QLabel
{
public:
    StatusLabel(QLayout *layout, int diffWidth, QWidget *parent = NULL, Qt::WindowFlags f = 0) : QLabel(parent, f)
    {
        m_layout = layout;
        m_diffWidth = diffWidth;
    }

    virtual QSize minimumSizeHint() const
    {
        const QSize sizeHint = QLabel::minimumSizeHint();

        // do not resize the layout
        return QSize(qMin(sizeHint.width(), m_layout->geometry().width() - m_diffWidth), sizeHint.height());
    }

private:
    QLayout *m_layout;
    int m_diffWidth;
};

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
    statusHashing = new StatusLabel(hbox, movie->frameRect().width() + hbox->spacing(), this);
    hbox->addWidget(statusHashing);

    QSpacerItem *horizontalSpacer = new QSpacerItem(3000, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(horizontalSpacer);
    
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
    if(s.isEmpty()) {
        statusHashing->hide();
        hashloader->hide();

        movie->stop();
    } else {
        statusHashing->setText(s);
        statusHashing->show();
        hashloader->show();

        movie->start();
    }
}
