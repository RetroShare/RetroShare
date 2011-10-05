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

/* Directory tree for stylesheets:

   Install version:  <RetroShare config dir>/stylesheets
   Portable version: <Application dir>/stylesheets

   stylesheets
   +history
   |-Style1
   |-Style2
   +private
   |-Style1
   |-Style2
   +public
   |-Style1
   |-Style2

 Files and dirs of the style:
   info.xml      - description
   incoming.htm  - incoming messages
   outgoing.htm  - outgoing messages
   hincoming.htm - incoming history messages
   houtgoing.htm - outgoing history messages
   ooutgoing.htm - outgoing offline messages (private chat)
   main.css      - stylesheet

   variants      - directory with variants (optional)
   +- *.css      - Stylesheets for the variants

 Example:
   info.xml
     <?xml version="1.0" encoding="UTF-8"?>
     <!DOCTYPE RetroShare_StyleInfo>
     <RetroShare_Style version="1.0">
     <style>
       <name>Name</name>
       <description>Description</description>
     </style>
     <author>
       <name>Author</name>
       <email>E-Mail</email>
     </author>
     </RetroShare_Style>

   incoming.htm
     <style type="text/css">
     %css-style%
     </style>
     <span class='incomingTime'>%time%</span>
     <span class='incomingName'><strong>%name%</strong></span>
     %message%

   outgoing.htm
     <style type="text/css">
     %css-style%
     </style>
     <span class='outgoingTime'>%time%</span>
     <span class='outgoingName'><strong>%name%</strong></span>
     %message%

   main.css
     .incomingTime {
       color:#C00000;
     }
     .incomingName{
       color:#2D84C9;
     }
     .outgoingTime {
       color:#C00000;
     }
     .outgoingName{
       color:#2D84C9;
     }
     :
     more definitions for history and offline messages

  See standard styles in retroshare-gui/src/gui/qss/chat/
*/

#include <QApplication>

#include "ChatStyle.h"
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
#include "gui/common/Emoticons.h"

#include <retroshare/rsinit.h>

enum enumGetStyle
{
    GETSTYLE_INCOMING,
    GETSTYLE_OUTGOING,
    GETSTYLE_HINCOMING,
    GETSTYLE_HOUTGOING,
    GETSTYLE_OOUTGOING
};

/* Default constructor */
ChatStyle::ChatStyle() : QObject()
{
    m_styleType = TYPE_UNKNOWN;

    connect(NotifyQt::getInstance(), SIGNAL(chatStyleChanged(int)), SLOT(styleChanged(int)));
}

/* Destructor. */
ChatStyle::~ChatStyle()
{
}

void ChatStyle::styleChanged(int styleType)
{
    if (m_styleType == styleType) {
        setStyleFromSettings(m_styleType);
    }
}

bool ChatStyle::setStylePath(const QString &stylePath, const QString &styleVariant)
{
    m_styleType = TYPE_UNKNOWN;

    m_styleDir.setPath(QApplication::applicationDirPath());
    if (m_styleDir.cd(stylePath) == false) {
        m_styleDir = QDir("");
        m_styleVariant.clear();
        return false;
    }

    m_styleVariant = styleVariant;

    return true;
}

bool ChatStyle::setStyleFromSettings(enumStyleType styleType)
{
    QString stylePath;
    QString styleVariant;

    switch (styleType) {
    case TYPE_PUBLIC:
        Settings->getPublicChatStyle(stylePath, styleVariant);
        break;
    case TYPE_PRIVATE:
        Settings->getPrivateChatStyle(stylePath, styleVariant);
        break;
    case TYPE_HISTORY:
        Settings->getHistoryChatStyle(stylePath, styleVariant);
        break;
    case TYPE_UNKNOWN:
        return false;
    }

    bool result = setStylePath(stylePath, styleVariant);

    m_styleType = styleType;

    // reset cache
    for (int i = 0; i < FORMATMSG_COUNT; i++) {
        m_style[i].clear();
    }

    return result;
}

static QString getStyle(const QDir &styleDir, const QString &styleVariant, enumGetStyle type)
{
    QString style;

    if (styleDir == QDir("")) {
        return "";
    }

    QFile fileHtml;
    switch (type) {
    case GETSTYLE_INCOMING:
        fileHtml.setFileName(QFileInfo(styleDir, "incoming.htm").absoluteFilePath());
        break;
    case GETSTYLE_OUTGOING:
        fileHtml.setFileName(QFileInfo(styleDir, "outgoing.htm").absoluteFilePath());
        break;
    case GETSTYLE_HINCOMING:
        fileHtml.setFileName(QFileInfo(styleDir, "hincoming.htm").absoluteFilePath());
        break;
    case GETSTYLE_HOUTGOING:
        fileHtml.setFileName(QFileInfo(styleDir, "houtgoing.htm").absoluteFilePath());
        break;
    case GETSTYLE_OOUTGOING:
        fileHtml.setFileName(QFileInfo(styleDir, "ooutgoing.htm").absoluteFilePath());
        break;
    default:
        return "";
    }

    if (fileHtml.open(QIODevice::ReadOnly)) {
        style = fileHtml.readAll();
        fileHtml.close();

        QFile fileCss(QFileInfo(styleDir, "main.css").absoluteFilePath());
        QString css;
        if (fileCss.open(QIODevice::ReadOnly)) {
            css = fileCss.readAll();
            fileCss.close();
        }

        if (styleVariant.isEmpty() == false) {
            QFile fileCssVariant(QFileInfo(styleDir, "variants/" + styleVariant + ".css").absoluteFilePath());
            QString cssVariant;
            if (fileCssVariant.open(QIODevice::ReadOnly)) {
                cssVariant = fileCssVariant.readAll();
                fileCssVariant.close();

                css += "\n" + cssVariant;
            }
        }

        style.replace("%css-style%", css);
    }

    return style;
}

QString ChatStyle::formatMessage(enumFormatMessage type, const QString &name, const QDateTime &timestamp, const QString &message, unsigned int flag)
{
    QString style = m_style[type];

    if (style.isEmpty()) {
        switch (type) {
        case FORMATMSG_INCOMING:
            style = getStyle(m_styleDir, m_styleVariant, GETSTYLE_INCOMING);
            break;
        case FORMATMSG_OUTGOING:
            style = getStyle(m_styleDir, m_styleVariant, GETSTYLE_OUTGOING);
            break;
        case FORMATMSG_HINCOMING:
            style = getStyle(m_styleDir, m_styleVariant, GETSTYLE_HINCOMING);
            break;
        case FORMATMSG_HOUTGOING:
            style = getStyle(m_styleDir, m_styleVariant, GETSTYLE_HOUTGOING);
            break;
        case FORMATMSG_OOUTGOING:
            style = getStyle(m_styleDir, m_styleVariant, GETSTYLE_OOUTGOING);
            break;
        }

        if (style.isEmpty()) {
            // default style
            style = "<table width='100%'><tr><td><b>%name%</b></td><td width='130' align='right'>%time%</td></tr></table><table width='100%'><tr><td>%message%</td></tr></table>";
        }

        m_style[type] = style;
    }

    unsigned int formatFlag = 0;
    if (flag & CHAT_FORMATMSG_EMBED_SMILEYS) {
        formatFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
    }
    if (flag & CHAT_FORMATMSG_EMBED_LINKS) {
        formatFlag |= RSHTML_FORMATTEXT_EMBED_LINKS;
    }

    QString msg = RsHtml::formatText(message, formatFlag);

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
//                message.replace(code, "<img src=\"" + i.value() + "\">");
//	}
//    }

    QString formatMsg = style.replace("%name%", name)
                             .replace("%date%", timestamp.date().toString("dd.MM.yyyy"))
                             .replace("%time%", timestamp.time().toString("hh:mm:ss"))
                             .replace("%message%", msg);

    return formatMsg;
}

static bool getStyleInfo(QString stylePath, QString stylePathRelative, ChatStyleInfo &info)
{
    // Initialize info
    info = ChatStyleInfo();

    QFileInfo file(stylePath, "info.xml");

    QFile xmlFile(file.filePath());
    if (xmlFile.open(QIODevice::ReadOnly) == false) {
        // No info file found
        return false;
    }

    QXmlStreamReader reader;
    reader.setDevice(&xmlFile);

    while (reader.atEnd() == false) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == "RetroShare_Style") {
                if (reader.attributes().value("version") == "1.0") {
                    info.stylePath = stylePathRelative;
                    continue;
                }
                // Not the right format of the xml file;
                return false ;
            }

            if (info.stylePath.isEmpty()) {
                continue;
            }

            if (reader.name() == "style") {
                // read style information
                while (reader.atEnd() == false) {
                    reader.readNext();
                    if (reader.isEndElement()) {
                        if (reader.name() == "style") {
                            break;
                        }
                        continue;
                    }
                    if (reader.isStartElement()) {
                        if (reader.name() == "name") {
                            info.styleName = reader.readElementText();
                            continue;
                        }
                        if (reader.name() == "description") {
                            info.styleDescription = reader.readElementText();
                            continue;
                        }
                        // ingore all other entries
                    }
                }
                continue;
            }

            if (reader.name() == "author") {
                // read author information
                while (reader.atEnd() == false) {
                    reader.readNext();
                    if (reader.isEndElement()) {
                        if (reader.name() == "author") {
                            break;
                        }
                        continue;
                    }
                    if (reader.isStartElement()) {
                        if (reader.name() == "name") {
                            info.authorName = reader.readElementText();
                            continue;
                        }
                        if (reader.name() == "email") {
                            info.authorEmail = reader.readElementText();
                            continue;
                        }
                        // ingore all other entries
                    }
                }
                continue;
            }
            // ingore all other entries
        }
    }

    if (reader.hasError()) {
        return false;
    }

    if (info.stylePath.isEmpty()) {
        return false;
    }
    return true;
}

static QString getBaseDir()
{
    // application path
    std::string configDir = RsInit::RsConfigDirectory();
    QString baseDir = QString::fromStdString(configDir);

#ifdef WIN32
    if (RsInit::isPortable ()) {
        // application dir for portable version
        baseDir = QApplication::applicationDirPath();
    }
#endif

    return baseDir;
}

/*static*/ bool ChatStyle::getAvailableStyles(enumStyleType styleType, QList<ChatStyleInfo> &styles)
{
    styles.clear();

    // base dir
    QDir baseDir(getBaseDir());

    ChatStyleInfo standardInfo;
    QString stylePath;

    switch (styleType) {
    case TYPE_PUBLIC:
        if (getStyleInfo(":/qss/chat/public", ":/qss/chat/public", standardInfo)) {
            standardInfo.styleDescription = tr("Standard style for group chat");
            styles.append(standardInfo);
        }
        stylePath = "stylesheets/public";
        break;
    case TYPE_PRIVATE:
        if (getStyleInfo(":/qss/chat/private", ":/qss/chat/private", standardInfo)) {
            standardInfo.styleDescription = tr("Standard style for private chat");
            styles.append(standardInfo);
        }
        stylePath = "stylesheets/private";
        break;
    case TYPE_HISTORY:
        if (getStyleInfo(":/qss/chat/history", ":/qss/chat/history", standardInfo)) {
            standardInfo.styleDescription = tr("Standard style for history");
            styles.append(standardInfo);
        }
        stylePath = "stylesheets/history";
        break;
    case TYPE_UNKNOWN:
    default:
        return false;
    }

    QDir dir(baseDir);
    if (dir.cd(stylePath) == false) {
        // no user styles available
        return true;
    }

    // get all style directories
    QFileInfoList dirList = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);

    // iterate style directories and get info
    for (QFileInfoList::iterator dir = dirList.begin(); dir != dirList.end(); dir++) {
        ChatStyleInfo info;
        if (getStyleInfo(dir->absoluteFilePath(), baseDir.relativeFilePath(dir->absoluteFilePath()), info)) {
            styles.append(info);
        }
    }

    return true;
}

/*static*/ bool ChatStyle::getAvailableVariants(const QString &stylePath, QStringList &variants)
{
    variants.clear();

    if (stylePath.isEmpty()) {
        return false;
    }

    // application path
    QDir dir(QApplication::applicationDirPath());
    if (dir.cd(stylePath) == false) {
        // style not found
        return false;
    }

    if (dir.cd("variants") == false) {
        // no variants available
        return true;
    }

    // get all variants
    QStringList filters;
    filters.append("*.css");
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    // iterate variants
    for (QFileInfoList::iterator file = fileList.begin(); file != fileList.end(); file++) {
        variants.append(file->baseName());
    }

    return true;
}
