#ifndef LINKTEXTBROWSER_H
#define LINKTEXTBROWSER_H

#include <QTextBrowser>

class LinkTextBrowser : public QTextBrowser
{
	Q_OBJECT

public:
	explicit LinkTextBrowser(QWidget *parent = 0);

	void setPlaceholderText(const QString &text);

private slots:
	void linkClicked(const QUrl &url);

protected:
	void paintEvent(QPaintEvent *event);

	QString placeholderText;
};

#endif // LINKTEXTBROWSER_H
