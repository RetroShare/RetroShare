/*******************************************************************************
 * retroshare-gui/src/gui/Posted/GxsForumMessageDialog.cpp                                *
 *                                                                             *
 * Copyright (C) 2020 by RetroShare Team       <retroshare.project@gmail.com>  *
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

#include "GxsForumMessageDialog.h"

#include <QMessageBox>

#include "gui/gxs/GxsIdDetails.h"
#include "gui/RetroShareLink.h"
#include "util/HandleRichText.h"
#include "gui/settings/rsharesettings.h"
#include "CreateGxsForumMsg.h"
#include "util/HandleRichText.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsforums.h>

/** Constructor */
GxsForumMessageDialog::GxsForumMessageDialog(QWidget *parent)
: QDialog(parent, Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint),
	ui(new Ui::GxsForumMessageDialog)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	connect(ui->shareButton, SIGNAL(clicked()), this, SLOT(copyMessageLink()));
	connect(ui->replyButton, SIGNAL(clicked()), this, SLOT(replyForumMessage()));
}

/** Destructor */
GxsForumMessageDialog::~GxsForumMessageDialog()
{
	delete ui;
}

void GxsForumMessageDialog::setText(const QString& text) 
{
	if(rsReputations->overallReputationLevel(mAuthorID) == RsReputationLevel::LOCALLY_NEGATIVE) {
		QString extraTxt = tr( "<p><font color=\"#ff0000\"><b>The author of this message (with ID %1) is banned.</b>").arg(QString::fromStdString(mAuthorID.toStdString())) ;
		extraTxt +=        tr( "<UL><li><b><font color=\"#ff0000\">Messages from this author are not forwarded. </font></b></li>") ;
		extraTxt +=        tr( "<li><b><font color=\"#ff0000\">Messages from this author are replaced by this text. </font></b></li></ul>") ;
		extraTxt +=        tr( "<p><b><font color=\"#ff0000\">You can force the visibility and forwarding of messages by setting a different opinion for that Id in People's tab.</font></b></p>") ;

		ui->postText->setHtml(extraTxt) ;
	} else {
		uint32_t flags = RSHTML_FORMATTEXT_EMBED_LINKS;
		if(Settings->getForumLoadEmoticons())
			flags |= RSHTML_FORMATTEXT_EMBED_SMILEYS ;
		flags |= RSHTML_OPTIMIZEHTML_MASK;

		QColor backgroundColor = ui->postText->palette().base().color();
		qreal desiredContrast = Settings->valueFromGroup("Forum",
			"MinimumContrast", 4.5).toDouble();
		int desiredMinimumFontSize = Settings->valueFromGroup("Forum",
			"MinimumFontSize", 10).toInt();

		QString postText = RsHtml().formatText(ui->postText->document(),
			text, flags, backgroundColor, desiredContrast, desiredMinimumFontSize);
		ui->postText->setHtml(postText);
	}
}

void GxsForumMessageDialog::setTitle(const QString& text) 
{
	setWindowTitle(text);
}

void GxsForumMessageDialog::setName(const RsGxsId& authorID) 
{
	mAuthorID = authorID;
	ui->nameLabel->setId(authorID);

	RsIdentityDetails idDetails ;
	rsIdentity->getIdDetails(authorID,idDetails);

	QPixmap pixmap ;

	if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
			pixmap = GxsIdDetails::makeDefaultIcon(authorID,GxsIdDetails::SMALL);
			
	ui->avatarWidget->setPixmap(pixmap);
}

void GxsForumMessageDialog::setTime(const QString& text) 
{
	ui->timeLabel->setText(text);
}

void GxsForumMessageDialog::setGroupId(const RsGxsGroupId &groupId) 
{
	mGroupId = groupId;
}

void GxsForumMessageDialog::setMessageId(const RsGxsMessageId& messageId) 
{
	mMessageId = messageId ;
}

void GxsForumMessageDialog::copyMessageLink()
{
	RetroShareLink link = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_FORUM, mGroupId, mMessageId, windowTitle());

	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
		QMessageBox::information(NULL,tr("information"),tr("The Retrohare link was copied to your clipboard.")) ;
	}
}

void GxsForumMessageDialog::replyForumMessage()
{
	if (!mAuthorID.isNull())
	{
		CreateGxsForumMsg *cfm = new CreateGxsForumMsg(mGroupId, mMessageId, RsGxsMessageId());

		RsHtml::makeQuotedText(ui->postText);

		cfm->insertPastedText(RsHtml::makeQuotedText(ui->postText)) ;
		cfm->show();

		/* window will destroy itself! */
	}
	else
	{
		QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to an Anonymous Author"));
	}
}
