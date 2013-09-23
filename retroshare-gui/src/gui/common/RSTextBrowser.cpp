#include <QDesktopServices>
#include <QPainter>

#include "RSTextBrowser.h"
#include "RSImageBlockWidget.h"

RSTextBrowser::RSTextBrowser(QWidget *parent) :
	QTextBrowser(parent)
{
	setOpenExternalLinks(true);
	setOpenLinks(false);

	mShowImages = true;
	mImageBlockWidget = NULL;

	connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
}

void RSTextBrowser::linkClicked(const QUrl &url)
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

void RSTextBrowser::setPlaceholderText(const QString &text)
{
	mPlaceholderText = text;
	viewport()->repaint();
}

void RSTextBrowser::paintEvent(QPaintEvent *event)
{
	QTextBrowser::paintEvent(event);

	if (mPlaceholderText.isEmpty() == false && document()->isEmpty()) {
		QWidget *vieportWidget = viewport();
		QPainter painter(vieportWidget);

		QPen pen = painter.pen();
		QColor color = pen.color();
		color.setAlpha(128);
		pen.setColor(color);
		painter.setPen(pen);

		painter.drawText(QRect(QPoint(), vieportWidget->size()), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, mPlaceholderText);
	}
}

QVariant RSTextBrowser::loadResource(int type, const QUrl &name)
{
	if (mShowImages || type != QTextDocument::ImageResource || name.scheme().compare("data", Qt::CaseInsensitive) != 0) {
		return QTextBrowser::loadResource(type, name);
	}

	if (mImageBlockWidget) {
		mImageBlockWidget->show();
	}

	return QPixmap(":/trolltech/styles/commonstyle/images/file-16.png");
}

void RSTextBrowser::setImageBlockWidget(RSImageBlockWidget *widget)
{
	if (mImageBlockWidget) {
		// disconnect
		disconnect(mImageBlockWidget, SIGNAL(destroyed()), this, SLOT(destroyImageBlockWidget()));
		disconnect(mImageBlockWidget, SIGNAL(showImages()), this, SLOT(showImages()));
	}

	mImageBlockWidget = widget;

	if (mImageBlockWidget) {
		// connect
		connect(mImageBlockWidget, SIGNAL(destroyed()), this, SLOT(destroyImageBlockWidget()));
		connect(mImageBlockWidget, SIGNAL(showImages()), this, SLOT(showImages()));
	}

	resetImagesStatus(false);
}

void RSTextBrowser::destroyImageBlockWidget()
{
	mImageBlockWidget = NULL;
}

void RSTextBrowser::showImages()
{
	if (mImageBlockWidget && sender() == mImageBlockWidget) {
		mImageBlockWidget->hide();
	}

	if (mShowImages) {
		return;
	}

	mShowImages = true;

	QString html = toHtml();
	clear();
	setHtml(html);
}

void RSTextBrowser::resetImagesStatus(bool load)
{
	if (mImageBlockWidget) {
		mImageBlockWidget->hide();
	}
	mShowImages = load;
}
