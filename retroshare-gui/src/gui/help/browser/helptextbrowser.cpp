/*******************************************************************************
 * retroshare-gui/src/gui/help/browser/helptextbrowser.cpp                     *
 *                                                                             *
 * Copyright (c) 2008, defnax          <retroshare.project@gmail.com>          *
 * Copyright (c) 2008, Matt Edman, Justin Hipple                               *
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

/*
** \file helptextbrowser.cpp
** \version $Id: helptextbrowser.cpp 2679 2008-06-10 03:07:10Z edmanm $
** \brief Displays an HTML-based help document
*/

#include <QDir>
#include <QFile>
#include <QDesktopServices>
#include "gui/common/vmessagebox.h"
#include "gui/common/rshtml.h"
#include <rshare.h>

#include "helptextbrowser.h"


/** Default constructor. */
HelpTextBrowser::HelpTextBrowser(QWidget *parent)
  : QTextBrowser(parent)
{
  setOpenExternalLinks(false);
}

/** Loads a resource into the browser. If it is an HTML resource, we'll load
 * it as UTF-8, so the special characters in our translations appear properly. */
QVariant
HelpTextBrowser::loadResource(int type, const QUrl &name)
{
  /* If it's an HTML file, we'll handle it ourselves */
  if (type == QTextDocument::HtmlResource) {
    QString helpPath = ":/help/";
    
    /* Fall back to English if there is no translation of the specified help
     * page in the current language. */
    if (!name.path().contains("/")) {
      QString language = Rshare::language();
      if (!QDir(":/help/" + language).exists())
        language = "en";
      helpPath += language + "/";
    }
    
    QFile file(helpPath + name.path());
    if (!file.open(QIODevice::ReadOnly)) {
      return tr("Error opening help file:")+" "+ name.path();
    }
    return QString::fromUtf8(file.readAll());
  }
  /* Everything else, just let QTextBrowser take care of it. */
  return QTextBrowser::loadResource(type, name);
}


/** Called when the displayed document is changed. If <b>url</b> specifies
 * an external link, then the user will be prompted for whether they want to
 * open the link in their default browser or not. */
void
HelpTextBrowser::setSource(const QUrl &url)
{
  if (url.scheme() != "qrc" && !url.isRelative()) {
    /* External link. Prompt the user for a response. */
    int ret = VMessageBox::question(this,
                tr("Opening External Link"),
                p(tr("RetroShare can open the link you selected in your default "
                     "Web browser. If your browser is not currently "
                     "configured to use Tor then the request will not be "
                     "anonymous.")) +
                p(tr("Do you want Retroshare to open the link in your Web "
                     "browser?")),
                VMessageBox::Yes|VMessageBox::Default, 
                VMessageBox::Cancel|VMessageBox::Cancel);
    
    if (ret == VMessageBox::Cancel)
      return;
    
    bool ok = QDesktopServices::openUrl(url);
    if (!ok) {
      VMessageBox::information(this,
        tr("Unable to Open Link"),
        tr("RetroShare was unable to open the selected link in your Web browser. "
           "You can still copy the URL and paste it into your browser."),
        VMessageBox::Ok);
    }
  } else {
    /* Internal link. Just load it like normal. */
    QTextBrowser::setSource(url);
  }
}

