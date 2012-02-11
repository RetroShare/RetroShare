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
	QDesktopServices::openUrl(url);
}
