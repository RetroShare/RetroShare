/*
    QSoloCards is a collection of Solitaire card games written using Qt
    Copyright (C) 2009  Steve Moore

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "About.h"
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPixmap>
#include <QtGui/QLabel>
#include <QtGui/QDialogButtonBox>
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
About::About(QWidget * pParent)
        :QDialog(pParent)
{
    this->setWindowTitle(tr("About QSoloCards").trimmed());

    // Layout the dialog  a Vbox that will be the top level. And contain the HBox that has the icon and text
    // and then the dialogButtonBox for the close button.
    QVBoxLayout * pLayout=new QVBoxLayout;

    QHBoxLayout * pMainLayout=new QHBoxLayout;

    QLabel * pIconLabel=new QLabel(this);

    pIconLabel->setPixmap(QPixmap(":/images/sol128x128.png"));

    pMainLayout->addWidget(pIconLabel,0,Qt::AlignTop|Qt::AlignLeft);

    QLabel * pWordsLabel=new QLabel(this);

    pWordsLabel->setWordWrap(true);

    // set the text for the about box.  The .pro file is setup to pass
    // VER_MAJ, VER_MIN, VER_PAT as a param when the file is compiled
    // So, version info is contained only in the .pro file and can be
    // easily changed in one place.
    pWordsLabel->setText(tr("<h3>QSoloCards %1.%2.%3</h3>"
                            "<p>A collection of Solitaire Games written in Qt.</p>"
                            "<p/>"
                            "<p/>"
                            "<p>Copyright 2009 Steve Moore</p>"
                            "<p/>"
                            "<p> License: <a href="":/help/gpl3.html"">GPLv3</a></p>"
                            "<p/>"
                            "<p/>"
                            "<p>Graphics: Playing cards are a modified version of the anglo_bitmap cards from Gnome-Games' AisleRiot.</p>"
                            ).arg(QString::number(VER_MAJ)).arg(QString::number(VER_MIN)).arg(QString::number(VER_PAT)));

    connect(pWordsLabel,SIGNAL(linkActivated(QString)),
            this,SLOT(slotLinkActivated(QString)));

    pMainLayout->addWidget(pWordsLabel,0,Qt::AlignTop|Qt::AlignHCenter);


    pLayout->addLayout(pMainLayout,20);

    QDialogButtonBox * pButtonBox=new QDialogButtonBox(this);

    pButtonBox->addButton(QDialogButtonBox::Close);

    pLayout->addWidget(pButtonBox,1);

    connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // don't allow resizing the window.
    pLayout->setSizeConstraint(QLayout::SetFixedSize);

    this->setLayout(pLayout);
}
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
About::~About()
{
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
void About::slotLinkActivated(const QString & link)
{
    emit showLink(link);
}
