#ifndef RSTEXTBROWSER_H
#define RSTEXTBROWSER_H

#include <QTextBrowser>

class RSImageBlockWidget;

class RSTextBrowser : public QTextBrowser
{
	Q_OBJECT

public:
	explicit RSTextBrowser(QWidget *parent = 0);

	void setPlaceholderText(const QString &text);
	void setImageBlockWidget(RSImageBlockWidget *widget);
	void resetImagesStatus(bool load);

	virtual QVariant loadResource(int type, const QUrl &name);

public slots:
	void showImages();

private slots:
	void linkClicked(const QUrl &url);
	void destroyImageBlockWidget();

protected:
	void paintEvent(QPaintEvent *event);

	QString mPlaceholderText;
	bool mShowImages;
	RSImageBlockWidget *mImageBlockWidget;
};

#endif // RSTEXTBROWSER_H
