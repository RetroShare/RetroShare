#include <QDesktopServices>
#include <QPainter>

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

void LinkTextBrowser::setPlaceholderText(const QString &text)
{
	placeholderText = text;
	viewport()->repaint();
}

void LinkTextBrowser::paintEvent(QPaintEvent *event)
{
	QTextBrowser::paintEvent(event);

	if (placeholderText.isEmpty() == false && document()->isEmpty()) {
		QWidget *vieportWidget = viewport();
		QPainter painter(vieportWidget);

		QPen pen = painter.pen();
		QColor color = pen.color();
		color.setAlpha(128);
		pen.setColor(color);
		painter.setPen(pen);

		painter.drawText(QRect(QPoint(), vieportWidget->size()), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, placeholderText);
	}
}

