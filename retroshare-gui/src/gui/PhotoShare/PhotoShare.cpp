#include "PhotoShare.h"
#include "ui_PhotoShare.h"

PhotoShare::PhotoShare(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PhotoShare)
{
    ui->setupUi(this);
}

PhotoShare::~PhotoShare()
{
    delete ui;
}
