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

#include "rshare.h"
#include "rsharesettings.h"
#include "retroshare/rsmsgs.h"

#include "MessagePage.h"
#include "gui/common/TagDefs.h"
#include <algorithm>
#include "NewTag.h"

MessagePage::MessagePage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
    ui.setupUi(this);

    m_pTags = new MsgTagType;

    connect (ui.addpushButton, SIGNAL(clicked(bool)), this, SLOT (addTag()));
    connect (ui.editpushButton, SIGNAL(clicked(bool)), this, SLOT (editTag()));
    connect (ui.deletepushButton, SIGNAL(clicked(bool)), this, SLOT (deleteTag()));
    connect (ui.defaultTagButton, SIGNAL(clicked(bool)), this, SLOT (defaultTag()));
    connect (ui.encryptedMsgs_CB, SIGNAL(toggled(bool)), this, SLOT (toggleEnableEncryptedDistantMsgs(bool)));

    connect (ui.tags_listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(currentRowChangedTag(int)));

    ui.editpushButton->setEnabled(false);
    ui.deletepushButton->setEnabled(false);

    ui.openComboBox->addItem(tr("A new tab"), RshareSettings::MSG_OPEN_TAB);
    ui.openComboBox->addItem(tr("A new window"), RshareSettings::MSG_OPEN_WINDOW);

	 //ui.encryptedMsgs_CB->setEnabled(false) ;
}

MessagePage::~MessagePage()
{
    delete(m_pTags);
}

void MessagePage::toggleEnableEncryptedDistantMsgs(bool b)
{
	rsMsgs->enableDistantMessaging(b) ;
}

/** Saves the changes on this page */
bool
MessagePage::save(QString &/*errmsg*/)
{
    Settings->setMsgSetToReadOnActivate(ui.setMsgToReadOnActivate->isChecked());
    Settings->setMsgLoadEmbeddedImages(ui.loadEmbeddedImages->isChecked());
    Settings->setMsgOpen((RshareSettings::enumMsgOpen) ui.openComboBox->itemData(ui.openComboBox->currentIndex()).toInt());

    std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
    for (Tag = m_pTags->types.begin(); Tag != m_pTags->types.end(); Tag++) {
        // check for changed tags
        std::list<uint32_t>::iterator changedTagId;
        for (changedTagId = m_changedTagIds.begin(); changedTagId != m_changedTagIds.end(); changedTagId++) {
            if (*changedTagId == Tag->first) {
                if (Tag->second.first.empty()) {
                    // delete tag
                    rsMsgs->removeMessageTagType(Tag->first);
                    continue;
                }

                rsMsgs->setMessageTagType(Tag->first, Tag->second.first, Tag->second.second);
                break;
            }
        }
    }

    return true;
}

/** Loads the settings for this page */
void
MessagePage::load()
{
    ui.setMsgToReadOnActivate->setChecked(Settings->getMsgSetToReadOnActivate());
    ui.loadEmbeddedImages->setChecked(Settings->getMsgLoadEmbeddedImages());
    ui.openComboBox->setCurrentIndex(ui.openComboBox->findData(Settings->getMsgOpen()));

	 ui.encryptedMsgs_CB->setChecked(rsMsgs->distantMessagingEnabled()) ;
    // fill items
    rsMsgs->getMessageTagTypes(*m_pTags);
    fillTags();
}

// fill tags
void MessagePage::fillTags()
{
    ui.tags_listWidget->clear();

    std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
    for (Tag = m_pTags->types.begin(); Tag != m_pTags->types.end(); Tag++) {
        QString text = TagDefs::name(Tag->first, Tag->second.first);

        QListWidgetItem *pItemWidget = new QListWidgetItem(text, ui.tags_listWidget);
        pItemWidget->setTextColor(QColor(Tag->second.second));
        pItemWidget->setData(Qt::UserRole, Tag->first);
    }
}

void MessagePage::addTag()
{
    NewTag TagDlg(*m_pTags);
    if (TagDlg.exec() == QDialog::Accepted && TagDlg.m_nId) {
        std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
        Tag = m_pTags->types.find(TagDlg.m_nId);
        if (Tag != m_pTags->types.end()) {
            QString text = TagDefs::name(Tag->first, Tag->second.first);

            QListWidgetItem *pItemWidget = new QListWidgetItem(text, ui.tags_listWidget);
            pItemWidget->setTextColor(QColor(Tag->second.second));
            pItemWidget->setData(Qt::UserRole, TagDlg.m_nId);

            m_changedTagIds.push_back(TagDlg.m_nId);
        }
    }
}

void MessagePage::editTag()
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->currentItem();
    if (pItemWidget == NULL) {
        return;
    }

    uint32_t nId = pItemWidget->data(Qt::UserRole).toInt();
    if (nId == 0) {
        return;
    }

    NewTag TagDlg(*m_pTags, nId);
    TagDlg.setWindowTitle(tr("Edit Tag"));
    if (TagDlg.exec() == QDialog::Accepted && TagDlg.m_nId) {
        std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
        Tag = m_pTags->types.find(TagDlg.m_nId);
        if (Tag != m_pTags->types.end()) {
            if (Tag->first >= RS_MSGTAGTYPE_USER) {
                pItemWidget->setText(QString::fromStdString(Tag->second.first));
            }
            pItemWidget->setTextColor(QColor(Tag->second.second));

            if (std::find(m_changedTagIds.begin(), m_changedTagIds.end(), TagDlg.m_nId) == m_changedTagIds.end()) {
                m_changedTagIds.push_back(TagDlg.m_nId);
            }
        }
    }
}

void MessagePage::deleteTag()
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->currentItem();
    if (pItemWidget == NULL) {
        return;
    }

    uint32_t nId = pItemWidget->data(Qt::UserRole).toInt();
    if (nId == 0) {
        return;
    }

    if (nId < RS_MSGTAGTYPE_USER) {
        // can't delete standard tag item
        return;
    }

    std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
    Tag = m_pTags->types.find(nId);
    if (Tag != m_pTags->types.end()) {
        // erase the text for later delete
        Tag->second.first.erase();
    }

    ui.tags_listWidget->removeItemWidget(pItemWidget);
    delete (pItemWidget);

    if (std::find(m_changedTagIds.begin(), m_changedTagIds.end(), nId) == m_changedTagIds.end()) {
        m_changedTagIds.push_back(nId);
    }
}

void MessagePage::defaultTag()
{
    rsMsgs->resetMessageStandardTagTypes(*m_pTags);

    // add all standard items to changed list
    std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
    for (Tag = m_pTags->types.begin(); Tag != m_pTags->types.end(); Tag++) {
        if (Tag->first < RS_MSGTAGTYPE_USER) {
            if (std::find(m_changedTagIds.begin(), m_changedTagIds.end(), Tag->first) == m_changedTagIds.end()) {
                m_changedTagIds.push_back(Tag->first);
            }
        }
    }

    fillTags();
}

void MessagePage::currentRowChangedTag(int row)
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->item(row);

    bool bEditEnable = false;
    bool bDeleteEnable = false;

    if (pItemWidget) {
        bEditEnable = true;

        uint32_t nId = pItemWidget->data(Qt::UserRole).toInt();

        if (nId >= RS_MSGTAGTYPE_USER) {
            bDeleteEnable = true;
        }
    }

    ui.editpushButton->setEnabled(bEditEnable);
    ui.deletepushButton->setEnabled(bDeleteEnable);
}
