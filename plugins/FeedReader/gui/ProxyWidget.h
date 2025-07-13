#ifndef PROXYWIDGET_H
#define PROXYWIDGET_H

#include <QWidget>

namespace Ui {
class ProxyWidget;
}

class ProxyWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ProxyWidget(QWidget *parent = nullptr);
	~ProxyWidget();

	QString address();
	void setAddress(const QString &value);

	int port();
	void setPort(int value);

Q_SIGNALS:
	void changed();

private:
	Ui::ProxyWidget *ui;
};

#endif // PROXYWIDGET_H
