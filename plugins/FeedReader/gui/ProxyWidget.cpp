#include "ProxyWidget.h"
#include "ui_ProxyWidget.h"

ProxyWidget::ProxyWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ProxyWidget)
{
	ui->setupUi(this);

	/* Connect signals */
	connect(ui->addressLineEdit, &QLineEdit::textChanged, this, &ProxyWidget::changed);
	connect(ui->portSpinBox, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, this, &ProxyWidget::changed);
}

ProxyWidget::~ProxyWidget()
{
	delete ui;
}

QString ProxyWidget::address()
{
	return ui->addressLineEdit->text();
}

void ProxyWidget::setAddress(const QString &value)
{
	ui->addressLineEdit->setText(value);
}

int ProxyWidget::port()
{
	return ui->portSpinBox->value();
}

void ProxyWidget::setPort(int value)
{
	ui->portSpinBox->setValue(value);
}
