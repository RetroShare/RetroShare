#include <QDesktopServices>

#include "LinkTextBrowser.h"

LinkTextBrowser::LinkTextBrowser(QWidget *parent) :
	QTextBrowser(parent)
{
	setOpenExternalLinks(true);
	setOpenLinks(false);

	connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
}

void LinkTextBrowser::linkClicked(const QUrl &url)
{
	// some links are opened directly in the QTextBrowser with open external links set to true,
	// so we handle links by our own

#ifdef TO_DO
	// If we want extra file links to be anonymous, we need to insert the actual source here.
	if(url.host() == HOST_EXTRAFILE)
	{
		std::cerr << "Extra file link detected. Adding parent id " << _target_sslid << " to sourcelist" << std::endl;

		RetroShareLink link ;
		link.fromUrl(url) ;

		link.createExtraFile( link.name(),link.size(),link.hash(), _target_ssl_id) ;

		QDesktopServices::openUrl(link.toUrl());
	}
	else
#endif
		QDesktopServices::openUrl(url);
}

