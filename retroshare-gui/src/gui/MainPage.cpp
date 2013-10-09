#include <retroshare-gui/mainpage.h>
#include "common/FloatingHelpBrowser.h"

MainPage::MainPage(QWidget *parent , Qt::WindowFlags flags ) : QWidget(parent, flags) 
{
	mHelpBrowser = NULL ;
}

void MainPage::registerHelpButton(QAbstractButton *button,const QString& help_html_txt)
{
	if (mHelpBrowser == NULL)
	{
		mHelpBrowser = new FloatingHelpBrowser(this, button) ;
	}

	mHelpBrowser->setHelpText(help_html_txt) ;
}

