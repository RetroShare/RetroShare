/*******************************************************************************
 * gui/MainPage.cpp                                                            *
 *                                                                             *
 * Copyright (c) 2006 Retroshare Team  <retroshare.project@gmail.com>          *
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

#include <retroshare-gui/mainpage.h>

#include "common/FloatingHelpBrowser.h"
#include "gui/settings/rsharesettings.h"
#include "util/misc.h"

#include <QToolButton>
#include <QTimer>

MainPage::MainPage(QWidget *parent , Qt::WindowFlags flags ) : QWidget(parent, flags)
{
	mHelpBrowser = NULL ;
	mIcon = QIcon();
	mName = "";
	mHelp = "";
    mUserNotify = nullptr;
}

UserNotify *MainPage::getUserNotify()
{
    if(!mUserNotify)
        mUserNotify = createUserNotify(this);

    return mUserNotify;
}

void MainPage::registerHelpButton(QToolButton *button, const QString& help_html_text, const QString &code_name)
{
	mHelpCodeName = code_name ;

	if (mHelpBrowser == nullptr)
		mHelpBrowser = new FloatingHelpBrowser(this, button) ;
	
	int H = misc::getFontSizeFactor("HelpButton").height();
	button->setIconSize(QSize(H, H)) ;//Square Icon

	mHelpBrowser->setHelpText(help_html_text) ;
}

void MainPage::showEvent(QShowEvent *s)
{
    if(!Settings->getPageAlreadyDisplayed(mHelpCodeName) && mHelpBrowser!=NULL)
    {
        // I use a timer to make sure that the GUI is able to process that.
        QTimer::singleShot(1000, mHelpBrowser,SLOT(show()));

        Settings->setPageAlreadyDisplayed(mHelpCodeName,true);
    }

    QWidget::showEvent(s);
}
