#include "HeaderFrame.h"
#include "ui_HeaderFrame.h"

HeaderFrame::HeaderFrame(QWidget *parent) :
	QFrame(parent),
	ui(new Ui::HeaderFrame)
{
	ui->setupUi(this);
}

HeaderFrame::~HeaderFrame()
{
	delete ui;
}

void HeaderFrame::setHeaderText(const QString &headerText)
{
	ui->headerLabel->setText(headerText);
}

void HeaderFrame::setHeaderImage(const QPixmap &headerImage)
{
	ui->headerImage->setPixmap(headerImage);
}
