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
	ui->schemeComboBox->addItem(tr("SOCKS4 (local DNS)"), "socks4://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("SOCKS4 Proxy. Hostnames are resolved locally and leak to your DNS resolver."), Qt::ToolTipRole);
	ui->schemeComboBox->addItem(tr("SOCKS4a (remote DNS)"), "socks4a://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("SOCKS4a Proxy. Hostnames are resolved by the proxy."), Qt::ToolTipRole);
	ui->schemeComboBox->addItem(tr("SOCKS5 (local DNS)"), "socks5://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("SOCKS5 Proxy. Hostnames are resolved locally and leak to your DNS resolver."), Qt::ToolTipRole);
	ui->schemeComboBox->addItem(tr("SOCKS5h (remote DNS, use with Tor)"), "socks5h://");
	ui->schemeComboBox->setItemData(ui->schemeComboBox->count() - 1, tr("SOCKS5 Proxy. Hostnames are resolved by the proxy. Recommended with Tor."), Qt::ToolTipRole);

	/* The warning only reflects the selected scheme, so it does not need to be
	 * disconnected while the widget updates itself (unlike the "changed" signal). */
	connect(ui->schemeComboBox, (void(QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &ProxyWidget::updateWarning);
	updateWarning();
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

void ProxyWidget::updateWarning()
{
	QString scheme = ui->schemeComboBox->currentData().toString();

	if (scheme == "socks4://" || scheme == "socks5://") {
		ui->warningLabel->setText(tr("Hostnames of the feeds will be resolved locally and leak to your DNS resolver. With Tor, use SOCKS5h (or SOCKS4a) instead."));
		ui->warningLabel->show();
	} else {
		ui->warningLabel->clear();
		ui->warningLabel->hide();
	}
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
