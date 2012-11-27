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
