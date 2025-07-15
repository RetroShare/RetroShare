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

private Q_SLOTS:
	void addressChanged(const QString &value);

private:
	void connectUi(bool doConnect);
	void splitAddress(const QString &value, int &schemeIndex, QString &host);

private:
	Ui::ProxyWidget *ui;
	QMetaObject::Connection mAddressConnection;
	QMetaObject::Connection mSchemeConnection;
};

#endif // PROXYWIDGET_H
