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

#include "hashingstatus.h"

#include "gui/notifyqt.h"

class QStatusLabel : public QLabel
{
public:
    QStatusLabel(QLayout *layout, QWidget *parent = NULL, Qt::WindowFlags f = 0) : QLabel(parent, f)
    {
        m_layout = layout;
    }

    virtual QSize minimumSizeHint() const
    {
        const QSize sizeHint = QLabel::minimumSizeHint();

        // do not resize the layout
        return QSize(qMin(sizeHint.width(), m_layout->geometry().width()), sizeHint.height());
    }

private:
    QLayout *m_layout;
};

HashingStatus::HashingStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(6);

    statusHashing = new QStatusLabel(hbox, this);
    hbox->addWidget(statusHashing);

    QSpacerItem *horizontalSpacer = new QSpacerItem(1000, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(horizontalSpacer);
    
    setLayout(hbox);

    statusHashing->hide();

    connect(NotifyQt::getInstance(), SIGNAL(hashingInfoChanged(const QString&)), SLOT(updateHashingInfo(const QString&)));
}

void HashingStatus::updateHashingInfo(const QString& s)
{
    if(s.isEmpty()) {
        statusHashing->hide() ;
    } else {
        statusHashing->setText(s);
        statusHashing->show();
    }
}
