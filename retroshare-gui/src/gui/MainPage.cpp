#include <QToolButton>

#include <retroshare-gui/mainpage.h>
#include "common/FloatingHelpBrowser.h"

MainPage::MainPage(QWidget *parent , Qt::WindowFlags flags ) : QWidget(parent, flags)
{
	mHelpBrowser = NULL ;
	mIcon = QIcon();
	mName = "";
	mHelp = "";
}

void MainPage::registerHelpButton(QToolButton *button,const QString& help_html_txt)
{
	if (mHelpBrowser == NULL)
		mHelpBrowser = new FloatingHelpBrowser(this, button) ;
	
	float S = QFontMetricsF(button->font()).height() ;
	button->setIconSize(QSize(S,S)) ;

	mHelpBrowser->setHelpText(help_html_txt) ;
}

