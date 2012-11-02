#include "GxsCreateGroupDialog.h"
#include "ui_GxsCreateGroupDialog.h"

GxsCreateGroupDialog::GxsCreateGroupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GxsCreateGroupDialog)
{
    ui->setupUi(this);
}

GxsCreateGroupDialog::~GxsCreateGroupDialog()
{
    delete ui;
}

void GxsCreateGroupDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
