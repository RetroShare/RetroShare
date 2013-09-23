#include "RSImageBlockWidget.h"
#include "ui_RSImageBlockWidget.h"

RSImageBlockWidget::RSImageBlockWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::RSImageBlockWidget)
{
	ui->setupUi(this);

	connect(ui->loadImagesButton, SIGNAL(clicked()), this, SIGNAL(showImages()));
}

RSImageBlockWidget::~RSImageBlockWidget()
{
	delete ui;
}
