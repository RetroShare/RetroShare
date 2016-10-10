#ifndef RSTEXTBROWSER_H
#define RSTEXTBROWSER_H

#include <QTextBrowser>
#include "util/RsSyntaxHighlighter.h"

//#define RSTEXTBROWSER_CHECKIMAGE_DEBUG 1

class RSImageBlockWidget;

class RSTextBrowser : public QTextBrowser
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorQuote READ textColorQuote WRITE setTextColorQuote)

public:
	explicit RSTextBrowser(QWidget *parent = 0);

	void setPlaceholderText(const QString &text);
	void setImageBlockWidget(RSImageBlockWidget *widget);
	void resetImagesStatus(bool load);
	QPixmap getBlockedImage();
	bool checkImage(QPoint pos, QString &imageStr);
	bool checkImage(QPoint pos) {QString imageStr; return checkImage(pos, imageStr); }

	void activateLinkClick(bool active);

	virtual QVariant loadResource(int type, const QUrl &name);

	QColor textColorQuote() const { return highlighter->textColorQuote();}
	bool getShowImages() const { return mShowImages; }

public slots:
	void showImages();
	void setTextColorQuote(QColor textColorQuote) { highlighter->setTextColorQuote(textColorQuote);}

private slots:
	void linkClicked(const QUrl &url);
	void destroyImageBlockWidget();

protected:
	void paintEvent(QPaintEvent *event);

private:
	QString mPlaceholderText;
	bool mShowImages;
	RSImageBlockWidget *mImageBlockWidget;
	bool mLinkClickActive;
	RsSyntaxHighlighter *highlighter;
#ifdef RSTEXTBROWSER_CHECKIMAGE_DEBUG
	QRect mCursorRectStart;
	QRect mCursorRectLeft;
	QRect mCursorRectRight;
	QRect mCursorRectEnd;
#endif
};

#endif // RSTEXTBROWSER_H
