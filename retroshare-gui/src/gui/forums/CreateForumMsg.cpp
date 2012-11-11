/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include "CreateForumMsg.h"

#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QDesktopWidget>
#include <QDropEvent>
#include <QPushButton>

#include <retroshare/rsforums.h>
#include <retroshare/rspeers.h>

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/common/Emoticons.h"

#include "util/misc.h"

#include <sys/stat.h>


/** Constructor */
CreateForumMsg::CreateForumMsg(const std::string &fId, const std::string &pId)
: QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint), mForumId(fId), mParentId(pId)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

    QString text = pId.empty() ? tr("Start New Thread") : tr("Post Forum Message");
    setWindowTitle(text);

    ui.headerFrame->setHeaderImage(QPixmap(":/images/konversation64.png"));
    ui.headerFrame->setHeaderText(text);

    Settings->loadWidgetInformation(this);

    connect( ui.forumMessage, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( forumMessageCostumPopupMenu( QPoint ) ) );

    connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

    // connect up the buttons.
    connect( ui.buttonBox, SIGNAL(accepted()), this, SLOT(createMsg()));
    connect( ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect( ui.emoticonButton, SIGNAL(clicked()), this, SLOT(smileyWidgetForums()));
    connect( ui.attachFileButton, SIGNAL(clicked() ), this , SLOT(addFile()));
    connect( ui.pastersButton, SIGNAL(clicked() ), this , SLOT(pasteLink()));

    setAcceptDrops(true);
    ui.hashBox->setDropWidget(this);
    ui.hashBox->setAutoHide(false);

    newMsg();
}

/** context menu searchTablewidget2 **/
void CreateForumMsg::forumMessageCostumPopupMenu(QPoint point)
{
    QMenu *contextMnu = ui.forumMessage->createStandardContextMenu(point);

    contextMnu->addSeparator();
    QAction *pasteLinkAct = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
    QAction *pasteLinkFullAct = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste full RetroShare Link"), this, SLOT(pasteLinkFull()));
    contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste own certificate link"), this, SLOT(pasteOwnCertificateLink()));

    if (RSLinkClipboard::empty()) {
        pasteLinkAct->setDisabled (true);
        pasteLinkFullAct->setDisabled (true);
    }

    contextMnu->exec(QCursor::pos());
    delete(contextMnu);
}

void  CreateForumMsg::newMsg()
{
    /* clear all */
    ForumInfo fi;
    if (rsForums->getForumInfo(mForumId, fi))
    {
        ForumMsgInfo msg;

        QString name = QString::fromStdWString(fi.forumName);
        QString subj;
        if ((mParentId != "") && (rsForums->getForumMessage(mForumId, mParentId, msg)))
        {
            QString title = QString::fromStdWString(msg.title);
            name += " " + tr("In Reply to") + ": ";
            name += title;

            QString text = title;

            if (text.startsWith("Re:", Qt::CaseInsensitive))
            {
                subj = title;
            }
            else
            {
                subj = "Re: " + title;
            }

        }

        ui.forumName->setText(misc::removeNewLine(name));
        ui.forumSubject->setText(misc::removeNewLine(subj));

        if (!ui.forumSubject->text().isEmpty())
        {
            ui.forumMessage->setFocus();
        }
        else
        {
            ui.forumSubject->setFocus();
        }

        if (fi.forumFlags & RS_DISTRIB_AUTHEN_REQ)
        {
            ui.signBox->setChecked(true);
            ui.signBox->setEnabled(false);
        }
        else
        {
            /* Uncheck sign box by default for anonymous forums */
            ui.signBox->setChecked(false);
            ui.signBox->setEnabled(true);
        }
    }

    ui.forumMessage->setText("");
}

void  CreateForumMsg::createMsg()
{
    QString name = misc::removeNewLine(ui.forumSubject->text());
    QString desc = ui.forumMessage->toHtml();

	 if(desc == QTextDocument(ui.forumMessage->toPlainText()).toHtml())
		 desc = ui.forumMessage->toPlainText() ;

    if(name.isEmpty())
    {	/* error message */
        QMessageBox::warning(this, tr("RetroShare"),tr("Please set a Forum Subject and Forum Message"),
                             QMessageBox::Ok, QMessageBox::Ok);

        return; //Don't add  a empty Subject!!
    }

    ForumMsgInfo msgInfo;

    msgInfo.forumId = mForumId;
    msgInfo.threadId = "";
    msgInfo.parentId = mParentId;
    msgInfo.msgId = "";

    msgInfo.title = name.toStdWString();
    msgInfo.msg = desc.toStdWString();
    msgInfo.msgflags = 0;

    if (ui.signBox->isChecked())
    {
        msgInfo.msgflags = RS_DISTRIB_AUTHEN_REQ;
    }

    if ((msgInfo.msg == L"") && (msgInfo.title == L""))
        return; /* do nothing */

    if (rsForums->ForumMessageSend(msgInfo) == true) {
        close();
    }
}

void CreateForumMsg::closeEvent (QCloseEvent * /*event*/)
{
    Settings->saveWidgetInformation(this);
}

void CreateForumMsg::smileyWidgetForums()
{
    Emoticons::showSmileyWidget(this, ui.emoticonButton, SLOT(addSmileys()), false);
}

void CreateForumMsg::addSmileys()
{
    ui.forumMessage->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

void CreateForumMsg::addFile()
{
    QStringList files;
    if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
        ui.hashBox->addAttachments(files,RS_FILE_REQ_ANONYMOUS_ROUTING);
    }
}

void CreateForumMsg::fileHashingFinished(QList<HashedFile> hashedFiles)
{
    std::cerr << "CreateForumMsg::fileHashingFinished() started." << std::endl;

    QString mesgString;

    QList<HashedFile>::iterator it;
    for (it = hashedFiles.begin(); it != hashedFiles.end(); ++it) {
        HashedFile& hashedFile = *it;
        RetroShareLink link;
        if (link.createFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash))) {
            mesgString += link.toHtmlSize() + "<br>";
        }
    }

#ifdef CHAT_DEBUG
    std::cerr << "CreateForumMsg::anchorClicked mesgString : " << mesgString.toStdString() << std::endl;
#endif

    if (!mesgString.isEmpty()) {
        ui.forumMessage->textCursor().insertHtml(mesgString);
    }

    ui.forumMessage->setFocus( Qt::OtherFocusReason );
}

void CreateForumMsg::pasteLink()
{
	ui.forumMessage->insertHtml(RSLinkClipboard::toHtml()) ;
}

void CreateForumMsg::pasteLinkFull()
{
	ui.forumMessage->insertHtml(RSLinkClipboard::toHtmlFull()) ;
}

void CreateForumMsg::pasteOwnCertificateLink()
{
	RetroShareLink link ;
	std::string ownId = rsPeers->getOwnId() ;
	if( link.createCertificate(ownId) )	{
		ui.forumMessage->insertHtml(link.toHtml() + " ");
	}
}
