#include <QToolButton>

#include <retroshare-gui/mainpage.h>
#include "common/FloatingHelpBrowser.h"
#include "gui/settings/rsharesettings.h"

MainPage::MainPage(QWidget *parent , Qt::WindowFlags flags ) : QWidget(parent, flags)
{
	mHelpBrowser = NULL ;
	mIcon = QIcon();
	mName = "";
	mHelp = "";
}

void MainPage::registerHelpButton(QToolButton *button, const QString& help_html_text, const QString &code_name)
{
    mHelpCodeName = code_name ;

	if (mHelpBrowser == NULL)
		mHelpBrowser = new FloatingHelpBrowser(this, button) ;
	
	float S = QFontMetricsF(button->font()).height() ;
	button->setIconSize(QSize(S,S)) ;

	mHelpBrowser->setHelpText(help_html_text) ;
}

void MainPage::showEvent(QShowEvent *s)
{
    if(Settings->getPageFirstTimeDisplay(mHelpCodeName) && mHelpBrowser!=NULL)
    {
        mHelpBrowser->show();

        Settings->setPageFirstTimeDisplay(mHelpCodeName,false);
    }

    QWidget::showEvent(s);
}
