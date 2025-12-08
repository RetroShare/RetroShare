#include "NotifyWidget.h"
#include "ui_NotifyWidget.h"
#include "RSTreeWidgetItem.h"

#define ROLE_ID              Qt::UserRole
#define ROLE_NAME            Qt::UserRole + 1
#define ROLE_DESCRIPTION     Qt::UserRole + 2
#define ROLE_SUBSCRIBE_FLAGS Qt::UserRole + 3
#define ROLE_COLOR           Qt::UserRole + 4
#define ROLE_REQUEST_ID      Qt::UserRole + 5
#define ROLE_SORT            Qt::UserRole + 6

NotifyWidget::NotifyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotifyWidget)
{
    ui->setupUi(this);
}

NotifyWidget::~NotifyWidget()
{
    delete ui;
}

void NotifyWidget::setUnreadCount(QTreeWidgetItem *item, int unreadCount)
{
    if (item == NULL) {
        return;
    }

    QFont itFont = item->font(GTW_COLUMN_NAME);

    if (unreadCount) {
        item->setText(GTW_COLUMN_UNREAD, QString::number(unreadCount));
        itFont.setBold(true);
    } else {
        item->setText(GTW_COLUMN_UNREAD, "");
        itFont.setBold(false);
    }
    item->setData(GTW_COLUMN_UNREAD, ROLE_SORT, unreadCount);

    item->setFont(GTW_COLUMN_NAME, itFont);
}

QTreeWidgetItem *NotifyWidget::getItemFromId(const QString &id)
{
    if (id.isEmpty()) {
        return NULL;
    }

    /* Search exisiting item */
    QTreeWidgetItemIterator itemIterator(ui->treeWidget);
    QTreeWidgetItem *item;
    while ((item = *itemIterator) != NULL) {
        ++itemIterator;

        if (item->parent() == NULL) {
            continue;
        }
        if (item->data(GTW_COLUMN_DATA, ROLE_ID).toString() == id) {
            return item;
        }
    }
    return NULL ;
}
