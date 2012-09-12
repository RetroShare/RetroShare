#include "AlbumDialog.h"
#include "ui_AlbumDialog.h"

AlbumDialog::AlbumDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AlbumDialog)
{
    ui->setupUi(this);
}

AlbumDialog::~AlbumDialog()
{
    delete ui;
}
