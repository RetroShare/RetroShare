/*******************************************************************************
 * gui/settings/MessagePage.cpp                                                *
 *                                                                             *
 * Copyright (C) 2006 Crypton <retroshare.project@gmail.com>                   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QFontDatabase>

#include "rshare.h"
#include "rsharesettings.h"
#include "retroshare/rsmsgs.h"

#include "MessagePage.h"
#include "util/misc.h"
#include "gui/common/TagDefs.h"
#include <algorithm>
#include "NewTag.h"
#include "util/qtthreadsutils.h"
#include "gui/notifyqt.h"

MessagePage::MessagePage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    ui.setupUi(this);

    m_pTags = new MsgTagType;

    connect (ui.addpushButton, SIGNAL(clicked(bool)), this, SLOT (addTag()));
    connect (ui.editpushButton, SIGNAL(clicked(bool)), this, SLOT (editTag()));
    connect (ui.deletepushButton, SIGNAL(clicked(bool)), this, SLOT (deleteTag()));
    connect (ui.defaultTagButton, SIGNAL(clicked(bool)), this, SLOT (defaultTag()));

    connect (ui.tags_listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(currentRowChangedTag(int)));

    ui.editpushButton->setEnabled(false);
    ui.deletepushButton->setEnabled(false);

    ui.openComboBox->addItem(tr("A new tab"), RshareSettings::MSG_OPEN_TAB);
    ui.openComboBox->addItem(tr("A new window"), RshareSettings::MSG_OPEN_WINDOW);

    // Font size
    QFontDatabase db;
    foreach(int size, db.standardSizes()) {
        ui.minimumFontSize->addItem(QString::number(size), size);
    }

    connect(ui.comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(distantMsgsComboBoxChanged(int)));

	connect(ui.setMsgToReadOnActivate,SIGNAL(toggled(bool)),          this,SLOT(updateMsgToReadOnActivate()));
	connect(ui.loadEmbeddedImages,    SIGNAL(toggled(bool)),          this,SLOT(updateLoadEmbededImages()  ));
	connect(ui.openComboBox,          SIGNAL(currentIndexChanged(int)),this,SLOT(updateMsgOpen()            ));
	connect(ui.emoticonscheckBox,     SIGNAL(toggled(bool)),          this,SLOT(updateLoadEmoticons()  ));
	connect(ui.minimumFontSize,       SIGNAL(activated(QString)),     this, SLOT(updateFontSize())) ;

    mTagEventHandlerId = 0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { RsQThreadUtils::postToObject( [this,event]() { handleEvent_main_thread(event); }); }, mTagEventHandlerId, RsEventType::MAIL_TAG );
}

MessagePage::~MessagePage()
{
    rsEvents->unregisterEventsHandler(mTagEventHandlerId);
     delete(m_pTags);
}

void MessagePage::distantMsgsComboBoxChanged(int i)
{
	switch(i)
	{
		case 0:  rsMail->setDistantMessagingPermissionFlags(RS_DISTANT_MESSAGING_CONTACT_PERMISSION_FLAG_FILTER_NONE) ; 
				  break ;
				  
		case 1:  rsMail->setDistantMessagingPermissionFlags(RS_DISTANT_MESSAGING_CONTACT_PERMISSION_FLAG_FILTER_NON_CONTACTS) ; 
				  break ;

		case 2: rsMail->setDistantMessagingPermissionFlags(RS_DISTANT_MESSAGING_CONTACT_PERMISSION_FLAG_FILTER_EVERYBODY) ;
				  break ;
				    
				  
		default: ;
	}

}

void MessagePage::updateMsgToReadOnActivate() { Settings->setMsgSetToReadOnActivate(ui.setMsgToReadOnActivate->isChecked()); }
void MessagePage::updateLoadEmbededImages()   { Settings->setMsgLoadEmbeddedImages(ui.loadEmbeddedImages->isChecked()); }
void MessagePage::updateMsgOpen()             { Settings->setMsgOpen( static_cast<RshareSettings::enumMsgOpen>(ui.openComboBox->itemData(ui.openComboBox->currentIndex()).toInt()) ); }
void MessagePage::updateDistantMsgs()         { Settings->setValue("DistantMessages", ui.comboBox->currentIndex()); }
void MessagePage::updateLoadEmoticons()       { Settings->setValueToGroup("Messages", "Emoticons", ui.emoticonscheckBox->isChecked()); }

void MessagePage::updateMsgTags()
{
    std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
    for (Tag = m_pTags->types.begin(); Tag != m_pTags->types.end(); ++Tag) {
        // check for changed tags
        std::list<uint32_t>::iterator changedTagId;
        for (changedTagId = m_changedTagIds.begin(); changedTagId != m_changedTagIds.end(); ++changedTagId) {
            if (*changedTagId == Tag->first) {
                if (Tag->second.first.empty()) {
                    // delete tag
                    rsMail->removeMessageTagType(Tag->first);
                    continue;
                }

                rsMail->setMessageTagType(Tag->first, Tag->second.first, Tag->second.second);
                break;
            }
        }
    }
}

/** Loads the settings for this page */
void
MessagePage::load()
{
    whileBlocking(ui.setMsgToReadOnActivate)->setChecked(Settings->getMsgSetToReadOnActivate());
    whileBlocking(ui.loadEmbeddedImages)->setChecked(Settings->getMsgLoadEmbeddedImages());
    whileBlocking(ui.openComboBox)->setCurrentIndex(ui.openComboBox->findData(Settings->getMsgOpen()));
    whileBlocking(ui.emoticonscheckBox)->setChecked(Settings->value("Emoticons", true).toBool());
	whileBlocking(ui.minimumFontSize)->setCurrentIndex(ui.minimumFontSize->findData(Settings->getMessageFontSize()));

	  // state of filter combobox
    
    uint32_t flags = rsMail->getDistantMessagingPermissionFlags() ;
    
    if(flags & RS_DISTANT_MESSAGING_CONTACT_PERMISSION_FLAG_FILTER_EVERYBODY)
	    whileBlocking(ui.comboBox)->setCurrentIndex(2);
    else if(flags & RS_DISTANT_MESSAGING_CONTACT_PERMISSION_FLAG_FILTER_NON_CONTACTS)
	    whileBlocking(ui.comboBox)->setCurrentIndex(1);
    else
	    whileBlocking(ui.comboBox)->setCurrentIndex(0);
	  
    // fill items
    rsMail->getMessageTagTypes(*m_pTags);
    fillTags();
}

void MessagePage::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    if (event->mType != RsEventType::MAIL_TAG) {
        return;
    }

    const RsMailTagEvent *fe = dynamic_cast<const RsMailTagEvent*>(event.get());
    if (!fe) {
        return;
    }

    switch (fe->mMailTagEventCode) {
    case RsMailTagEventCode::TAG_ADDED:
    case RsMailTagEventCode::TAG_CHANGED:
    case RsMailTagEventCode::TAG_REMOVED:
       rsMail->getMessageTagTypes(*m_pTags);
       fillTags();
       break;
    }
}

// fill tags
void MessagePage::fillTags()
{
    ui.tags_listWidget->clear();

    std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
    for (Tag = m_pTags->types.begin(); Tag != m_pTags->types.end(); ++Tag) {
        QString text = TagDefs::name(Tag->first, Tag->second.first);

        QListWidgetItem *pItemWidget = new QListWidgetItem(text, ui.tags_listWidget);
        pItemWidget->setData(Qt::ForegroundRole, QColor(Tag->second.second));
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
            pItemWidget->setData(Qt::ForegroundRole, QColor(Tag->second.second));
            pItemWidget->setData(Qt::UserRole, TagDlg.m_nId);

            m_changedTagIds.push_back(TagDlg.m_nId);
        }
    }

    updateMsgTags();
}

void MessagePage::editTag()
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->currentItem();
    if (!pItemWidget) {
        return;
    }

    uint32_t nId = pItemWidget->data(Qt::UserRole).toUInt();
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
            pItemWidget->setData(Qt::ForegroundRole, QColor(Tag->second.second));

            if (std::find(m_changedTagIds.begin(), m_changedTagIds.end(), TagDlg.m_nId) == m_changedTagIds.end()) {
                m_changedTagIds.push_back(TagDlg.m_nId);
            }
        }
    }
    updateMsgTags();
}

void MessagePage::deleteTag()
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->currentItem();
    if (!pItemWidget) {
        return;
    }

    uint32_t nId = pItemWidget->data(Qt::UserRole).toUInt();
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
    updateMsgTags();
}

void MessagePage::defaultTag()
{
    rsMail->resetMessageStandardTagTypes(*m_pTags);

    // add all standard items to changed list
    std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
    for (Tag = m_pTags->types.begin(); Tag != m_pTags->types.end(); ++Tag) {
        if (Tag->first < RS_MSGTAGTYPE_USER) {
            if (std::find(m_changedTagIds.begin(), m_changedTagIds.end(), Tag->first) == m_changedTagIds.end()) {
                m_changedTagIds.push_back(Tag->first);
            }
        }
    }

    updateMsgTags();
    fillTags();
}

void MessagePage::currentRowChangedTag(int row)
{
    QListWidgetItem *pItemWidget = ui.tags_listWidget->item(row);

    bool bEditEnable = false;
    bool bDeleteEnable = false;

    if (pItemWidget) {
        bEditEnable = true;

        uint32_t nId = pItemWidget->data(Qt::UserRole).toUInt();

        if (nId >= RS_MSGTAGTYPE_USER) {
            bDeleteEnable = true;
        }
    }

    ui.editpushButton->setEnabled(bEditEnable);
    ui.deletepushButton->setEnabled(bDeleteEnable);
}

void MessagePage::updateFontSize()
{
	Settings->setMessageFontSize(ui.minimumFontSize->currentData().toInt());

	NotifyQt::getInstance()->notifySettingsChanged();
}
