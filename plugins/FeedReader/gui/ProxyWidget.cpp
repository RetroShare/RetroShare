#include "ProxyWidget.h"
#include "ui_ProxyWidget.h"

ProxyWidget::ProxyWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ProxyWidget)
{
	ui->setupUi(this);

	/* Connect signals */
	connectUi(true);
	connect(ui->portSpinBox, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, this, &ProxyWidget::changed);

	/* Initialize types */
	ui->schemeComboBox->addItem("", "");
	ui->schemeComboBox->addItem("HTTP", "http://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("HTTP Proxy."), Qt::ToolTipRole);
	ui->schemeComboBox->addItem("HTTPS", "https://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("HTTPS Proxy."), Qt::ToolTipRole);
	ui->schemeComboBox->addItem("SOCKS4", "socks4://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("SOCKS4 Proxy."), Qt::ToolTipRole);
	ui->schemeComboBox->addItem("SOCKS4a", "socks4a://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("SOCKS4a Proxy. Proxy resolves URL hostname."), Qt::ToolTipRole);
	ui->schemeComboBox->addItem("SOCKS5", "socks5://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("SOCKS5 Proxy."), Qt::ToolTipRole);
	ui->schemeComboBox->addItem("SOCKS5h", "socks5h://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("SOCKS5 Proxy. Proxy resolves URL hostname."), Qt::ToolTipRole);
}

ProxyWidget::~ProxyWidget()
{
	delete ui;
}

void ProxyWidget::connectUi(bool doConnect)
{
	if (doConnect) {
		if (!mAddressConnection) {
			mAddressConnection = connect(ui->addressLineEdit, &QLineEdit::textChanged, this, &ProxyWidget::addressChanged);
		}
		if (!mSchemeConnection) {
			mSchemeConnection = connect(ui->schemeComboBox, (void(QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &ProxyWidget::changed);
		}
	} else {
		if (mAddressConnection) {
			disconnect(mAddressConnection);
		}
		if (mSchemeConnection) {
			disconnect(mSchemeConnection);
		}
	}
}

QString ProxyWidget::address()
{
	QString host = ui->addressLineEdit->text();
	if (host.isEmpty()) {
		return "";
	}

	QString value;

	QString scheme = ui->schemeComboBox->currentData().toString();
	if (!scheme.isEmpty()) {
		value = scheme;
	}

	value += ui->addressLineEdit->text();

	return value;
}

void ProxyWidget::setAddress(const QString &value)
{
	int schemeIndex;
	QString host;

	splitAddress(value, schemeIndex, host);

	connectUi(false);
	ui->schemeComboBox->setCurrentIndex(schemeIndex);
	ui->addressLineEdit->setText(host);
	connectUi(true);
}

int ProxyWidget::port()
{
	return ui->portSpinBox->value();
}

void ProxyWidget::setPort(int value)
{
	ui->portSpinBox->setValue(value);
}

void ProxyWidget::addressChanged(const QString &value)
{
	int schemeIndex;
	QString host;

	splitAddress(value, schemeIndex, host);

	connectUi(false);
	ui->schemeComboBox->setCurrentIndex(schemeIndex);
	if (host != ui->addressLineEdit->text()) {
		ui->addressLineEdit->setText(host);
	}
	connectUi(true);

	emit changed();
}

void ProxyWidget::splitAddress(const QString &value, int &schemeIndex, QString &host)
{
	if (value.isEmpty()) {
		schemeIndex = ui->schemeComboBox->currentIndex();
		host = value;
		return;
	}

	QString scheme;
	int index = value.indexOf("://");
	if (index >= 0) {
		scheme = value.left(index + 3);
		host = value.mid(index + 3);
	} else {
		if (ui->schemeComboBox->currentIndex() == 0) {
			// Default to HTTP
			scheme = "http://";
		} else {
			scheme = ui->schemeComboBox->currentData().toString();
		}
		host = value;
	}

	schemeIndex = ui->schemeComboBox->findData(scheme);
	if (schemeIndex < 0) {
		/* Unknown scheme */
		schemeIndex = 0;
		host = value;
	}
}
