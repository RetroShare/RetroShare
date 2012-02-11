#ifndef LINKTEXTBROWSER_H
#define LINKTEXTBROWSER_H

#include <QTextBrowser>

class LinkTextBrowser : public QTextBrowser
{
	Q_OBJECT

public:
	explicit LinkTextBrowser(QWidget *parent = 0);

private slots:
	void linkClicked(const QUrl &url);
};

#endif // LINKTEXTBROWSER_H
