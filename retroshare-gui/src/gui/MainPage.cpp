#include <iostream>
#include <retroshare-gui/mainpage.h>
#include <QGraphicsBlurEffect>
#include <QGraphicsDropShadowEffect>

MainPage::MainPage(QWidget *parent , Qt::WindowFlags flags ) : QWidget(parent, flags) 
{
	help_browser = NULL ;
}


void MainPage::showHelp(bool b) 
{
	if(help_browser == NULL)
	{
		help_browser = new QTextBrowser(this) ;
		help_browser->setHtml(helpHtmlText()) ;

		QGraphicsDropShadowEffect * effect = new QGraphicsDropShadowEffect(help_browser) ;
		effect->setBlurRadius(30.0);
		help_browser->setGraphicsEffect(effect);

		help_browser->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum)) ;
		help_browser->resize(size()*0.5) ;
		help_browser->move(width()/2 - help_browser->width()/2,height()/2 - help_browser->height()/2);
		help_browser->update() ;
	}
	std::cerr << "Toggling help to " << b << std::endl;
	if(b)
		help_browser->show() ;
	else
		help_browser->hide() ;
}


