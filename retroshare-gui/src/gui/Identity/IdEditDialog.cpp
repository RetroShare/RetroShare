/*******************************************************************************
 * retroshare-gui/src/gui/Identity/IdEditDialog.cpp                            *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#include <QBuffer>
#include <QMessageBox>

#include "IdEditDialog.h"
#include "ui_IdEditDialog.h"
#include "gui/common/UIStateHelper.h"
#include "gui/common/AvatarDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/qtthreadsutils.h"
#include "util/misc.h"
#include "gui/notifyqt.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>

#include <iostream>

#define IDEDITDIALOG_LOADID   1
#define IDEDITDIALOG_CREATEID 2

/** Constructor */
IdEditDialog::IdEditDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new(Ui::IdEditDialog))
{
	mIsNew = true;

	ui->setupUi(this);

	ui->headerFrame->setHeaderImage(QPixmap(":/icons/png/person.png"));
	ui->headerFrame->setHeaderText(tr("Create New Identity"));

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui->lineEdit_Nickname);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui->lineEdit_KeyId);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui->lineEdit_GpgHash);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui->lineEdit_GpgId);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui->lineEdit_GpgName);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui->radioButton_GpgId);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui->radioButton_Pseudo);

	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui->lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui->lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui->lineEdit_KeyId);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui->lineEdit_GpgHash);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui->lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui->lineEdit_GpgName);

	/* Initialize recogn tags */
	loadRecognTags();

	/* Connect signals */
	connect(ui->radioButton_GpgId, SIGNAL(toggled(bool)), this, SLOT(idTypeToggled(bool)));
	connect(ui->radioButton_Pseudo, SIGNAL(toggled(bool)), this, SLOT(idTypeToggled(bool)));
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(submit()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	connect(ui->plainTextEdit_Tag, SIGNAL(textChanged()), this, SLOT(checkNewTag()));
	connect(ui->pushButton_Tag, SIGNAL(clicked(bool)), this, SLOT(addRecognTag()));
	connect(ui->toolButton_Tag1, SIGNAL(clicked(bool)), this, SLOT(rmTag1()));
	connect(ui->toolButton_Tag2, SIGNAL(clicked(bool)), this, SLOT(rmTag2()));
	connect(ui->toolButton_Tag3, SIGNAL(clicked(bool)), this, SLOT(rmTag3()));
	connect(ui->toolButton_Tag4, SIGNAL(clicked(bool)), this, SLOT(rmTag4()));
	connect(ui->toolButton_Tag5, SIGNAL(clicked(bool)), this, SLOT(rmTag5()));
	connect(ui->avatarButton, SIGNAL(clicked(bool)), this, SLOT(changeAvatar()));

	/* Initialize ui */
	ui->lineEdit_Nickname->setMaxLength(RSID_MAXIMUM_NICKNAME_SIZE);

	ui->pushButton_Tag->setEnabled(false);
	ui->pushButton_Tag->hide(); // unfinished
	ui->plainTextEdit_Tag->hide();
	ui->label_TagCheck->hide();
}

IdEditDialog::~IdEditDialog() {}

void IdEditDialog::changeAvatar()
{
	AvatarDialog dialog(this);

	dialog.setAvatar(mAvatar);
	if (dialog.exec() == QDialog::Accepted) {
		QPixmap newAvatar;
		dialog.getAvatar(newAvatar);

		setAvatar(newAvatar);
	}
}

void IdEditDialog::setupNewId(bool pseudo,bool enable_anon)
{
	setWindowTitle(tr("New identity"));

    	if(pseudo && !enable_anon)
        {
            std::cerr << "IdEditDialog::setupNewId: Error. Cannot init with pseudo-anonymous id when anon ids are disabled." << std::endl;
            pseudo = false ;
        }
        
	mIsNew = true;
	mGroupId.clear();

	ui->lineEdit_KeyId->setText(tr("To be generated"));
	ui->lineEdit_Nickname->setText("");
	ui->radioButton_GpgId->setEnabled(true);
	ui->radioButton_Pseudo->setEnabled(true);

	if (pseudo)
	{
		ui->radioButton_Pseudo->setChecked(true);
	}
	else
	{
		ui->radioButton_GpgId->setChecked(true);
	}

	// force - incase it wasn't triggered.
	idTypeToggled(true);

	ui->frame_Tags->setHidden(true);
	ui->radioButton_GpgId->setEnabled(true);
    
    	if(enable_anon)
		ui->radioButton_Pseudo->setEnabled(true);
        else
		ui->radioButton_Pseudo->setEnabled(false);

	setAvatar(QPixmap());

    // force resize of dialog, to hide empty space from the hidden recogn tags area
    adjustSize();
}

void IdEditDialog::idTypeToggled(bool checked)
{
	if (checked)
	{
		bool pseudo = ui->radioButton_Pseudo->isChecked();
		updateIdType(pseudo);
	}
}

void IdEditDialog::updateIdType(bool pseudo)
{
	if (pseudo)
	{
		ui->lineEdit_GpgHash->setText(tr("N/A"));
		ui->lineEdit_GpgId->setText(tr("N/A"));
		ui->lineEdit_GpgName->setText(tr("N/A"));
	}
	else
	{
		/* get GPG Details from rsPeers */
		RsPgpId gpgid = rsPeers->getGPGOwnId();
		RsPeerDetails details;
		rsPeers->getGPGDetails(gpgid, details);

		ui->lineEdit_GpgId->setText(QString::fromStdString(gpgid.toStdString()));
		ui->lineEdit_GpgHash->setText(tr("To be generated"));
		ui->lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
	}
}

void IdEditDialog::setAvatar(const QPixmap &avatar)
{
	mAvatar = avatar;

	if (!mAvatar.isNull()) {
		ui->avatarLabel->setPixmap(mAvatar);
	} else {
		// we need to use the default pixmap here, generated from the ID
		ui->avatarLabel->setPixmap(GxsIdDetails::makeDefaultIcon(RsGxsId(mEditGroup.mMeta.mGroupId)));
	}
}

void IdEditDialog::setupExistingId(const RsGxsGroupId& keyId)
{
	setWindowTitle(tr("Edit identity"));
	ui->headerFrame->setHeaderImage(QPixmap(":/icons/png/person.png"));
	ui->headerFrame->setHeaderText(tr("Edit identity"));

	mStateHelper->setLoading(IDEDITDIALOG_LOADID, true);

	mIsNew = false;
	mGroupId.clear();

	RsThread::async([this,keyId]()
	{
		std::vector<RsGxsIdGroup> datavector;

        bool res = rsIdentity->getIdentitiesInfo(std::set<RsGxsId>({(RsGxsId)keyId}),datavector);

		RsQThreadUtils::postToObject( [this,keyId,res,datavector]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

			mStateHelper->setLoading(IDEDITDIALOG_LOADID, false);

			/* get details from libretroshare */

			if (!res || datavector.size() != 1)
			{
				std::cerr << __PRETTY_FUNCTION__ << " failed to collect group info for identity " << keyId << std::endl;

				ui->lineEdit_KeyId->setText(tr("Error KeyID invalid"));
				ui->lineEdit_Nickname->setText("");

				ui->lineEdit_GpgHash->setText(tr("N/A"));
				ui->lineEdit_GpgId->setText(tr("N/A"));
				ui->lineEdit_GpgName->setText(tr("N/A"));
				return;
			}

            loadExistingId(datavector[0]);

		}, this );
	});
}

void IdEditDialog::enforceNoAnonIds()
{
    ui->radioButton_GpgId->setChecked(true);
    ui->radioButton_GpgId->setEnabled(false);
    ui->radioButton_Pseudo->setEnabled(false);
}

void IdEditDialog::loadExistingId(const RsGxsIdGroup& id_group)
{
	mEditGroup = id_group;
	mGroupId = mEditGroup.mMeta.mGroupId;

	QPixmap avatar;
	if (mEditGroup.mImage.mSize > 0)
		GxsIdDetails::loadPixmapFromData(mEditGroup.mImage.mData, mEditGroup.mImage.mSize, avatar,GxsIdDetails::LARGE);

	setAvatar(avatar);

	bool realid = (mEditGroup.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility);

	if (realid)
	{
		ui->radioButton_GpgId->setChecked(true);
	}
	else
	{
		ui->radioButton_Pseudo->setChecked(true);
	}
	// these are not editable for existing Id.
	ui->radioButton_GpgId->setEnabled(false);
	ui->radioButton_Pseudo->setEnabled(false);

	// DOES THIS TRIGGER ALREADY???
	// force - incase it wasn't triggered.
	idTypeToggled(true);

    ui->lineEdit_Nickname->setText(QString::fromUtf8(mEditGroup.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));
	ui->lineEdit_KeyId->setText(QString::fromStdString(mEditGroup.mMeta.mGroupId.toStdString()));

	if (realid)
	{
		ui->lineEdit_GpgHash->setText(QString::fromStdString(mEditGroup.mPgpIdHash.toStdString()));

		if (mEditGroup.mPgpKnown)
		{
			RsPeerDetails details;
			rsPeers->getGPGDetails(mEditGroup.mPgpId, details);
			ui->lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));

			ui->lineEdit_GpgId->setText(QString::fromStdString(mEditGroup.mPgpId.toStdString()));
		}
		else
		{
			ui->lineEdit_GpgId->setText(tr("Unknown GpgId"));
			ui->lineEdit_GpgName->setText(tr("Unknown real name"));
		}
	}
	else
	{
		ui->lineEdit_GpgHash->setText(tr("N/A"));
		ui->lineEdit_GpgId->setText(tr("N/A"));
		ui->lineEdit_GpgName->setText(tr("N/A"));
	}

	// RecognTags.
	ui->frame_Tags->setHidden(false);

	loadRecognTags();
}

#define MAX_RECOGN_TAGS 	5

void IdEditDialog::checkNewTag()
{
	std::string tag = ui->plainTextEdit_Tag->toPlainText().toStdString();
    RsGxsId id ( ui->lineEdit_KeyId->text().toStdString());
    std::string name = ui->lineEdit_Nickname->text().left(RSID_MAXIMUM_NICKNAME_SIZE).toUtf8().data();

	QString desc;
	bool ok = tagDetails(id, name, tag, desc);
	ui->label_TagCheck->setText(desc);

	// hack to allow add invalid tags (for testing).
	if (!tag.empty())
	{
		ok = true;
	}

	if (mEditGroup.mRecognTags.size() >= MAX_RECOGN_TAGS)
	{
		ok = false;
	}

	ui->pushButton_Tag->setEnabled(ok);
}

void IdEditDialog::addRecognTag()
{
	std::string tag = ui->plainTextEdit_Tag->toPlainText().toStdString();
	if (mEditGroup.mRecognTags.size() >= MAX_RECOGN_TAGS)
	{
		std::cerr << "IdEditDialog::addRecognTag() Too many Tags, delete one first";
		std::cerr << std::endl;
	}

	mEditGroup.mRecognTags.push_back(tag);
	loadRecognTags();
}

void IdEditDialog::rmTag1()
{
	rmTag(0);
}

void IdEditDialog::rmTag2()
{
	rmTag(1);
}

void IdEditDialog::rmTag3()
{
	rmTag(2);
}

void IdEditDialog::rmTag4()
{
	rmTag(3);
}

void IdEditDialog::rmTag5()
{
	rmTag(4);
}
	
void IdEditDialog::rmTag(int idx)
{
	std::list<std::string>::iterator it;
	int i = 0;
	for(it = mEditGroup.mRecognTags.begin(); it != mEditGroup.mRecognTags.end() && (idx < i); ++it, ++i) ;

	if (it != mEditGroup.mRecognTags.end())
	{
		mEditGroup.mRecognTags.erase(it);
	}
	loadRecognTags();
}

bool IdEditDialog::tagDetails(const RsGxsId &id, const std::string &name, const std::string &tag, QString &desc)
{
	if (tag.empty())
	{
		desc += "Empty Tag";
		return false;
	}
		
	/* extract details for each tag */
	RsRecognTagDetails tagDetails;

	bool ok = false;
	if (rsIdentity->parseRecognTag(id, name, tag, tagDetails))
	{
		desc += QString::number(tagDetails.tag_class);
		desc += ":";
		desc += QString::number(tagDetails.tag_type);

		if (tagDetails.is_valid)
		{
			ok = true;
			desc += " Valid";
		}
		else
		{
			desc += " Invalid";
		}

		if (tagDetails.is_pending)
		{
			ok = true;
			desc += " Pending";
		}
	}
	else
	{
		desc += "Unparseable";
	}
	return ok;
}

void IdEditDialog::loadRecognTags()
{
	std::cerr << "IdEditDialog::loadRecognTags()";
	std::cerr << std::endl;

	// delete existing items.
	ui->label_Tag1->setHidden(true);
	ui->label_Tag2->setHidden(true);
	ui->label_Tag3->setHidden(true);
	ui->label_Tag4->setHidden(true);
	ui->label_Tag5->setHidden(true);
	ui->toolButton_Tag1->setHidden(true);
	ui->toolButton_Tag2->setHidden(true);
	ui->toolButton_Tag3->setHidden(true);
	ui->toolButton_Tag4->setHidden(true);
	ui->toolButton_Tag5->setHidden(true);
	ui->plainTextEdit_Tag->setPlainText("");

	int i = 0;
	std::list<std::string>::const_iterator it;
	for(it = mEditGroup.mRecognTags.begin(); it != mEditGroup.mRecognTags.end(); ++it, ++i)
	{
		QString recognTag;
        tagDetails(mEditGroup.mMeta.mAuthorId, mEditGroup.mMeta.mGroupName, *it, recognTag);

		switch(i)
		{
			default:
			case 0:
				ui->label_Tag1->setText(recognTag);
				ui->label_Tag1->setHidden(false);
				ui->toolButton_Tag1->setHidden(false);
				break;
			case 1:
				ui->label_Tag2->setText(recognTag);
				ui->label_Tag2->setHidden(false);
				ui->toolButton_Tag2->setHidden(false);
				break;
			case 2:
				ui->label_Tag3->setText(recognTag);
				ui->label_Tag3->setHidden(false);
				ui->toolButton_Tag3->setHidden(false);
				break;
			case 3:
				ui->label_Tag4->setText(recognTag);
				ui->label_Tag4->setHidden(false);
				ui->toolButton_Tag4->setHidden(false);
				break;
			case 4:
				ui->label_Tag5->setText(recognTag);
				ui->label_Tag5->setHidden(false);
				ui->toolButton_Tag5->setHidden(false);
				break;
		}
	}
}

void IdEditDialog::submit()
{
	if (mIsNew)
	{
		createId();
	}
	else
	{
		updateId();
	}
}

void IdEditDialog::createId()
{
    QString groupname = ui->lineEdit_Nickname->text();

	if (groupname.size() < RSID_MINIMUM_NICKNAME_SIZE)
	{
		std::cerr << "IdEditDialog::createId() Nickname too short (min " << RSID_MINIMUM_NICKNAME_SIZE << " chars)";
		std::cerr << std::endl;

		QMessageBox::warning(this, "", tr("The nickname is too short. Please input at least %1 characters.").arg(RSID_MINIMUM_NICKNAME_SIZE), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
    if (groupname.size() > RSID_MAXIMUM_NICKNAME_SIZE)
    {
        std::cerr << "IdEditDialog::createId() Nickname too long (max " << RSID_MAXIMUM_NICKNAME_SIZE << " chars)";
        std::cerr << std::endl;

        QMessageBox::warning(this, "", tr("The nickname is too long. Please reduce the length to %1 characters.").arg(RSID_MAXIMUM_NICKNAME_SIZE), QMessageBox::Ok, QMessageBox::Ok);

        return;
    }
	RsIdentityParameters params;
    params.nickname = groupname.toUtf8().constData();
	params.isPgpLinked = (ui->radioButton_GpgId->isChecked());

	if (!mAvatar.isNull())
	{
		QByteArray ba;
		QBuffer buffer(&ba);

		buffer.open(QIODevice::WriteOnly);
		mAvatar.save(&buffer, "PNG"); // writes image into ba in PNG format

		params.mImage.copy((uint8_t *) ba.data(), ba.size());
	}
	else
		params.mImage.clear();

    RsGxsId keyId;
    std::string gpg_password;

    if(params.isPgpLinked)
    {
		std::string gpg_name = rsPeers->getGPGName(rsPeers->getGPGOwnId());
        bool cancelled;

        if(!NotifyQt::getInstance()->askForPassword(tr("Profile password needed.").toStdString(),
		                                            gpg_name + " (" + rsPeers->getOwnId().toStdString() + ")",
		                                            false,
		                                            gpg_password,cancelled))
		{
			QMessageBox::critical(NULL,tr("Identity creation failed"),tr("Cannot create an identity linked to your profile without your profile password."));
			return;
        }

    }

    if(rsIdentity->createIdentity(keyId,params.nickname,params.mImage,!params.isPgpLinked,gpg_password))
    {
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

        RsIdentityDetails det;

        if(rsIdentity->getIdDetails(keyId,det))
        {
            QMessageBox::information(NULL,tr("Identity creation success"),tr("Your new identity was successfuly created."));
            close();
        }
        else
			QMessageBox::critical(NULL,tr("Identity creation failed"),tr("Cannot create identity. Something went wrong."));
    }
    else
        QMessageBox::critical(NULL,tr("Identity creation failed"),tr("Cannot create identity. Something went wrong. Check your profile password."));
}

#ifdef TO_REMOVE
void IdEditDialog::idCreated(uint32_t token)
{
	if (!rsIdentity->acknowledgeGrp(token, mGroupId)) {
		std::cerr << "IdDialog::idCreated() acknowledgeGrp failed";
		std::cerr << std::endl;

		reject();
		return;
	}

	accept();
}
#endif

void IdEditDialog::updateId()
{
	/* submit updated details */
    QString groupname = ui->lineEdit_Nickname->text();

	if (groupname.size() < 2)
	{
		std::cerr << "IdEditDialog::updateId() Nickname too short";
		std::cerr << std::endl;
		return;
	}
    if (groupname.size() > RSID_MAXIMUM_NICKNAME_SIZE)
    {
        std::cerr << "IdEditDialog::createId() Nickname too long (max " << RSID_MAXIMUM_NICKNAME_SIZE << " chars)";
        std::cerr << std::endl;
        return;
    }

    mEditGroup.mMeta.mGroupName = groupname.toUtf8().constData();

	if (!mAvatar.isNull())
	{
		QByteArray ba;
		QBuffer buffer(&ba);

		buffer.open(QIODevice::WriteOnly);
		mAvatar.save(&buffer, "PNG"); // writes image into ba in PNG format

		mEditGroup.mImage.copy((uint8_t *) ba.data(), ba.size());
	}
	else
		mEditGroup.mImage.clear();

	uint32_t dummyToken = 0;
	rsIdentity->updateIdentity(mEditGroup);

	accept();
}

#ifdef TO_REMOVE
void IdEditDialog::loadRequest(const TokenQueue */*queue*/, const TokenRequest &req)
{
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	switch (req.mUserType) {
	case IDEDITDIALOG_LOADID:
		loadExistingId(req.mToken);
		break;

	case IDEDITDIALOG_CREATEID:
		idCreated(req.mToken);
		break;
	}

}
#endif
