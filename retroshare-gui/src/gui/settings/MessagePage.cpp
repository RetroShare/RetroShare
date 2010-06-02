/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "MessagePage.h"
#include "rshare.h"
#include "rsharesettings.h"

#include "../MainWindow.h"
#include "../MessagesDialog.h"

MessagePage::MessagePage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);

    connect (ui.addpushButton, SIGNAL(clicked(bool)), this, SLOT (addTag()));
    connect (ui.editpushButton, SIGNAL(clicked(bool)), this, SLOT (editTag()));
    connect (ui.deletepushButton, SIGNAL(clicked(bool)), this, SLOT (deleteTag()));
    connect (ui.defaultTagButton, SIGNAL(clicked(bool)), this, SLOT (defaultTag()));

    connect (ui.tags_listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(currentRowChangedTag(int)));

    ui.editpushButton->setEnabled(false);
    ui.deletepushButton->setEnabled(false);
}

MessagePage::~MessagePage()
{
}

void
MessagePage::closeEvent (QCloseEvent * event)
{
    QWidget::closeEvent(event);
}

/** Saves the changes on this page */
bool
MessagePage::save(QString &errmsg)
{
    Settings->setMsgSetToReadOnActivate(ui.setMsgToReadOnActivate->isChecked());

#ifdef STATIC_MSGID
    MessagesDialog *pPage = (MessagesDialog*) MainWindow::getPage (MainWindow::Messages);
    if (pPage) {
        pPage->setTagItems (m_TagItems);
    }
#endif

    return true;
}

/** Loads the settings for this page */
void
MessagePage::load()
{
    ui.setMsgToReadOnActivate->setChecked(Settings->getMsgSetToReadOnActivate());

#ifdef STATIC_MSGID
    MessagesDialog *pPage = (MessagesDialog*) MainWindow::getPage (MainWindow::Messages);
    if (pPage) {
        pPage->getTagItems (m_TagItems);

        // fill items
        fillTagItems();
    } else {
        // MessagesDialog not available
        ui.tags_listWidget->setEnabled(false);
        ui.addpushButton->setEnabled(false);
        ui.editpushButton->setEnabled(false);
        ui.deletepushButton->setEnabled(false);
        ui.defaultTagButton->setEnabled(false);
    }
#else
    ui.tags_listWidget->setEnabled(false);
    ui.addpushButton->setEnabled(false);
    ui.editpushButton->setEnabled(false);
    ui.deletepushButton->setEnabled(false);
    ui.defaultTagButton->setEnabled(false);
#endif
}

// fill items
void MessagePage::fillTagItems()
{
    ui.tags_listWidget->clear();

    std::map<int, TagItem>::iterator Item;
    for (Item = m_TagItems.begin(); Item != m_TagItems.end(); Item++) {
        if (Item->second._delete) {
            continue;
        }

        QListWidgetItem *pItemWidget = new QListWidgetItem(Item->second.text, ui.tags_listWidget);
        pItemWidget->setTextColor(QColor(Item->second.color));
        pItemWidget->setData(Qt::UserRole, Item->first);
    }
}

void MessagePage::addTag()
{
    NewTag Tag(m_TagItems);
    if (Tag.exec() == QDialog::Accepted && Tag.m_nId) {
        TagItem &Item = m_TagItems [Tag.m_nId];
        QListWidgetItem *pItemWidget = new QListWidgetItem(Item.text, ui.tags_listWidget);
        pItemWidget->setTextColor(QColor(Item.color));
        pItemWidget->setData(Qt::UserRole, Tag.m_nId);
    }
}

void MessagePage::editTag()
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->currentItem();
    if (pItemWidget == NULL) {
        return;
    }

    int nId = pItemWidget->data(Qt::UserRole).toInt();
    if (nId == 0) {
        return;
    }

    NewTag Tag(m_TagItems, nId);
    Tag.setWindowTitle(tr("Edit Tag"));
    if (Tag.exec() == QDialog::Accepted && Tag.m_nId) {
        TagItem &Item = m_TagItems [Tag.m_nId];
        pItemWidget->setText(Item.text);
        pItemWidget->setTextColor(QColor(Item.color));
    }
}

void MessagePage::deleteTag()
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->currentItem();
    if (pItemWidget == NULL) {
        return;
    }

    int nId = pItemWidget->data(Qt::UserRole).toInt();
    if (nId == 0) {
        return;
    }

    if (nId < 0) {
        return;
    }

    TagItem &Item = m_TagItems [nId];
    Item._delete = true;

    ui.tags_listWidget->removeItemWidget(pItemWidget);
    delete (pItemWidget);
}

void MessagePage::defaultTag()
{
#ifdef STATIC_MSGID
    MessagesDialog::initStandardTagItems(m_TagItems);
#endif
    fillTagItems();
}

void MessagePage::currentRowChangedTag(int row)
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->item(row);

    bool bEditEnable = false;
    bool bDeleteEnable = false;

    if (pItemWidget) {
        bEditEnable = true;

        int nId = pItemWidget->data(Qt::UserRole).toInt();

        if (nId > 0) {
            bDeleteEnable = true;
        }
    }

    ui.editpushButton->setEnabled(bEditEnable);
    ui.deletepushButton->setEnabled(bDeleteEnable);
}
