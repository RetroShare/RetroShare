/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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

#include <QHash>
#include <QFile>
#include <QApplication>
#include <QWidget>
#include <QFile>
#include <QIcon>
#include <QDesktopWidget>
#include <QPushButton>

#include <iostream>

#include "Emoticons.h"

static QHash<QString, QString> Smileys;
RsChat::EmbedInHtmlImg Emoticons::defEmbedImg;

void Emoticons::load()
{
    QString sm_codes;
    bool internalEmoticons = true;

#if defined(Q_OS_WIN32)
    // first try external emoticons
    QFile sm_file(QApplication::applicationDirPath() + "/emoticons/emotes.acs");
    if(sm_file.open(QIODevice::ReadOnly))
    {
        internalEmoticons = false;
    } else {
        // then embedded emotions
        sm_file.setFileName(":/smileys/emotes.acs");
        if(!sm_file.open(QIODevice::ReadOnly))
        {
            std::cout << "error opening ressource file" << std::endl ;
            return ;
        }
    }
#else
    QFile sm_file(QString(":/smileys/emotes.acs"));
    if(!sm_file.open(QIODevice::ReadOnly))
    {
        std::cout << "error opening ressource file" << std::endl ;
        return ;
    }
#endif

    sm_codes = sm_file.readAll();
    sm_file.close();

    sm_codes.remove("\n");
    sm_codes.remove("\r");

    int i = 0;
    QString smcode;
    QString smfile;
    while(sm_codes[i] != '{')
    {
        i++;
    }
    while (i < sm_codes.length()-2)
    {
        smcode = "";
        smfile = "";

        while(sm_codes[i] != '\"')
        {
            i++;
        }
        i++;
        while (sm_codes[i] != '\"')
        {
            smcode += sm_codes[i];
            i++;
        }
        i++;

        while(sm_codes[i] != '\"')
        {
            i++;
        }
        i++;
        while(sm_codes[i] != '\"' && sm_codes[i+1] != ';')
        {
            smfile += sm_codes[i];
            i++;
        }
        i++;
        if(!smcode.isEmpty() && !smfile.isEmpty()) {
            while (smcode.right(1) == "|") {
                smcode.remove(smcode.length() - 1, 1);
            }
   
            if (internalEmoticons) {
                Smileys.insert(smcode, ":/"+smfile);
            } else {
                Smileys.insert(smcode, smfile);
            }
        }
    }

    // init <img> embedder
    defEmbedImg.InitFromAwkwardHash(Smileys);
}

void Emoticons::showSmileyWidget(QWidget *parent, QWidget *button, const char *slotAddMethod, bool above)
{
    QWidget *smWidget = new QWidget(parent, Qt::Popup);

    const int buttonWidth = 26;
    const int buttonHeight = 26;
    const int countPerLine = 9;

    int rowCount = (Smileys.size()/countPerLine) + ((Smileys.size() % countPerLine) ? 1 : 0);

    smWidget->setAttribute( Qt::WA_DeleteOnClose);
    smWidget->setWindowTitle("Emoticons");
    smWidget->setWindowIcon(QIcon(QString(":/images/rstray3.png")));
    smWidget->setBaseSize(countPerLine*buttonWidth, rowCount*buttonHeight);

    //Warning: this part of code was taken from kadu instant messenger;
    //         It was EmoticonSelector::alignTo(QWidget* w) function there
    //         comments are Polish, I dont' know how does it work...
    // oblicz pozycj� widgetu do kt�rego r�wnamy
    // oblicz rozmiar selektora
    QPoint pos = button->mapToGlobal(QPoint(0,0));
    QSize e_size = smWidget->sizeHint();
    // oblicz rozmiar pulpitu
    QSize s_size = QApplication::desktop()->size();
    // oblicz dystanse od widgetu do lewego brzegu i do prawego
    int l_dist = pos.x();
    int r_dist = s_size.width() - (pos.x() + button->width());
    // oblicz pozycj� w zale�no�ci od tego czy po lewej stronie
    // jest wi�cej miejsca czy po prawej
    int x;
    if (l_dist >= r_dist)
        x = pos.x() - e_size.width();
    else
        x = pos.x() + button->width();
    // oblicz pozycj� y - centrujemy w pionie
    int y = pos.y() + button->height()/2 - e_size.height()/2;
    // je�li wychodzi poza doln� kraw�d� to r�wnamy do niej
    if (y + e_size.height() > s_size.height())
        y = s_size.height() - e_size.height();

    if (above) {
        y -= rowCount*buttonHeight;
    }

    // je�li wychodzi poza g�rn� kraw�d� to r�wnamy do niej
    if (y < 0)
        y = 0;
    // ustawiamy selektor na wyliczonej pozycji
    smWidget->move(x, y);

    x = 0;
    y = 0;

    QHashIterator<QString, QString> i(Smileys);
    while(i.hasNext())
    {
        i.next();
        QPushButton *smButton = new QPushButton("", smWidget);
        smButton->setGeometry(x*buttonWidth, y*buttonHeight, buttonWidth, buttonHeight);
        smButton->setIconSize(QSize(buttonWidth, buttonHeight));
        smButton->setIcon(QPixmap(i.value()));
        smButton->setToolTip(i.key());
        smButton->setStyleSheet("QPushButton:hover {border: 3px solid white; border-radius: 2px;}");
        smButton->setFlat(true);
        ++x;
        if(x >= countPerLine)
        {
            x = 0;
            y++;
        }
        QObject::connect(smButton, SIGNAL(clicked()), parent, slotAddMethod);
        QObject::connect(smButton, SIGNAL(clicked()), smWidget, SLOT(close()));
    }

    smWidget->show();
}

void Emoticons::formatText(QString &text)
{
    QHashIterator<QString, QString> i(Smileys);
    while(i.hasNext()) {
        i.next();
        foreach (QString code, i.key().split("|")) {
            text.replace(code, "<img src=\"" + i.value() + "\" />");
        }
    }
}
