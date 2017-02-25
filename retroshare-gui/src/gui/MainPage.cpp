#include <QToolButton>
#include <QTimer>

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
    if(!Settings->getPageAlreadyDisplayed(mHelpCodeName) && mHelpBrowser!=NULL)
    {
        // I use a timer to make sure that the GUI is able to process that.
        QTimer::singleShot(1000, mHelpBrowser,SLOT(show()));

        Settings->setPageAlreadyDisplayed(mHelpCodeName,true);
    }

    QWidget::showEvent(s);
}
