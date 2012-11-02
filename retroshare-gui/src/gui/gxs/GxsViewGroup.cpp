#include "GxsViewGroup.h"
#include "ui_GxsViewGroup.h"

GxsViewGroup::GxsViewGroup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GxsViewGroup)
{
    ui->setupUi(this);
}

GxsViewGroup::~GxsViewGroup()
{
    delete ui;
}

void GxsViewGroup::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
