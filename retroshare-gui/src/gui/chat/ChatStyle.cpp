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
   +style1
   |-history
   |-private
   |-public
   +style2
   |-history
   |-private
   |-public

 Files and dirs of the style:
   info.xml      - description
   incoming.htm  - incoming messages
   outgoing.htm  - outgoing messages
   hincoming.htm - incoming history messages
   houtgoing.htm - outgoing history messages
   ooutgoing.htm - outgoing offline messages (private chat)
   system.htm    - system messages
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
#include <QColor>
#include <QXmlStreamReader>

#include "ChatStyle.h"
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
#include "util/DateTime.h"
#include "util/HandleRichText.h"

#include <retroshare/rsinit.h>

#define COLORED_NICKNAMES

enum enumGetStyle
{
    GETSTYLE_INCOMING,
    GETSTYLE_OUTGOING,
    GETSTYLE_HINCOMING,
    GETSTYLE_HOUTGOING,
    GETSTYLE_OOUTGOING,
    GETSTYLE_SYSTEM
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

static QString getBaseDir()
{
    // application path
    QString baseDir = QString::fromUtf8(RsInit::RsConfigDirectory().c_str());

#ifdef WIN32
    if (RsInit::isPortable ()) {
        // application dir for portable version
        baseDir = QApplication::applicationDirPath();
    }
#endif

    return baseDir;
}

bool ChatStyle::setStylePath(const QString &stylePath, const QString &styleVariant)
{
    m_styleType = TYPE_UNKNOWN;

    m_styleDir.setPath(getBaseDir());
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
    case GETSTYLE_SYSTEM:
        fileHtml.setFileName(QFileInfo(styleDir, "system.htm").absoluteFilePath());
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

        QString variant = styleVariant;
        if (styleVariant.isEmpty()) {
            // use standard
            variant = "Standard";
        }

        QFile fileCssVariant(QFileInfo(styleDir, "variants/" + variant + ".css").absoluteFilePath());
        QString cssVariant;
        if (fileCssVariant.open(QIODevice::ReadOnly)) {
            cssVariant = fileCssVariant.readAll();
            fileCssVariant.close();

            css += "\n" + cssVariant;
        }

        style.replace("%css-style%", css).replace("%style-dir%", "file:///" + styleDir.absolutePath());
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
        case FORMATMSG_SYSTEM:
            style = getStyle(m_styleDir, m_styleVariant, GETSTYLE_SYSTEM);
            break;
        }

        if (style.isEmpty()) {
            // default style
            style = "<table width='100%'><tr><td><b>%name%</b></td><td width='130' align='right'>%time%</td></tr></table><table width='100%'><tr><td>%message%</td></tr></table>";
        }

        m_style[type] = style;
    }

#ifdef COLORED_NICKNAMES
    QColor color;
    if (flag & CHAT_FORMATMSG_SYSTEM) {
        color = Qt::darkBlue;
    } else {
        // Calculate color from the name
//        uint hash = 0x73ffe23a;
//        for (int i = 0; i < name.length(); ++i) {
//            hash = (hash ^ 0x28594888) + ((uint32_t) name[i].toAscii() << (i % 4));
//        }

        uint hash = 0;
        for (int i = 0; i < name.length(); ++i) {
            hash = (((hash << 1) + (hash >> 14)) ^ ((int) name[i].toAscii())) & 0x3fff;
        }

        color.setHsv(hash, 255, 150);
    }
#else
    Q_UNUSED(flag);
#endif

    QString formatMsg = style.replace("%name%", RsHtml::plainText(name))
                             .replace("%date%", DateTime::formatDate(timestamp.date()))
                             .replace("%time%", DateTime::formatTime(timestamp.time()))
#ifdef COLORED_NICKNAMES
                             .replace("%color%", color.name())
#endif
                             .replace("%message%", message);

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

/*static*/ bool ChatStyle::getAvailableStyles(enumStyleType styleType, QList<ChatStyleInfo> &styles)
{
    styles.clear();

    // base dir
    QDir baseDir(getBaseDir());

    ChatStyleInfo standardInfo;
    QString stylePath;

    switch (styleType) {
    case TYPE_PUBLIC:
        if (getStyleInfo(":/qss/chat/standard/public", ":/qss/chat/standard/public", standardInfo)) {
            standardInfo.styleDescription = tr("Standard style for group chat");
            styles.append(standardInfo);
        }
        if (getStyleInfo(":/qss/chat/compact/public", ":/qss/chat/compact/public", standardInfo)) {
            standardInfo.styleDescription = tr("Compact style for group chat");
            styles.append(standardInfo);
        }
        stylePath = "public";
        break;
    case TYPE_PRIVATE:
        if (getStyleInfo(":/qss/chat/standard/private", ":/qss/chat/standard/private", standardInfo)) {
            standardInfo.styleDescription = tr("Standard style for private chat");
            styles.append(standardInfo);
        }
        if (getStyleInfo(":/qss/chat/compact/private", ":/qss/chat/compact/private", standardInfo)) {
            standardInfo.styleDescription = tr("Compact style for private chat");
            styles.append(standardInfo);
        }
        stylePath = "private";
        break;
    case TYPE_HISTORY:
        if (getStyleInfo(":/qss/chat/standard/history", ":/qss/chat/standard/history", standardInfo)) {
            standardInfo.styleDescription = tr("Standard style for history");
            styles.append(standardInfo);
        }
        if (getStyleInfo(":/qss/chat/compact/history", ":/qss/chat/compact/history", standardInfo)) {
            standardInfo.styleDescription = tr("Compact style for history");
            styles.append(standardInfo);
        }
        stylePath = "history";
        break;
    case TYPE_UNKNOWN:
    default:
        return false;
    }

    QDir dir(baseDir);
    if (dir.cd("stylesheets") == false) {
        // no user styles available
        return true;
    }

    // get all style directories
    QFileInfoList dirList = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);

    // iterate style directories and get info
    for (QFileInfoList::iterator it = dirList.begin(); it != dirList.end(); it++) {
        QDir styleDir = QDir(it->absoluteFilePath());
        if (styleDir.cd(stylePath) == false) {
            // no user styles available
            continue;
        }
        ChatStyleInfo info;
        if (getStyleInfo(styleDir.absolutePath(), baseDir.relativeFilePath(styleDir.absolutePath()), info)) {
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
#ifndef COLORED_NICKNAMES
        if (file->baseName().toLower() == "colored") {
            continue;
        }
#endif
        variants.append(file->baseName());
    }

    return true;
}
