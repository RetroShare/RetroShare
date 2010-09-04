/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, Thunder
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

#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include <QFile>
#include <QIcon>
#include <QPushButton>
#include <iostream>

#include "ChatStyle.h"

enum enumGetStyle
{
    GETSTYLE_INCOMING,
    GETSTYLE_OUTGOING,
    GETSTYLE_HINCOMING,
    GETSTYLE_HOUTGOING
};

/* Default constructor */
ChatStyle::ChatStyle()
{
}

/* Destructor. */
ChatStyle::~ChatStyle()
{
}

void ChatStyle::setStylePath(QString path)
{
    stylePath = path;
    if (stylePath.right(1) != "/" && stylePath.right(1) != "\\") {
        stylePath += "/";
    }
}

void ChatStyle::loadEmoticons()
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
            if (internalEmoticons) {
                smileys.insert(smcode, ":/"+smfile);
            } else {
                smileys.insert(smcode, smfile);
            }
        }
    }

    // init <img> embedder
    defEmbedImg.InitFromAwkwardHash(smileys);
}

void ChatStyle::showSmileyWidget(QWidget *parent, QWidget *button, const char *slotAddMethod)
{
    QWidget *smWidget = new QWidget(parent, Qt::Popup);

    smWidget->setAttribute( Qt::WA_DeleteOnClose);
    smWidget->setWindowTitle("Emoticons");
    smWidget->setWindowIcon(QIcon(QString(":/images/rstray3.png")));
    smWidget->setBaseSize(4*24, (smileys.size()/4)*24);

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
    // je�li wychodzi poza g�rn� kraw�d� to r�wnamy do niej
    if (y < 0)
        y = 0;
    // ustawiamy selektor na wyliczonej pozycji
    smWidget->move(x, y);

    x = 0;
    y = 0;

    QHashIterator<QString, QString> i(smileys);
    while(i.hasNext())
    {
        i.next();
        QPushButton *smButton = new QPushButton("", smWidget);
        smButton->setGeometry(x*24, y*24, 24,24);
        smButton->setIconSize(QSize(24,24));
        smButton->setIcon(QPixmap(i.value()));
        smButton->setToolTip(i.key());
        ++x;
        if(x > 4)
        {
            x = 0;
            y++;
        }
        QObject::connect(smButton, SIGNAL(clicked()), parent, slotAddMethod);
        QObject::connect(smButton, SIGNAL(clicked()), smWidget, SLOT(close()));
    }

    smWidget->show();
}

static QString getStyle(QString &stylePath, enumGetStyle type)
{
    QString style;

    if (stylePath.isEmpty()) {
        return "";
    }

    QFile fileHtml;
    switch (type) {
    case GETSTYLE_INCOMING:
        fileHtml.setFileName(stylePath + "incoming.htm");
        break;
    case GETSTYLE_OUTGOING:
        fileHtml.setFileName(stylePath + "outgoing.htm");
        break;
    case GETSTYLE_HINCOMING:
        fileHtml.setFileName(stylePath + "hincoming.htm");
        break;
    case GETSTYLE_HOUTGOING:
        fileHtml.setFileName(stylePath + "houtgoing.htm");
        break;
    default:
        return "";
    }

    if (fileHtml.open(QIODevice::ReadOnly)) {
        style = fileHtml.readAll();
        fileHtml.close();

        QFile fileCss(stylePath + "main.css");
        QString css;
        if (fileCss.open(QIODevice::ReadOnly)) {
            css = fileCss.readAll();
            fileCss.close();
        }
        style.replace("%css-style%", css);
    }

    return style;
}

QString ChatStyle::formatText(QString &message, unsigned int flag)
{
    if (flag == 0) {
        // nothing to do
        return message;
    }

    QDomDocument doc;
    doc.setContent(message);

    QDomElement body = doc.documentElement();
    if (flag & CHAT_FORMATTEXT_EMBED_LINKS) {
        RsChat::embedHtml(doc, body, defEmbedAhref);
    }
    if (flag & CHAT_FORMATTEXT_EMBED_SMILEYS) {
        RsChat::embedHtml(doc, body, defEmbedImg);
    }

    return doc.toString(-1);		// -1 removes any annoying carriage return misinterpreted by QTextEdit
}

QString ChatStyle::formatMessage(enumFormatMessage type, QString &name, QDateTime &timestamp, QString &message, unsigned int flag)
{
    QString style;

    switch (type) {
    case FORMATMSG_INCOMING:
        style = getStyle(stylePath, GETSTYLE_INCOMING);
        break;
    case FORMATMSG_OUTGOING:
        style = getStyle(stylePath, GETSTYLE_OUTGOING);
        break;
    case FORMATMSG_HINCOMING:
        style = getStyle(stylePath, GETSTYLE_HINCOMING);
        break;
    case FORMATMSG_HOUTGOING:
        style = getStyle(stylePath, GETSTYLE_HOUTGOING);
        break;
    }

    if (style.isEmpty()) {
        // default style
        style = "%timestamp% %name% \n %message% ";
    }

    unsigned int formatFlag = 0;
    if (flag & CHAT_FORMATMSG_EMBED_SMILEYS) {
        formatFlag |= CHAT_FORMATTEXT_EMBED_SMILEYS;
    }
    if (flag & CHAT_FORMATMSG_EMBED_LINKS) {
        formatFlag |= CHAT_FORMATTEXT_EMBED_LINKS;
    }

    QString msg = formatText(message, formatFlag);

//    //replace http://, https:// and www. with <a href> links
//    QRegExp rx("(retroshare://[^ <>]*)|(https?://[^ <>]*)|(www\\.[^ <>]*)");
//    int count = 0;
//    int pos = 100; //ignore the first 100 char because of the standard DTD ref
//    while ( (pos = rx.indexIn(message, pos)) != -1 ) {
//        //we need to look ahead to see if it's already a well formed link
//        if (message.mid(pos - 6, 6) != "href=\"" && message.mid(pos - 6, 6) != "href='" && message.mid(pos - 6, 6) != "ttp://" ) {
//            QString tempMessg = message.left(pos) + "<a href=\"" + rx.cap(count) + "\">" + rx.cap(count) + "</a>" + message.mid(pos + rx.matchedLength(), -1);
//            message = tempMessg;
//        }
//        pos += rx.matchedLength() + 15;
//        count ++;
//    }

//    {
//	QHashIterator<QString, QString> i(smileys);
//	while(i.hasNext())
//	{
//            i.next();
//            foreach(QString code, i.key().split("|"))
//                message.replace(code, "<img src=\"" + i.value() + "\" />");
//	}
//    }

    QString formatMsg = style.replace("%name%", name)
                             .replace("%timestamp%", timestamp.toString("hh:mm:ss"))
                             .replace("%message%", msg);

    return formatMsg;
}
