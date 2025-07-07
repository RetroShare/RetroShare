/*******************************************************************************
 * gui/chat/ChatStyle.cpp                                                      *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2006, Thunder                                                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

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
#include <QDomDocument>
#include <QTextStream>

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

static QStringList getBaseDirList()
{
    // Search chat styles in config dir and data dir (is application dir for portable)
    QStringList baseDirs;
    baseDirs.append(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str()));
    baseDirs.append(QString::fromUtf8(RsAccounts::systemDataDirectory().c_str()));

    return baseDirs;
}

bool ChatStyle::setStylePath(const QString &stylePath, const QString &styleVariant)
{
    m_styleType = TYPE_UNKNOWN;

    foreach (QString dir, getBaseDirList()) {
        m_styleDir.setPath(dir);
        if (m_styleDir.cd(stylePath)) {
            m_styleVariant = styleVariant;
            return true;
        }
    }
    m_styleDir.setPath("");
    m_styleVariant.clear();
    return false;
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
    for (int i = 0; i < FORMATMSG_COUNT; ++i) {
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

QString ChatStyle::formatMessage(enumFormatMessage type
                                 , const QString &name
                                 , const QString &gxsid
                                 , const QDateTime &timestamp
                                 , const QString &message
                                 , unsigned int flag
                                 , const QColor &backgroundColor /*= Qt::white*/
                                 )
{
	bool me = false;
	QDomDocument doc ;
	QString styleOptimized ;
	QString errorMsg ; int errorLine ; int errorColumn ;
	QString messageBody = message ;
	me = me || message.trimmed().startsWith("/me ");
	if (doc.setContent(messageBody, &errorMsg, &errorLine, &errorColumn)) {
		QDomElement body = doc.documentElement();
		if (!body.isNull()){
			messageBody = "";
			int count = body.childNodes().count();
			for (int curs = 0; curs < count; ++curs){
				QDomNode it = body.childNodes().item(curs);
				if (it.nodeName().toLower() != "style") {
					//find out if the message starts with /me
					if(it.isText()){
						me = me || it.toText().data().trimmed().startsWith("/me ");
					}else if(it.isElement()){
						me = me || it.toElement().text().trimmed().startsWith("/me ");
					}
					QString str;
					QTextStream stream(&str);
					it.toElement().save(stream, -1);
					//remove Body element, as it was saved with...
					if ((str.startsWith("<body>")) && (str.endsWith( "</body>"))) {
						str.remove(0, 6);
						str.chop(     7);
					}
					//remove first <span> if it is without attribute
					if ((str.startsWith("<span>")) && (str.endsWith( "</span>"))) {
						str.remove(0, 6);
						str.chop(     7);
					}
					messageBody += str;
				} else {
					QDomText text = it.firstChild().toText();
					styleOptimized = text.data();
				}
			}
		}
	}
	if (messageBody.isEmpty()) messageBody = message;

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
	QString colorName;

	if (flag & CHAT_FORMATMSG_SYSTEM) {
		color = Qt::darkBlue;
	} else {
		// Calculate color from the name
		uint hash = 0;
		for (int i = 0; i < gxsid.length(); ++i) {
			hash = (((hash << 1) + (hash >> 14)) ^ ((int) gxsid[i].toLatin1())) & 0x3fff;
		}

		color.setHsv(hash, 255, 150);
		// Always fix colors
		qreal desiredContrast = Settings->valueFromGroup("Chat", "MinimumContrast", 4.5).toDouble();
		colorName = color.name();
		RsHtml::findBestColor(colorName, backgroundColor, desiredContrast);
	}
#else
	Q_UNUSED(flag);
	Q_UNUSED(backgroundColor);
#endif

	QString strName = RsHtml::plainText(name).prepend(QString("<a name=\"name\">")).append(QString("</a>"));
	QString strDate = DateTime::formatDate(timestamp.date()).prepend(QString("<a name=\"date\">")).append(QString("</a>"));
	QString strTime = DateTime::formatTime(timestamp.time()).prepend(QString("<a name=\"time\">")).append(QString("</a>"));

	int bi = name.lastIndexOf(QRegExp(" \\(.*\\)")); //trim location from the end
	QString strShortName = RsHtml::plainText(name.left(bi)).prepend(QString("<a name=\"name\">")).append(QString("</a>"));

	//handle /me
	//%nome% and %me% for including formatting conditionally
	//meName class for modifying the style of the name in the palce of /me
	if(me){
		messageBody = messageBody.replace(messageBody.indexOf("/me "), 3, strShortName.prepend(QString("<span class=\"meName\">")).append(QString("</span>"))); //replace only the first /me
		style = style.remove(QRegExp("%nome%.*%/nome%")).remove("%me%").remove("%/me%");
	} else {
		style = style.remove(QRegExp("%me%.*%/me%")).remove("%nome%").remove("%/nome%");
	}

	QString formatMsg = style.replace("%name%", strName)
							 .replace("%shortname%", strShortName)
	                         .replace("%date%", strDate)
	                         .replace("%time%", strTime)
#ifdef COLORED_NICKNAMES
	                         .replace("%color%", colorName)
#endif
	                         .replace("%message%", messageBody ) ;
	if ( !styleOptimized.isEmpty() ) {
		int pos = formatMsg.indexOf("</style>") ;
		if ( pos >= 0 ) {
			formatMsg.insert(pos, styleOptimized) ;
		}
	}
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

    foreach (QDir baseDir, getBaseDirList()) {
        QDir dir(baseDir);
        if (dir.cd("stylesheets") == false) {
            // no user styles available here
            continue;
        }

        // get all style directories
        QFileInfoList dirList = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);

        // iterate style directories and get info
        for (QFileInfoList::iterator it = dirList.begin(); it != dirList.end(); ++it) {
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
    }

    return true;
}

/*static*/ bool ChatStyle::getAvailableVariants(const QString &stylePath, QStringList &variants)
{
    variants.clear();

    std::cerr << "Getting variants for style: \"" << stylePath.toStdString() << "\"" << std::endl;

    if (stylePath.isEmpty()) {
    	std::cerr << "empty!" << std::endl;
        return false;
    }

    // application path
    QDir dir(QApplication::applicationDirPath());
    if (dir.cd(stylePath) == false) {
        // style not found
    	std::cerr << "no path!" << std::endl;
        return false;
    }

    if (dir.cd("variants") == false) {
        // no variants available
    	std::cerr << "no variants!" << std::endl;
        return true;
    }

    // get all variants
    QStringList filters;
    filters.append("*.css");
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    // iterate variants
    for (QFileInfoList::iterator file = fileList.begin(); file != fileList.end(); ++file) {
#ifndef COLORED_NICKNAMES
        if (file->baseName().toLower() == "colored") {
            continue;
        }
#endif
        variants.append(file->baseName());
    }

    return true;
}
