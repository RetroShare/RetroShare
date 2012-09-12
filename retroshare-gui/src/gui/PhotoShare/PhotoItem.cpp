#include "PhotoItem.h"
#include "ui_PhotoItem.h"

PhotoItem::PhotoItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PhotoItem)
{
    ui->setupUi(this);
}

PhotoItem::~PhotoItem()
{
    delete ui;
}
